cmake_minimum_required(VERSION 3.24)
project(DynamicWallpaper)

set(CMAKE_CXX_STANDARD 17)

find_package(FFMPEG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)



add_executable(${PROJECT_NAME} CodeCvt.h main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
target_link_libraries(${PROJECT_NAME} fmt::fmt)
target_link_libraries(${PROJECT_NAME}
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)