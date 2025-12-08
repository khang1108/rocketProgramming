# Makefile for common library tests
# Usage:
#   make all          - Build all tests
#   make test         - Build and run all tests
#   make test_timer   - Build and run timer test
#   make test_rtsp    - Build and run RTSP message test
#   make clean        - Remove build artifacts

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I../include -I../src
LDFLAGS := -pthread

# Source directories
SRC_DIR := ../src
UTILS_SRC_DIR := $(SRC_DIR)/utils
NETWORK_SRC_DIR := $(SRC_DIR)/network

# Build directory
BUILD_DIR := build

# Test executables
TEST_TIMER := $(BUILD_DIR)/test_timer
TEST_RTSP := $(BUILD_DIR)/test_rtsp_message

# Source files for tests
TIMER_SOURCES := $(UTILS_SRC_DIR)/Timer.cpp test_timer.cpp
RTSP_SOURCES := $(NETWORK_SRC_DIR)/RTSPMessage.cpp test_rtsp_message.cpp

# Object files
TIMER_OBJECTS := $(BUILD_DIR)/Timer.o $(BUILD_DIR)/test_timer.o
RTSP_OBJECTS := $(BUILD_DIR)/RTSPMessage.o $(BUILD_DIR)/test_rtsp_message.o

# Colors for output
GREEN := \033[0;32m
YELLOW := \033[0;33m
RED := \033[0;31m
NC := \033[0m # No Color

# ==================== Main Targets ====================

.PHONY: all test clean run_timer run_rtsp test_timer test_rtsp

all: $(BUILD_DIR) $(TEST_TIMER) $(TEST_RTSP)
	@echo "$(GREEN)✓ All tests built successfully$(NC)"

test: all run_timer run_rtsp
	@echo ""
	@echo "$(GREEN)========================================$(NC)"
	@echo "$(GREEN)   All Tests Completed$(NC)"
	@echo "$(GREEN)========================================$(NC)"

# ==================== Build Targets ====================

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Timer test
$(TEST_TIMER): $(TIMER_OBJECTS)
	@echo "$(YELLOW)Linking test_timer...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✓ test_timer built$(NC)"

$(BUILD_DIR)/Timer.o: $(UTILS_SRC_DIR)/Timer.cpp
	@echo "$(YELLOW)Compiling Timer.cpp...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_timer.o: test_timer.cpp
	@echo "$(YELLOW)Compiling test_timer.cpp...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# RTSP Message test
$(TEST_RTSP): $(RTSP_OBJECTS)
	@echo "$(YELLOW)Linking test_rtsp_message...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✓ test_rtsp_message built$(NC)"

$(BUILD_DIR)/RTSPMessage.o: $(NETWORK_SRC_DIR)/RTSPMessage.cpp
	@echo "$(YELLOW)Compiling RTSPMessage.cpp...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_rtsp_message.o: test_rtsp_message.cpp
	@echo "$(YELLOW)Compiling test_rtsp_message.cpp...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ==================== Run Targets ====================

run_timer: $(TEST_TIMER)
	@echo ""
	@echo "$(YELLOW)Running Timer tests...$(NC)"
	@./$(TEST_TIMER)

run_rtsp: $(TEST_RTSP)
	@echo ""
	@echo "$(YELLOW)Running RTSP Message tests...$(NC)"
	@./$(TEST_RTSP)

test_timer: $(TEST_TIMER) run_timer

test_rtsp: $(TEST_RTSP) run_rtsp

# ==================== Clean ====================

clean:
	@echo "$(YELLOW)Cleaning build directory...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

# ==================== Help ====================

help:
	@echo "Common Library Test Suite"
	@echo ""
	@echo "Available targets:"
	@echo "  make all          - Build all tests"
	@echo "  make test         - Build and run all tests"
	@echo "  make test_timer   - Build and run timer test only"
	@echo "  make test_rtsp    - Build and run RTSP message test only"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Individual test executables:"
	@echo "  $(TEST_TIMER)"
	@echo "  $(TEST_RTSP)"

# ... existing content ...

# Add VideoStream and EncodingStrategy sources
VIDEO_STREAM_SRC = ../../server/src/video/VideoStream.cpp
ENCODING_STRATEGY_SRC = ../../server/src/rtp/EncodingStrategy.cpp

# New test target
TEST_SERVER_SIM = test_server_simulation

all: $(BUILD_DIR) $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM)

# Test Server Simulation
$(TEST_SERVER_SIM): $(BUILD_DIR)/test_server_simulation.o $(BUILD_DIR)/FrameBuffer.o \
                    $(BUILD_DIR)/RTPPacket.o $(BUILD_DIR)/FrameReassembler.o \
                    $(BUILD_DIR)/Logger.o $(BUILD_DIR)/VideoStream.o \
                    $(BUILD_DIR)/EncodingStrategy.o
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_server_simulation.o: test_server_simulation.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/VideoStream.o: $(VIDEO_STREAM_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/EncodingStrategy.o: $(ENCODING_STRATEGY_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TEST_RTP_PIPELINE) $(TEST_SERVER_SIM)