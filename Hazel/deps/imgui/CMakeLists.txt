cmake_minimum_required(VERSION 3.10)

set(SRCS
    "imgui/imgui.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imgui_demo.cpp"
)

add_library(imgui STATIC "${SRCS}")
target_include_directories(imgui PUBLIC "imgui")
