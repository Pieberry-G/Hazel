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
IncludeDir["GLFW"] = "Hazel/vendor/GLFW/include"
IncludeDir["Glad"] = "Hazel/vendor/Glad/include"
IncludeDir["Imgui"] = "Hazel/vendor/imgui"
IncludeDir["glm"] = "Hazel/vendor/glm"
IncludeDir["stb_image"] = "Hazel/vendor/stb_image"
IncludeDir["entt"] = "Hazel/vendor/entt/include"

group "Dependencies"
    include "Hazel/vendor/GLFW"
    include "Hazel/vendor/Glad"
    include "Hazel/vendor/imgui"
group ""

project "Hazel"
    location "Hazel"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl"
    }

    includedirs
    {
        "Hazel/vendor/spdlog/include",
        "Hazel/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.Imgui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.entt}"
    }

    links
    {
        "GLFW",
        "Glad",
        "Imgui",
        "opengl32.lib"
    }

    pchsource "Hazel/src/hzpch.cpp"
    pchheader "hzpch.h"

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "HZ_PLATFORM_WINDOWS",
            "HZ_ENABLE_ASSERTS",
            "GLFW_INCLUDE_NONE"
        }

    filter "configurations:Debug"
        defines { "HZ_DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "HZ_NDEBUG" }
        runtime "Release"
        optimize "On"
    
    filter "configurations:Dist"
        defines { "HZ_DIST" }
        runtime "Release"
        optimize "On"


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
        "Hazel/vendor/spdlog/include",
        "Hazel/src",
        "%{IncludeDir.Imgui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}"
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