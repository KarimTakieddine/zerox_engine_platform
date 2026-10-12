cmake_minimum_required(VERSION 4.3.1 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine_platform)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb")

Project(${PROJECT_NAME})

if (WIN32)
    list(APPEND SOURCES ${SRC_DIR}/platform_win32.cpp)
elseif(LINUX)
    list(APPEND SOURCES ${SRC_DIR}/platform_arch_linux.cpp)
endif()

list(APPEND SOURCES ${SRC_DIR}/platform.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR} ${STB_DIR})

if (WIN32)
elseif(LINUX)
    target_link_libraries(${PROJECT_NAME} PUBLIC uring)
endif()
