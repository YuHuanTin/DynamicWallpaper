cmake_minimum_required(VERSION 3.24)
project(DynamicWallpaper)

set(CMAKE_CXX_STANDARD 17)


include_directories(${PROJECT_SOURCE_DIR}/sdk/include)

add_executable(DynamicWallpaper CodeCvt.h main.cpp)

target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/sdk/lib/libvlc.lib)
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/sdk/lib/libvlccore.lib)
