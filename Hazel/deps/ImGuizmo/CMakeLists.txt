cmake_minimum_required(VERSION 3.10)

set(SRCS "ImGuizmo/ImGuizmo.cpp")

add_library(ImGuizmo STATIC "${SRCS}")
target_include_directories(ImGuizmo PUBLIC "ImGuizmo")

target_link_libraries(ImGuizmo imgui)
