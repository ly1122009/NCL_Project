# Set sample variable
# SET(EXTERNAL_LIBS FMT wiringPi GMP)

cmake_minimum_required(VERSION 3.22)

# Project settings
project(Template-Project-CPP LANGUAGES CXX)

# Set the C++ standard globally
set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add libraries
add_subdirectory(interface)
add_subdirectory(feature)

# Define the target executable
add_executable(Main_Executable main.cpp)

# Link some target libraries
target_link_libraries(Main_Executable PUBLIC Library OSAL Interface)
