/**
 * @file test_rtsp_message.cpp
 * @brief Test suite for RTSPMessage class
 */

#include <cassert>
#include <iostream>
#include <string>
#include "../include/network/RTSPMessage.hpp"

// Simple test framework
class TestReporter {
  private:
    int passed = 0;
    int failed = 0;
    std::string currentTest;

  public:
    void startTest(const std::string& name) {
        currentTest = name;
        std::cout << "Testing: " << name << " ... ";
    }

    void pass() {
        passed++;
        std::cout << "✓ PASS" << std::endl;
    }

    void fail(const std::string& message) {
        failed++;
        std::cout << "✗ FAIL: " << message << std::endl;
    }

    void report() {
        std::cout << "\n========================================\n";
        std::cout << "Total: " << (passed + failed) << " tests\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "========================================\n";
    }

    bool allPassed() const { return failed == 0; }
};

TestReporter reporter;

// Test helper
#define TEST(name) reporter.startTest(name)
#define ASSERT(condition, message) \
    if (!(condition)) {            \
        reporter.fail(message);    \
        return;                    \
    }
#define PASS() reporter.pass()

// ==================== Test Cases ====================

void test_buildRequest_basic() {
    TEST("buildRequest - Basic SETUP");

    std::string request = RTSPMessage::buildRequest("SETUP", "movie.Mjpeg", 1);

    ASSERT(request.find("SETUP movie.Mjpeg RTSP/1.0\r\n") != std::string::npos,
           "Missing request line");
    ASSERT(request.find("CSeq: 1\r\n") != std::string::npos, "Missing CSeq header");
    ASSERT(request.find("\r\n\r\n") != std::string::npos, "Missing double CRLF");

    PASS();
}

void test_buildRequest_withHeaders() {
    TEST("buildRequest - With Transport header");

    std::map<std::string, std::string> headers;
    headers["Transport"] = "RTP/UDP; client_port=25000";

    std::string request = RTSPMessage::buildRequest("SETUP", "movie.Mjpeg", 1, headers);

    ASSERT(request.find("Transport: RTP/UDP; client_port=25000\r\n") != std::string::npos,
           "Missing Transport header");

    PASS();
}

void test_buildRequest_multipleHeaders() {
    TEST("buildRequest - Multiple headers");

    std::map<std::string, std::string> headers;
    headers["Transport"] = "RTP/UDP; client_port=25000";
    headers["Session"] = "123456789";

    std::string request = RTSPMessage::buildRequest("PLAY", "movie.Mjpeg", 2, headers);

    ASSERT(request.find("PLAY movie.Mjpeg RTSP/1.0\r\n") != std::string::npos,
           "Missing request line");
    ASSERT(request.find("CSeq: 2\r\n") != std::string::npos, "Missing CSeq header");
    ASSERT(request.find("Transport:") != std::string::npos, "Missing Transport header");
    ASSERT(request.find("Session:") != std::string::npos, "Missing Session header");

    PASS();
}

void test_parseRequest_basic() {
    TEST("parseRequest - Basic SETUP");

    std::string message = "SETUP movie.Mjpeg RTSP/1.0\r\n"
                          "CSeq: 1\r\n"
                          "Transport: RTP/UDP; client_port=25000\r\n"
                          "\r\n";

    RTSPMessage::Request req = RTSPMessage::parseRequest(message);

    ASSERT(req.method == "SETUP", "Method mismatch");
    ASSERT(req.url == "movie.Mjpeg", "URL mismatch");
    ASSERT(req.cseq == 1, "CSeq mismatch");
    ASSERT(req.headers["Transport"] == "RTP/UDP; client_port=25000", "Transport header mismatch");

    PASS();
}

void test_parseRequest_allMethods() {
    TEST("parseRequest - All RTSP methods");

    std::string methods[] = {"SETUP", "PLAY", "PAUSE", "TEARDOWN"};

    for (const auto& method : methods) {
        std::string message = method + " movie.Mjpeg RTSP/1.0\r\n"
                                       "CSeq: 1\r\n"
                                       "\r\n";

        RTSPMessage::Request req = RTSPMessage::parseRequest(message);

        ASSERT(req.method == method, "Method parsing failed for " + method);
    }

    PASS();
}

void test_buildResponse_basic() {
    TEST("buildResponse - 200 OK");

    std::string response = RTSPMessage::buildResponse(200, "OK", 1);

    ASSERT(response.find("RTSP/1.0 200 OK\r\n") != std::string::npos, "Missing status line");
    ASSERT(response.find("CSeq: 1\r\n") != std::string::npos, "Missing CSeq header");

    PASS();
}

void test_buildResponse_withSession() {
    TEST("buildResponse - With Session ID");

    std::map<std::string, std::string> headers;
    headers["Session"] = "987654321";
    headers["Transport"] = "RTP/UDP; server_port=25000";

    std::string response = RTSPMessage::buildResponse(200, "OK", 1, headers);

    ASSERT(response.find("Session: 987654321\r\n") != std::string::npos, "Missing Session header");
    ASSERT(response.find("Transport: RTP/UDP; server_port=25000\r\n") != std::string::npos,
           "Missing Transport header");

    PASS();
}

