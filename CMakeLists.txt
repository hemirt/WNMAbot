cmake_minimum_required(VERSION 3.5.0)
project(WNMAbot CXX)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.45.0 REQUIRED COMPONENTS system regex)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "-pthread -std=c++14 -lhiredis ${CMAKE_CXX_FLAGS}")

set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/connectionhandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commandshandler.cpp
)

add_executable(WNMAbot ${SOURCE})
target_link_libraries(WNMAbot ${Boost_LIBRARIES})
