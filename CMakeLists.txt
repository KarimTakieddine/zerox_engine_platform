cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine_platform)
set(INCLUDE_DIR include)
set(SRC_DIR src)

Project(${PROJECT_NAME})

if (WIN32)
elseif(LINUX)
    list(APPEND SOURCES
        ${SRC_DIR}/platform_arch_linux.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})
