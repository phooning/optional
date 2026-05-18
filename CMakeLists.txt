cmake_minimum_required(VERSION 3.16)
project(optional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(hello-imgui CONFIG REQUIRED)

hello_imgui_add_app(optional src/main.cpp)

# find_package(glfw3 CONFIG REQUIRED)
