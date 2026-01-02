cmake_minimum_required(VERSION 3.20)

project(GxDocMake)

############### ToolLib ###############

file(GLOB_RECURSE LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tool_lib/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/tool_lib/src/*.c)
file(GLOB_RECURSE LIB_HDR ${CMAKE_CURRENT_SOURCE_DIR}/tool_lib/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/tool_lib/src/*.h)

add_library(doc-make-lib
        ${LIB_SRC}
        ${LIB_HDR})

target_include_directories(doc-make-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tool_lib/include)
target_link_libraries(doc-make-lib PUBLIC gx)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(doc-make-lib PRIVATE BUILD_SHARED_LIBS=1)
endif ()

set_target_properties(doc-make-lib PROPERTIES FOLDER GAny/Tools)

############### App ###############

set(TARGET_NAME doc-make)

add_executable(${TARGET_NAME} src/main.cpp)

target_link_libraries(${TARGET_NAME} PRIVATE gany getopt doc-make-lib)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER GAny/Tools)
