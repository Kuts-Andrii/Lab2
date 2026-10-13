cmake_minimum_required(VERSION 3.20)
project(lab2 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(TBB REQUIRED)

add_executable(lab2
        main.cpp
        DataGenerator.cpp
        ReduceAlgorithms.cpp
)

target_link_libraries(lab2 PRIVATE TBB::tbb)