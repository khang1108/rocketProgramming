add_executable(server_app
    src/main.cpp
    src/Server.cpp
    src/ClientSession.cpp
    src/VideoStream.cpp
    src/ServerWindow.cpp
)

target_include_directories(server_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_app PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core

    common_lib
)

if(WIN32)
    target_link_libraries(client_app PRIVATE ws2_32)
endif()