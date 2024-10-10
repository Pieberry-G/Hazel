cmake_minimum_required(VERSION 3.10.0)

file(GLOB_RECURSE SRCS "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_executable(Hazelnut "${SRCS}" "${HEADERS}")
target_include_directories(Hazelnut PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(Hazelnut PRIVATE HZ_PLATFORM_WINDOWS)
target_compile_definitions(Hazelnut PRIVATE "$<$<CONFIG:DEBUG>:HZ_DEBUG>")
target_compile_definitions(Hazelnut PRIVATE "$<$<CONFIG:RELEASE>:HZ_NDEBUG>")

target_link_libraries(Hazelnut Hazel)
