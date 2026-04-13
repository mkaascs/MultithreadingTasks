cmake_minimum_required(VERSION 3.28)
project(expr)

set(CMAKE_CXX_STANDARD 17)

add_executable(expr main.cpp
        query/query.h
)
