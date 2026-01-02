cmake_minimum_required(VERSION 3.20)

project(AutoAny)

set(TARGET_NAME autoany)

file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_executable(${TARGET_NAME} ${SRCS})

target_link_libraries(${TARGET_NAME} gany gx getopt)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER GAny/Tools)
