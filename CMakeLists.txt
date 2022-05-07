cmake_minimum_required(VERSION 3.21)
project(BigDecimal)

set(CMAKE_CXX_STANDARD 20)

add_executable(BigDecimal
        BigDecimal.cpp
        BigDecimal.h
        doctest.h tests.cpp)

include(CTest)
include(doctest.cmake)
doctest_discover_tests(BigDecimal)