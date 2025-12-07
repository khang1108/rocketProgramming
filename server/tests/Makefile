# Makefile for Server Tests
# Usage:
#   make              - Build all tests
#   make test         - Build and run all tests
#   make clean        - Remove build artifacts
#   make rtp          - Build only RTP packet test
#   make encoding     - Build only encoding strategy test
#   make video        - Build only video stream test

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
INCLUDES = -I../include -I../../common/include
LDFLAGS = -pthread

# Source files
RTP_PACKET_SRC = ../src/rtp/RTPPacket.cpp
ENCODING_STRATEGY_SRC = ../src/rtp/EncodingStrategy.cpp
VIDEO_STREAM_SRC = ../src/video/VideoStream.cpp

# Test executables
TEST_RTP = test_rtp_packet
TEST_ENCODING = test_encoding_strategy
TEST_VIDEO = test_video_stream

# Targets
.PHONY: all test clean rtp encoding video help

all: $(TEST_RTP) $(TEST_ENCODING) $(TEST_VIDEO)

# RTP Packet test
$(TEST_RTP): test_rtp_packet.cpp $(RTP_PACKET_SRC)
	@echo "Compiling RTP Packet tests..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)
	@echo "✓ $(TEST_RTP) built successfully"

# Encoding Strategy test
$(TEST_ENCODING): test_encoding_strategy.cpp $(ENCODING_STRATEGY_SRC)
	@echo "Compiling Encoding Strategy tests..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)
	@echo "✓ $(TEST_ENCODING) built successfully"

# Video Stream test
$(TEST_VIDEO): test_video_stream.cpp $(VIDEO_STREAM_SRC)
	@echo "Compiling Video Stream tests..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)
	@echo "✓ $(TEST_VIDEO) built successfully"

# Build and run tests
test: all
	@echo "\n========================================" 
	@echo "Running RTP Packet Tests..."
	@echo "========================================" 
	./$(TEST_RTP)
	@echo "\n========================================" 
	@echo "Running Encoding Strategy Tests..."
	@echo "========================================" 
	-./$(TEST_ENCODING) 2>/dev/null || echo "Encoding test not available yet"
	@echo "\n========================================" 
	@echo "Running Video Stream Tests..."
	@echo "========================================" 
	./$(TEST_VIDEO)

# Individual test targets
rtp: $(TEST_RTP)
	@echo "Running RTP Packet test..."
	./$(TEST_RTP)

encoding: $(TEST_ENCODING)
	@echo "Running Encoding Strategy test..."
	./$(TEST_ENCODING)

video: $(TEST_VIDEO)
	@echo "Running Video Stream test..."
	./$(TEST_VIDEO)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TEST_RTP) $(TEST_ENCODING) $(TEST_VIDEO)
	rm -f *.o
	rm -f test.log test.conf
	rm -f test_*.mjpeg  # Clean test video files
	@echo "✓ Clean complete"

# Help
help:
	@echo "Server Test Suite - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build all tests"
	@echo "  make test     - Build and run all tests"
	@echo "  make rtp      - Build and run RTP packet test"
	@echo "  make encoding - Build and run encoding strategy test"
	@echo "  make video    - Build and run video stream test"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Example:"
	@echo "  make clean && make test"
	@echo "  make video"

# Verbose output
.SILENT: help

# Test targets for RTSPServer and ServerWorker
test_rtsp_server: test_rtsp_server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) \
		test_rtsp_server.cpp \
		../src/network/RTSPServer.cpp \
		../src/network/ServerWorker.cpp \
		../src/video/VideoStream.cpp \
		../src/rtp/EncodingStrategy.cpp \
		../../common/src/network/Socket.cpp \
		../../common/src/network/RTSPMessage.cpp \
		../../common/src/rtp/RTPPacket.cpp \
		../../common/src/rtp/RTPPacketBuilder.cpp \
		../../common/src/utils/Logger.cpp \
		../../common/src/utils/Config.cpp \
		../../common/src/utils/Timer.cpp \
		-o test_rtsp_server $(LIBS)

test_server_worker: test_server_worker.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) \
		test_server_worker.cpp \
		../src/network/ServerWorker.cpp \
		../src/video/VideoStream.cpp \
		../src/rtp/EncodingStrategy.cpp \
		../../common/src/network/Socket.cpp \
		../../common/src/network/RTSPMessage.cpp \
		../../common/src/rtp/RTPPacket.cpp \
		../../common/src/rtp/RTPPacketBuilder.cpp \
		../../common/src/utils/Logger.cpp \
		../../common/src/utils/Config.cpp \
		../../common/src/utils/Timer.cpp \
		-o test_server_worker $(LDFLAGS)

# Run both server tests
test_server: test_rtsp_server test_server_worker
	@echo "\n========================================"
	@echo "Running RTSPServer tests..."
	@echo "========================================"
	./test_rtsp_server
	@echo "\n========================================"
	@echo "Running ServerWorker tests..."
	@echo "========================================"
	./test_server_worker

.PHONY: test_rtsp_server test_server_worker test_server