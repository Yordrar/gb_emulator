workspace "gb_emulator"
	configurations { "Debug", "Release" }
	system "Windows"
    architecture "x86_64"
	language "C++"
	cppdialect "C++20"
	location "build"
	startproject "gb_emulator"
	staticruntime "on"
	flags "MultiProcessorCompile"

filter "Debug"
	runtime "Debug"
	symbols "on"
	optimize "off"
	targetdir "build/bin/debug"
	debugdir "build/bin/debug"
	defines "RENDERER_DEBUG"

filter "Release"
	runtime "Release"
	symbols "off"
	optimize "on"
	targetdir "build/bin/release"
	debugdir "build/bin/release"
	flags "LinkTimeOptimization"

include "external/dx12_renderer"

project "gb_emulator"
	kind "WindowedApp"
	files
	{
		"src/**.h",
		"src/**.cpp",
	}
	libdirs "external/"
	links { "dx12_renderer" }
	defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN" }
	includedirs { 
		"src",
		"external/dx12_renderer/src",
		"external/dx12_renderer/external/dx12_agility_sdk/build/native/include",
	}
	prebuildcommands {
		"{COPYDIR} " .. _WORKING_DIR .. "/shader %{cfg.buildtarget.directory}shader",
	}

