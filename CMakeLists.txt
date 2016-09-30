cmake_minimum_required(VERSION 3.6)
project(ParallelWebCrawler)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp HttpRequest.cpp HttpRequest.h WebPage.cpp WebPage.h Link.cpp Link.h WebCrawler.cpp WebCrawler.h ThreadPool.cpp ThreadPool.h)
add_executable(ParallelWebCrawler ${SOURCE_FILES})

file(GLOB SEED_FILES "*.txt")
file(COPY ${SEED_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
