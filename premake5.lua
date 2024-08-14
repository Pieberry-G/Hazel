workspace "Hazel"
    architecture "x64"
    startproject "Hazelnut"
    
    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder {solution directory}
IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Hazel/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Hazel/vendor/Glad/include"
IncludeDir["Imgui"] = "%{wks.location}/Hazel/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/Hazel/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Hazel/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/Hazel/vendor/entt/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Hazel/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Hazel/vendor/ImGuizmo"

group "Dependencies"
    include "Hazel/vendor/GLFW"
    include "Hazel/vendor/Glad"
    include "Hazel/vendor/imgui"
    include "Hazel/vendor/yaml-cpp"
group ""

group "Core"
	include "Hazel"
group ""

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
        "Hazel/vendor/spdlog/include",
        "Hazel/src",
        "%{IncludeDir.Imgui}",
        "%{IncludeDir.glm}"
    }
    
    links
    {
        "Hazel"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "HZ_PLATFORM_WINDOWS",
        }

    filter "configurations:Debug"
        defines { "HZ_DEBUG" }
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines { "HZ_NDEBUG" }
        runtime "Release"
        optimize "on"
    
        filter "configurations:Dist"
        defines { "HZ_DIST" }
        runtime "Release"
        optimize "on"

        
project "Hazelnut"
    location "Hazelnut"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
		"%{wks.location}/Hazel/vendor/spdlog/include",
		"%{wks.location}/Hazel/src",
        "%{IncludeDir.Imgui}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glm}",
        "%{IncludeDir.ImGuizmo}"
    }

    links
    {
        "Hazel"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "HZ_PLATFORM_WINDOWS",
        }

    filter "configurations:Debug"
        defines { "HZ_DEBUG" }
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines { "HZ_NDEBUG" }
        runtime "Release"
        optimize "on"
    
        filter "configurations:Dist"
        defines { "HZ_DIST" }
        runtime "Release"
        optimize "on"