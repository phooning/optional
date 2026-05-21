cmake_minimum_required(VERSION 3.16)
project(optional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(hello-imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

hello_imgui_add_app(optional 
    src/main.cpp
    src/panels.cpp
    src/options/option_math.cpp
    src/options/options_calculator.cpp
    src/options/payoff_plot.cpp
    src/options/sidebar.cpp
    src/options/strategy_eval.cpp
    src/options/strategy_registry.cpp
    src/options/strategy_types.cpp
    src/options/strategy_ui.cpp
    src/window.cpp
)

target_link_libraries(optional PRIVATE implot::implot)
