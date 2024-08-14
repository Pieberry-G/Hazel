project "Hazel"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "hzpch.h"
    pchsource "src/hzpch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
        "vendor/stb_image/**.h",
        "vendor/stb_image/**.cpp",
        "vendor/glm/glm/**.hpp",
        "vendor/glm/glm/**.inl",

        "vendor/ImGuizmo/ImGuizmo.h",
        "vendor/ImGuizmo/ImGuizmo.cpp",
    }

    includedirs
    {
        "vendor/spdlog/include",
        "src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.Imgui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.ImGuizmo}"
    }

    links
    {
        "GLFW",
        "Glad",
        "Imgui",
        "yaml-cpp",
        "opengl32.lib"
    }

    filter "files:vendor/ImGuizmo/**.cpp"
        flags{ "NoPCH" }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "_CRT_SECURE_NO_WARNINGS",
            "HZ_PLATFORM_WINDOWS",
            "HZ_ENABLE_ASSERTS",
            "GLFW_INCLUDE_NONE",
            "YAML_CPP_STATIC_DEFINE"
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