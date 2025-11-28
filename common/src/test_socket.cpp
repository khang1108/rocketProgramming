#include "Socket.hpp"
#include <iostream>

void testTCPServer() {
    std::cout << "Testing TCP Server..." << std::endl;
    try {
        Socket server(SocketType::TCP);
        server.setReuseAddress(true);
        server.bind("0.0.0.0", 8554);
        server.listen(5);
        std::cout << "✅ TCP Server: bind & listen OK" << std::endl;
        
        int port = server.getLocalPort();
        std::cout << "✅ Server listening on port: " << port << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ TCP Server failed: " << e.what() << std::endl;
    }
}

void testTCPClient() {
    std::cout << "\nTesting TCP Client..." << std::endl;
    try {
        Socket client(SocketType::TCP);
        client.connect("127.0.0.1", 80);  // Try connect to localhost:80
        std::cout << "✅ TCP Client: connect OK" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "ℹ️  TCP Client failed (expected if no server): " << e.what() << std::endl;
    }
}

void testUDPSocket() {
    std::cout << "\nTesting UDP Socket..." << std::endl;
    try {
        Socket udp(SocketType::UDP);
        udp.bind("0.0.0.0", 0);  // Let OS assign port
        int port = udp.getLocalPort();
        std::cout << "✅ UDP Socket: bind OK on port " << port << std::endl;
        
        // Test sendTo
        const char* msg = "Hello";
        udp.sendTo((const uint8_t*)msg, 5, "127.0.0.1", port);
        std::cout << "✅ UDP Socket: sendTo OK" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ UDP Socket failed: " << e.what() << std::endl;
    }
}

void testSocketOptions() {
    std::cout << "\nTesting Socket Options..." << std::endl;
    try {
        Socket sock(SocketType::TCP);
        
        sock.setReuseAddress(true);
        std::cout << "✅ setReuseAddress OK" << std::endl;
        
        sock.setTimeout(1000);
        std::cout << "✅ setTimeout OK" << std::endl;
        
        sock.setNonBlocking(true);
        std::cout << "✅ setNonBlocking OK" << std::endl;
        
        sock.setNoDelay(true);
        std::cout << "✅ setNoDelay OK" << std::endl;
        
        sock.setBufferSize(65536, 65536);
        std::cout << "✅ setBufferSize OK" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Socket Options failed: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Socket.cpp Test Suite ===" << std::endl;
    
    testTCPServer();
    testTCPClient();
    testUDPSocket();
    testSocketOptions();
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}