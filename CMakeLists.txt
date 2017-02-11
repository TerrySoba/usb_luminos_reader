cmake_minimum_required(VERSION 3.1)
project(usb_luminos_reader)

set (CMAKE_CXX_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
find_package(HIDAPI REQUIRED)

INCLUDE_DIRECTORIES(
    ${HIDAPI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR})

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${HIDAPI_LIBRARIES})
