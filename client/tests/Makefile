CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../../common/include -I../include -I../../server/include -pthread
# Special flags for server simulation test (no client include to avoid RTPPacket conflict)
CXXFLAGS_SERVER_SIM = -std=c++17 -Wall -Wextra -I../../common/include -I../../server/include -pthread
LDFLAGS = -pthread

# Directories
BUILD_DIR = build
SRC_DIR = ../src
COMMON_SRC = ../../common/src
SERVER_SRC = ../../server/src

# Source files
FRAME_BUFFER_SRC = $(SRC_DIR)/buffer/FrameBuffer.cpp
RTP_PACKET_SRC = $(SERVER_SRC)/rtp/RTPPacket.cpp
FRAME_REASSEMBLER_SRC = $(SRC_DIR)/rtp/FrameReassembler.cpp
LOGGER_SRC = $(COMMON_SRC)/utils/Logger.cpp
VIDEO_STREAM_SRC = $(SERVER_SRC)/video/VideoStream.cpp
ENCODING_STRATEGY_SRC = $(SERVER_SRC)/rtp/EncodingStrategy.cpp
RTP_PACKET_BUILDER_SRC = $(SERVER_SRC)/rtp/RTPPacketBuilder.cpp

# Test targets
TEST_RTP_PIPELINE = test_rtp_pipeline
TEST_SERVER_SIM = test_server_simulation

.PHONY: all test clean

all: $(BUILD_DIR) $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Test RTP Pipeline (normal)
$(TEST_RTP_PIPELINE): $(BUILD_DIR)/test_rtp_pipeline.o $(BUILD_DIR)/FrameBuffer.o \
                      $(BUILD_DIR)/RTPPacket.o $(BUILD_DIR)/FrameReassembler.o \
                      $(BUILD_DIR)/Logger.o
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_rtp_pipeline.o: test_rtp_pipeline.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Test Server Simulation (special: use server's RTPPacket, avoid client include)
$(TEST_SERVER_SIM): $(BUILD_DIR)/test_server_simulation.o \
                    $(BUILD_DIR)/FrameBuffer_serversim.o \
                    $(BUILD_DIR)/RTPPacket_server.o \
                    $(BUILD_DIR)/FrameReassembler_serversim.o \
                    $(BUILD_DIR)/Logger_serversim.o \
                    $(BUILD_DIR)/VideoStream.o \
                    $(BUILD_DIR)/EncodingStrategy.o \
                    $(BUILD_DIR)/RTPPacketBuilder.o
	$(CXX) $^ -o $@ $(LDFLAGS)

# Special compile for test_server_simulation (no client include path)
$(BUILD_DIR)/test_server_simulation.o: test_server_simulation.cpp
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

# Compile client sources with server-compatible flags (for server sim test)
$(BUILD_DIR)/FrameBuffer_serversim.o: $(FRAME_BUFFER_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

$(BUILD_DIR)/FrameReassembler_serversim.o: $(FRAME_REASSEMBLER_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -I$(SRC_DIR)/../include/buffer -c $< -o $@

$(BUILD_DIR)/Logger_serversim.o: $(LOGGER_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

# Normal client components (for rtp_pipeline test)
$(BUILD_DIR)/FrameBuffer.o: $(FRAME_BUFFER_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/RTPPacket.o: $(RTP_PACKET_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/FrameReassembler.o: $(FRAME_REASSEMBLER_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Logger.o: $(LOGGER_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Server components (only for server sim test)
$(BUILD_DIR)/RTPPacket_server.o: $(RTP_PACKET_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

$(BUILD_DIR)/VideoStream.o: $(VIDEO_STREAM_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

$(BUILD_DIR)/EncodingStrategy.o: $(ENCODING_STRATEGY_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

$(BUILD_DIR)/RTPPacketBuilder.o: $(RTP_PACKET_BUILDER_SRC)
	$(CXX) $(CXXFLAGS_SERVER_SIM) -c $< -o $@

test: $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM)
	./$(TEST_RTP_PIPELINE)
	./$(TEST_SERVER_SIM)

clean:
	rm -rf $(BUILD_DIR) $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM)

# Add to existing Makefile

# RTSPClient test
TEST_RTSP_CLIENT = test_rtsp_client

all: $(BUILD_DIR) $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM) $(TEST_RTSP_CLIENT)

$(TEST_RTSP_CLIENT): $(BUILD_DIR)/test_rtsp_client.o \
                     $(BUILD_DIR)/Socket.o \
                     $(BUILD_DIR)/RTSPClient.o
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_rtsp_client.o: test_rtsp_client.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Socket.o: $(COMMON_SRC)/network/Socket.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/RTSPClient.o: $(SRC_DIR)/network/RTSPClient.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM) $(TEST_RTSP_CLIENT)