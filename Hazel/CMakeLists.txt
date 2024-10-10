cmake_minimum_required(VERSION 3.10.0)

add_subdirectory(deps)

file(GLOB_RECURSE SRCS "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_library(Hazel STATIC "${SRCS}" "${HEADERS}")
target_precompile_headers(Hazel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/hzpch.h")
target_include_directories(Hazel PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(Hazel PRIVATE HZ_PLATFORM_WINDOWS)
target_compile_definitions(Hazel PRIVATE "$<$<CONFIG:DEBUG>:HZ_DEBUG>")
target_compile_definitions(Hazel PRIVATE "$<$<CONFIG:RELEASE>:HZ_NDEBUG>")

target_link_libraries(Hazel glad glfw glm imgui ImGuizmo spdlog stb_image  yaml-cpp)
target_include_directories(Hazel PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/entt/include")
