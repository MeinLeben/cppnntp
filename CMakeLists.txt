cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cppnntp)

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

find_package(OpenSSL)

find_package(Boost 1.87 REQUIRED COMPONENTS system iostreams)

set(SOURCE
	src/boostRegexExceptions.cpp
	src/nntp.cpp
	src/socket.cpp
	src/yencdecode.cpp
)

add_library(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system Boost::iostreams ${OPENSSL_LIBRARIES})

add_subdirectory(test)