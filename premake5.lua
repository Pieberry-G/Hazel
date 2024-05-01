workspace "Hazel"
    architecture "x64"
    
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

include "Hazel/vendor/GLFW"
include "Hazel/vendor/Glad"
include "Hazel/vendor/imgui"

project "Hazel"
    location "Hazel"
    kind "SharedLib"
    language "C++"

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
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.Imgui}"
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
        cppdialect "C++17"
        staticruntime "Off"
        systemversion "latest"

        defines
        {
            "HZ_PLATFORM_WINDOWS",
            "HZ_BUILD_DLL",
            "GLFW_INCLUDE_NONE"
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }

    filter "configurations:Debug"
        defines { "HZ_DEBUG" }
        -- buildoptions "/MDd"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "HZ_NDEBUG" }
        -- buildoptions "/MD"
        runtime "Release"
        optimize "On"
    
    filter "configurations:Dist"
        defines { "HZ_DIST" }
        -- buildoptions "/MD"
        runtime "Release"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"

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
        "Hazel/src"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "Off"
        systemversion "latest"

        defines
        {
            "HZ_PLATFORM_WINDOWS",
        }

    links
    {
        "Hazel"
    }

    filter "configurations:Debug"
        defines { "HZ_DEBUG" }
        -- buildoptions "/MDd"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "HZ_NDEBUG" }
        -- buildoptions "/MD"
        runtime "Release"
        optimize "On"
    
        filter "configurations:Dist"
        defines { "HZ_DIST" }
        -- buildoptions "/MD"
        runtime "Release"
        optimize "On"