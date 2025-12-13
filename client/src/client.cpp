#include <iostream>
#include <string>

#include <QApplication>

#include "ui/ClientUI.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"

int main(int argc, char* argv[]) {
    try {
        QApplication app(argc, argv);

        Logger::getInstance().initialize("client.log", LogLevel::INFO);

        LOG_INFO("========================================");
        LOG_INFO("   RTSP/RTP Video Streaming Client (Qt)");
        LOG_INFO("========================================");

        LOG_INFO("Arguments: " + std::to_string(argc) + " " + std::string(argv[1]) + " " +
                 std::string(argv[2]) + " " + std::string(argv[3]) + " " + std::string(argv[4]));

        std::string serverIP = "127.0.0.1";
        int serverPort = 8554;
        std::string videoFile = "movie.Mjpeg";
        int clientRTPPort = 25000;

        if (argc >= 2) {
            serverIP = argv[1];
        }
        if (argc >= 3) {
            serverPort = std::stoi(argv[2]);
        }
        if (argc >= 4) {
            videoFile = argv[3];
        }
        if (argc >= 5) {
            clientRTPPort = std::stoi(argv[4]);
        }

        std::cout << "Configuration:\n";
        std::cout << "  Server IP:    " << serverIP << "\n";
        std::cout << "  Server Port:  " << serverPort << "\n";
        std::cout << "  Video File:   " << videoFile << "\n";
        std::cout << "  RTP Port:     " << clientRTPPort << "\n";
        std::cout << "========================================\n\n";

        LOG_INFO("Server: " + serverIP + ":" + std::to_string(serverPort));
        LOG_INFO("Video: " + videoFile);
        LOG_INFO("RTP Port: " + std::to_string(clientRTPPort));

        ClientUI clientUI(serverIP, serverPort, videoFile, clientRTPPort);

        if (!clientUI.initialize()) {
            std::cerr << "Failed to initialize ClientUI\n";
            LOG_ERROR("Failed to initialize ClientUI");
            return 1;
        }

        LOG_INFO("ClientUI initialized successfully");

        clientUI.show();

        int ret = app.exec();

        LOG_INFO("Client application exiting (Qt), code=" + std::to_string(ret));
        std::cout << "\nThank you for using RTSP Client (Qt)!\n";
        return ret;
    } catch (const std::exception& e) {
        LOG_ERROR("FATAL ERROR: " + std::string(e.what()));
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}