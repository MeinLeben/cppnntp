cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME test)

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

find_package(Boost 1.87 REQUIRED COMPONENTS regex)

add_executable(${PROJECT_NAME} test_nntp.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ../include)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::regex cppnntp)
