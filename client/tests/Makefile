CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../../common/include -I../include -I../../server/include -pthread
LDFLAGS = -pthread

# Simplified OpenCV (only core modules)
OPENCV_LIBS = -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
OPENCV_FLAGS = $(shell pkg-config --cflags opencv4 2>/dev/null || echo "-I/usr/include/opencv4")
CXXFLAGS_DEMO = $(CXXFLAGS) -DUSE_OPENCV $(OPENCV_FLAGS)
LDFLAGS_DEMO = $(LDFLAGS) $(OPENCV_LIBS)

# Directories
BUILD_DIR = build
SRC_DIR = ../src
COMMON_SRC = ../../common/src
SERVER_SRC = ../../server/src

# Source files
FRAME_BUFFER_SRC = $(SRC_DIR)/buffer/FrameBuffer.cpp
RTP_PACKET_SRC = $(COMMON_SRC)/rtp/RTPPacket.cpp              # ← SERVER path, not common
RTP_PACKET_BUILDER_SRC = $(COMMON_SRC)/rtp/RTPPacketBuilder.cpp  # ← SERVER path
FRAME_REASSEMBLER_SRC = $(SRC_DIR)/rtp/FrameReassembler.cpp
LOGGER_SRC = $(COMMON_SRC)/utils/Logger.cpp
SOCKET_SRC = $(COMMON_SRC)/network/Socket.cpp
RTSP_CLIENT_SRC = $(SRC_DIR)/network/RTSPClient.cpp
RTP_RECEIVER_SRC = $(SRC_DIR)/rtp/RTPReceiver.cpp
CLIENT_UI_SRC = $(SRC_DIR)/ui/ClientUI.cpp
FRAME_DISPLAY_SRC = $(SRC_DIR)/ui/FrameDisplay.cpp

# Test target
TEST_CLIENTUI_DEMO = test_clientui_demo

.PHONY: all demo clean

all: $(BUILD_DIR) $(TEST_CLIENTUI_DEMO)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ClientUI Demo
$(TEST_CLIENTUI_DEMO): $(BUILD_DIR)/test_clientui_demo.o \
                       $(BUILD_DIR)/ClientUI.o \
                       $(BUILD_DIR)/FrameDisplay.o \
                       $(BUILD_DIR)/RTSPClient.o \
                       $(BUILD_DIR)/RTPReceiver.o \
                       $(BUILD_DIR)/FrameReassembler.o \
                       $(BUILD_DIR)/FrameBuffer.o \
                       $(BUILD_DIR)/RTPPacket.o \
                       $(BUILD_DIR)/RTPPacketBuilder.o \
                       $(BUILD_DIR)/Socket.o \
                       $(BUILD_DIR)/Logger.o
	$(CXX) $^ -o $@ $(LDFLAGS_DEMO)

$(BUILD_DIR)/test_clientui_demo.o: test_clientui_demo.cpp
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/ClientUI.o: $(CLIENT_UI_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/FrameDisplay.o: $(FRAME_DISPLAY_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/RTSPClient.o: $(RTSP_CLIENT_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/RTPReceiver.o: $(RTP_RECEIVER_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/FrameReassembler.o: $(FRAME_REASSEMBLER_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/FrameBuffer.o: $(FRAME_BUFFER_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/RTPPacket.o: $(RTP_PACKET_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/RTPPacketBuilder.o: $(RTP_PACKET_BUILDER_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/Socket.o: $(SOCKET_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

$(BUILD_DIR)/Logger.o: $(LOGGER_SRC)
	$(CXX) $(CXXFLAGS_DEMO) -c $< -o $@

demo: $(TEST_CLIENTUI_DEMO)
	@echo ""
	@echo "========================================"
	@echo "   Running ClientUI Demo"
	@echo "========================================"
	@echo ""
	./$(TEST_CLIENTUI_DEMO)

clean:
	rm -rf $(BUILD_DIR) $(TEST_CLIENTUI_DEMO)