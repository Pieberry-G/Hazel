add_subdirectory(glad)
add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(imgui)
add_subdirectory(ImGuizmo)
add_subdirectory(spdlog)
add_subdirectory(stb_image)
add_subdirectory(yaml-cpp)

set_target_properties(glad PROPERTIES FOLDER "Dependencies")
set_target_properties(glm PROPERTIES FOLDER "Dependencies")
set_target_properties(imgui PROPERTIES FOLDER "Dependencies")
set_target_properties(ImGuizmo PROPERTIES FOLDER "Dependencies")
set_target_properties(spdlog PROPERTIES FOLDER "Dependencies")
set_target_properties(stb_image PROPERTIES FOLDER "Dependencies")
set_target_properties(yaml-cpp PROPERTIES FOLDER "Dependencies")
set_target_properties(yaml-cpp-parse PROPERTIES FOLDER "Dependencies")
set_target_properties(yaml-cpp-read PROPERTIES FOLDER "Dependencies")
set_target_properties(yaml-cpp-sandbox PROPERTIES FOLDER "Dependencies")
