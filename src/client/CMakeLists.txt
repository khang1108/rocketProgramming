add_executable(client_app
    src/main.cpp
    src/ClientWindow.cpp
    src/RTSPClient.cpp
)

target_include_directories(client_app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party
)

target_link_libraries(client_app PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Network

    common_lib
)