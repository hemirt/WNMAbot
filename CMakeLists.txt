cmake_minimum_required(VERSION 3.5.0)
project(WNMAbot CXX)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.60.0 REQUIRED COMPONENTS system regex thread)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Hiredis 0.13.3)
include_directories(${HIREDIS_INCLUDE_DIR})
find_package(CURL)
include_directories(${CURL_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("debug")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread -std=c++1z -O1 -g -fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
else()
    message("release")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread -std=c++1z -O3 -flto ${CMAKE_CXX_FLAGS}")
endif()

set(WNMAbot
    ${WNMAbot}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/connectionhandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commandshandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisauth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/remindusers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mtrandom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/afkers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/comebackmsg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/userids.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/countries.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/messenger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pingme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ayah.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bible.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encryption.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/randomquote.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ignore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lotto.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lotto/lottoimpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hemirt.cpp
)

add_subdirectory(mariadb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mariadb/include)

add_executable(WNMAbot ${WNMAbot})
target_link_libraries(WNMAbot ${Boost_LIBRARIES} ${HIREDIS_LIBRARIES} ${CURL_LIBRARIES} database)