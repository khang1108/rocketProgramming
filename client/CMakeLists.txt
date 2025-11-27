# ==============================================================================
# RTSP/RTP Client Executable
# ==============================================================================

# Define the client executable
set(CLIENT_TARGET rtsp_client)

# ==============================================================================
# Source Files
# ==============================================================================

# Collect all source files
file(GLOB_RECURSE CLIENT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Collect all header files
file(GLOB_RECURSE CLIENT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# ==============================================================================
# Create Executable
# ==============================================================================

add_executable(${CLIENT_TARGET} ${CLIENT_SOURCES} ${CLIENT_HEADERS})

# Set target properties
set_target_properties(${CLIENT_TARGET} PROPERTIES
    OUTPUT_NAME "client"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(${CLIENT_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/buffer
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/patterns
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/state
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
)

# ==============================================================================
# Link Libraries
# ==============================================================================

target_link_libraries(${CLIENT_TARGET}
    PRIVATE
        common_lib
        Threads::Threads
)

# ==============================================================================
# Compile Definitions
# ==============================================================================

# Add compile definitions based on build type
target_compile_definitions(${CLIENT_TARGET}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG_MODE>
        $<$<CONFIG:Release>:RELEASE_MODE>
)

# ==============================================================================
# Post-build Commands
# ==============================================================================

# Copy config files to build directory if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    add_custom_command(TARGET ${CLIENT_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${CLIENT_TARGET}>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/config"
            "$<TARGET_FILE_DIR:${CLIENT_TARGET}>/config"
        COMMENT "Copying client configuration files..."
    )
endif()

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS ${CLIENT_TARGET}
    RUNTIME DESTINATION bin
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/"
        DESTINATION bin/config
    )
endif()

# ==============================================================================
# Build Info
# ==============================================================================

message(STATUS "Client executable: ${CLIENT_TARGET}")
message(STATUS "Client sources: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Client headers: ${CMAKE_CURRENT_SOURCE_DIR}/include")
