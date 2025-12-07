# ==============================================================================
# RTSP/RTP Server Executable
# ==============================================================================

# Define the server executable
set(SERVER_TARGET rtsp_server)

# ==============================================================================
# Source Files
# ==============================================================================

# Collect all source files
file(GLOB_RECURSE SERVER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Collect all header files
file(GLOB_RECURSE SERVER_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# ==============================================================================
# Create Executable
# ==============================================================================

add_executable(${SERVER_TARGET} ${SERVER_SOURCES} ${SERVER_HEADERS})

# Set target properties
set_target_properties(${SERVER_TARGET} PROPERTIES
    OUTPUT_NAME "server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(${SERVER_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/patterns
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/video
)

# ==============================================================================
# Link Libraries
# ==============================================================================

target_link_libraries(${SERVER_TARGET}
    PRIVATE
        common
        Threads::Threads
        Qt6::Core
        Qt6::Network
)

# ==============================================================================
# Compile Definitions
# ==============================================================================

# Add compile definitions based on build type
target_compile_definitions(${SERVER_TARGET}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG_MODE>
        $<$<CONFIG:Release>:RELEASE_MODE>
)

# ==============================================================================
# Post-build Commands
# ==============================================================================

# Copy config files to build directory if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    add_custom_command(TARGET ${SERVER_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${SERVER_TARGET}>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/config"
            "$<TARGET_FILE_DIR:${SERVER_TARGET}>/config"
        COMMENT "Copying server configuration files..."
    )
endif()

# Copy video files to build directory if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/videos")
    add_custom_command(TARGET ${SERVER_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${SERVER_TARGET}>/videos"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/videos"
            "$<TARGET_FILE_DIR:${SERVER_TARGET}>/videos"
        COMMENT "Copying video files..."
    )
endif()

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS ${SERVER_TARGET}
    RUNTIME DESTINATION bin
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/"
        DESTINATION bin/config
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/videos")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/videos/"
        DESTINATION bin/videos
    )
endif()

# ==============================================================================
# Build Info
# ==============================================================================

message(STATUS "Server executable: ${SERVER_TARGET}")
message(STATUS "Server sources: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Server headers: ${CMAKE_CURRENT_SOURCE_DIR}/include")
