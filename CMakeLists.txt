cmake_minimum_required(VERSION 3.24)
project(DynamicWallpaper)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/sdk/include)
include_directories(${PROJECT_SOURCE_DIR}/libevent/dll/x64-release/include)

add_executable(DynamicWallpaper main.cpp CodeCvt.h)

target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/sdk/lib/libvlc.lib)
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/sdk/lib/libvlccore.lib)
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/libevent/dll/x64-release/lib/event.lib)
target_link_libraries(${PROJECT_NAME} "ws2_32.lib")