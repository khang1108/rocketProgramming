# ==============================================================================
# RTSP/RTP Common Library
# ==============================================================================

# Define the common library
set(COMMON_TARGET common)

# ==============================================================================
# Source Files
# ==============================================================================

# Collect all source files
file(GLOB_RECURSE COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Collect all header files
file(GLOB_RECURSE COMMON_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# ==============================================================================
# Create Library
# ==============================================================================

add_library(${COMMON_TARGET} STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})

# Set target properties
set_target_properties(${COMMON_TARGET} PROPERTIES
    OUTPUT_NAME "common"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(${COMMON_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp
)

target_link_libraries(${COMMON_TARGET}
    PUBLIC
        Threads::Threads
        Qt6::Core
        Qt6::Network
)


# Platform-specific libraries
if(WIN32)
    # Windows requires ws2_32 for socket programming
    target_link_libraries(${COMMON_TARGET}
        PUBLIC
            ws2_32
    )
endif()

# ==============================================================================
# Compile Definitions
# ==============================================================================

# Add platform definitions
if(WIN32)
    target_compile_definitions(${COMMON_TARGET}
        PUBLIC
            _WIN32_WINNT=0x0601  # Windows 7 or later
            WIN32_LEAN_AND_MEAN  # Exclude rarely-used Windows headers
            NOMINMAX             # Prevent min/max macro conflicts
    )
endif()

# Add build type definitions
target_compile_definitions(${COMMON_TARGET}
    PUBLIC
        $<$<CONFIG:Debug>:DEBUG_MODE>
        $<$<CONFIG:Release>:RELEASE_MODE>
)

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS ${COMMON_TARGET}
    ARCHIVE DESTINATION lib
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
)

# ==============================================================================
# Build Info
# ==============================================================================

message(STATUS "Common library: ${COMMON_TARGET}")
message(STATUS "Common sources: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Common headers: ${CMAKE_CURRENT_SOURCE_DIR}/include")

# Add RTP sources to common_lib
file(GLOB_RECURSE COMMON_RTP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rtp/*.cpp"
)
target_sources(common PRIVATE ${COMMON_RTP_SOURCES})