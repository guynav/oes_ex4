# cmake_minimum_required(VERSION <specify CMake version here>)
project(oes_ex4)

set(CMAKE_CXX_STANDARD 11)

add_executable(oes_ex4
        MemoryConstants.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        SimpleTest.cpp
        VirtualMemory.cpp
        VirtualMemory.h ourTester.cpp)