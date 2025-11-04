#* Create a static library named 'common_lib'
add_library(common_lib STATIC src/RtpPacket.h)

target_include_directories(common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)