void test_parseResponse_success() {
    TEST("parseResponse - 200 OK");

    std::string message = "RTSP/1.0 200 OK\r\n"
                          "CSeq: 1\r\n"
                          "Session: 123456789\r\n"
                          "\r\n";

    RTSPMessage::Response res = RTSPMessage::parseResponse(message);

    ASSERT(res.statusCode == 200, "Status code mismatch");
    ASSERT(res.reason == "OK", "Reason mismatch");
    ASSERT(res.cseq == 1, "CSeq mismatch");
    ASSERT(res.headers["Session"] == "123456789", "Session header mismatch");

    PASS();
}

void test_parseResponse_errorCodes() {
    TEST("parseResponse - Error codes");

    int codes[] = {404, 454, 500};
    std::string reasons[] = {"Not Found", "Session Not Found", "Internal Server Error"};

    for (int i = 0; i < 3; i++) {
        std::string message = "RTSP/1.0 " + std::to_string(codes[i]) + " " + reasons[i] +
                              "\r\n"
                              "CSeq: 1\r\n"
                              "\r\n";

        RTSPMessage::Response res = RTSPMessage::parseResponse(message);

        ASSERT(res.statusCode == codes[i], "Status code mismatch for " + std::to_string(codes[i]));
        ASSERT(res.reason == reasons[i], "Reason mismatch for " + reasons[i]);
    }

    PASS();
}

void test_statusCodeToReason() {
    TEST("statusCodeToReason - Common codes");

    ASSERT(RTSPMessage::statusCodeToReason(200) == "OK", "200 reason mismatch");
    ASSERT(RTSPMessage::statusCodeToReason(400) == "Bad Request", "400 reason mismatch");
    ASSERT(RTSPMessage::statusCodeToReason(404) == "Not Found", "404 reason mismatch");
    ASSERT(RTSPMessage::statusCodeToReason(454) == "Session Not Found", "454 reason mismatch");
    ASSERT(RTSPMessage::statusCodeToReason(500) == "Internal Server Error", "500 reason mismatch");
    ASSERT(RTSPMessage::statusCodeToReason(999) == "Unknown Status", "Unknown code mismatch");

    PASS();
}

void test_getHeader() {
    TEST("getHeader - Get header value");

    std::map<std::string, std::string> headers;
    headers["Session"] = "123456789";
    headers["Transport"] = "RTP/UDP; client_port=25000";

    ASSERT(RTSPMessage::getHeader(headers, "Session") == "123456789", "Session header not found");
    ASSERT(RTSPMessage::getHeader(headers, "Transport") == "RTP/UDP; client_port=25000",
           "Transport header not found");
    ASSERT(RTSPMessage::getHeader(headers, "NonExistent", "default") == "default",
           "Default value not returned");

    PASS();
}

void test_roundtrip_request() {
    TEST("Roundtrip - Request build and parse");

    std::map<std::string, std::string> headers;
    headers["Transport"] = "RTP/UDP; client_port=25000";

    std::string built = RTSPMessage::buildRequest("SETUP", "movie.Mjpeg", 1, headers);
    RTSPMessage::Request parsed = RTSPMessage::parseRequest(built);

    ASSERT(parsed.method == "SETUP", "Method lost in roundtrip");
    ASSERT(parsed.url == "movie.Mjpeg", "URL lost in roundtrip");
    ASSERT(parsed.cseq == 1, "CSeq lost in roundtrip");
    ASSERT(parsed.headers["Transport"] == "RTP/UDP; client_port=25000", "Header lost in roundtrip");

    PASS();
}

void test_roundtrip_response() {
    TEST("Roundtrip - Response build and parse");

    std::map<std::string, std::string> headers;
    headers["Session"] = "987654321";

    std::string built = RTSPMessage::buildResponse(200, "OK", 1, headers);
    RTSPMessage::Response parsed = RTSPMessage::parseResponse(built);

    ASSERT(parsed.statusCode == 200, "Status code lost in roundtrip");
    ASSERT(parsed.reason == "OK", "Reason lost in roundtrip");
    ASSERT(parsed.cseq == 1, "CSeq lost in roundtrip");
    ASSERT(parsed.headers["Session"] == "987654321", "Session header lost in roundtrip");

    PASS();
}

void test_parseRequest_emptyMessage() {
    TEST("parseRequest - Empty message handling");

    try {
        RTSPMessage::parseRequest("");
        reporter.fail("Should throw exception for empty message");
    } catch (const std::runtime_error&) {
        PASS();
    }
}

void test_parseResponse_emptyMessage() {
    TEST("parseResponse - Empty message handling");

    try {
        RTSPMessage::parseResponse("");
        reporter.fail("Should throw exception for empty message");
    } catch (const std::runtime_error&) {
        PASS();
    }
}

// ==================== Main ====================

int main() {
    std::cout << "\n========================================\n";
    std::cout << "   RTSPMessage Test Suite\n";
    std::cout << "========================================\n\n";

    // Build tests
    test_buildRequest_basic();
    test_buildRequest_withHeaders();
    test_buildRequest_multipleHeaders();
    test_buildResponse_basic();
    test_buildResponse_withSession();

    // Parse tests
    test_parseRequest_basic();
    test_parseRequest_allMethods();
    test_parseResponse_success();
    test_parseResponse_errorCodes();

    // Utility tests
    test_statusCodeToReason();
    test_getHeader();

    // Roundtrip tests
    test_roundtrip_request();
    test_roundtrip_response();

    // Error handling tests
    test_parseRequest_emptyMessage();
    test_parseResponse_emptyMessage();

    reporter.report();

    return reporter.allPassed() ? 0 : 1;
}
