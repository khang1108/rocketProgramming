#include "network/RTSPServer.hpp"

#ifdef ERROR
#undef ERROR
#endif

RTSPServer::RTSPServer(int serverPort) : port(serverPort)
{
    running = false;
    nextClientId = 1;
}

void RTSPServer::run() 
{
    try {
        LOG_INFO("RTSPServer starting on port " + std::to_string(port));

        //Tạo Socket TCP
        listenSocket = std::make_unique<Socket>(SocketType::TCP);
        listenSocket->setReuseAddress(true);

        listenSocket->setTimeout(1000);
        
        //Bind (Gắn cổng)
        //bind("IP", port). "0.0.0.0" để lắng nghe mọi IP
        listenSocket->bind("0.0.0.0", port);

        //Listen (Lắng nghe)
        listenSocket->listen(SOMAXCONN); 

        running = true;
        LOG_INFO("RTSPServer is listening...");

        //Vòng lặp chính chấp nhận kết nối
        while (running) {
            try {
                //Accept: Chờ Client kết nối
                std::unique_ptr<Socket> clientSock = listenSocket->accept();

                //Kiểm tra nếu server đã bị stop khi đang chờ accept
                if (!running) break;

                int id = nextClientId++;
                LOG_INFO("New client connected. ID: " + std::to_string(id));

                {
                    std::lock_guard<std::mutex> lock(sessionMutex);

                    //Tạo Worker để xử lý riêng cho Client này
                    auto worker = std::make_unique<ServerWorker>(id, std::move(clientSock));

                    //Tạo Thread để chạy worker->run()
                    std::thread t(&ServerWorker::run, worker.get());

                    //Lưu vào vector quản lý
                    activeSession.push_back({
                        id,
                        nullptr, //Socket đã chuyển vào worker nên ở đây null
                        std::move(worker),
                        std::move(t)
                    });
                }
            } catch (const SocketTimeout& e) {
                continue;
            } catch (const std::exception& e) {
                if (running) {
                    LOG_ERROR("Error accepting client: " + std::string(e.what()));
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal server error: " + std::string(e.what()));
        stop();
    }
}

void RTSPServer::stop() 
{
    LOG_INFO("Stopping RTSPServer...");
    running = false;

    //Đóng socket lắng nghe để thoát vòng lặp accept
    if (listenSocket) {
        listenSocket->close();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //Dọn dẹp các luồng con
    std::lock_guard<std::mutex> lock(sessionMutex);
    for (auto& session : activeSession) {
        session.worker->stop();
        
        //Chờ luồng kết thúc
        if (session.workerThread.joinable()) {
            session.workerThread.join();
        }
    }
    activeSession.clear();
    LOG_INFO("RTSPServer stopped.");
}