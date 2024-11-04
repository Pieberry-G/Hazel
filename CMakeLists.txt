cmake_minimum_required(VERSION 3.10.0)

project(Hazel3D)

set(CMAKE_CXX_STANDARD 17)

### Configure output locations
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include subdirectories
add_subdirectory(Hazel)
add_subdirectory(Hazelnut)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Hazelnut)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/config/imgui.ini" DESTINATION "${CMAKE_BINARY_DIR}/Hazelnut")
