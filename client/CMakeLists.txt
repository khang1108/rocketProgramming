# client/CMakeLists.txt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Core
    Network
    Multimedia
    MultimediaWidgets
)
# ==============================================================================
# Client Sources
# ==============================================================================

set(CLIENT_SOURCES
    src/client.cpp
    src/ui/ClientUI.cpp
    src/ui/FrameDisplay.cpp
    src/network/RTSPClient.cpp
    src/rtp/RTPReceiver.cpp
    src/rtp/FrameReassembler.cpp
    src/buffer/FrameBuffer.cpp
    src/buffer/BufferSlider.cpp
)

set(CLIENT_HEADERS
    ${CMAKE_SOURCE_DIR}/client/include/ui/ClientUI.hpp
    ${CMAKE_SOURCE_DIR}/client/include/ui/FrameDisplay.hpp
    ${CMAKE_SOURCE_DIR}/client/include/buffer/BufferSlider.hpp
)

# ==============================================================================
# Client Executable
# ==============================================================================

add_executable(rtsp_client 
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}  # ✅ Include headers for AUTOMOC
)

set_target_properties(rtsp_client PROPERTIES
    AUTOMOC ON  
    OUTPUT_NAME "client"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(rtsp_client PRIVATE
    ${CMAKE_SOURCE_DIR}/client/include
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_BINARY_DIR}/client/rtsp_client_autogen/include
)

# ==============================================================================
# Link Libraries - ✅ THÊM Qt6::Widgets
# ==============================================================================

target_link_libraries(rtsp_client 
    PRIVATE
    common                    # Common library
    Qt6::Core                 # Qt Core
    Qt6::Widgets              # ✅ THÊM DÒNG NÀY - Qt Widgets (QWidget, QPushButton, QLabel)
    Qt6::Network              # Qt Network
    Qt6::Multimedia           # Qt Multimedia
    Threads::Threads
    Qt6::MultimediaWidgets
)

# ==============================================================================
# Install
# ==============================================================================

install(TARGETS rtsp_client DESTINATION bin)