cmake_minimum_required(VERSION 3.10)
project(MrEfficient)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(mr_efficient
    main.cpp
    Graph.cpp
    Utils.cpp
)

# Link math library (for sqrt, sin, cos, etc.)
target_link_libraries(mr_efficient m)