cmake_minimum_required(VERSION 3.20)
project(lab2_13 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPT_LEVEL "O3" CACHE STRING "Optimization level")
set(CMAKE_CXX_FLAGS_RELEASE "-${OPT_LEVEL} -DNDEBUG")

find_package(TBB REQUIRED)

add_executable(${PROJECT_NAME} main.cpp DataGenerator.cpp ReduceAlgorithms.cpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        OPT_LABEL="${OPT_LEVEL}"
        DATA_DIR="${CMAKE_SOURCE_DIR}/data"
        OUTPUT_DIR="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)
