workspace "gb_emulator"
	configurations { "Debug", "Debugopt", "Release" }
	system "Windows"
    architecture "x86_64"
	language "C++"
	cppdialect "C++20"
	location "build"
	startproject "gb_emulator"
	staticruntime "on"
	flags "MultiProcessorCompile"
	floatingpoint "fast"
	flags {"NoRuntimeChecks", "NoBufferSecurityCheck"}
	
filter "Debug"
	runtime "Debug"
	symbols "on"
	optimize "off"
	targetdir "build/bin/debug"
	debugdir "build/bin/debug"
	defines {"RENDERER_DEBUG", "EMULATOR_DEBUG"}

filter "Debugopt"
	runtime "Debug"
	symbols "on"
	optimize "debug"
	targetdir "build/bin/debugopt"
	debugdir "build/bin/debugopt"
	defines {"RENDERER_DEBUG", "EMULATOR_DEBUGOPT"}

filter "Release"
	runtime "Release"
	symbols "off"
	optimize "speed"
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
	libdirs {"external/SDL2-2.30.3/lib/x64/", "external/"}
	links { "dx12_renderer", "SDL2" }
	defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN" }
	includedirs { 
		"src",
		"external/dx12_renderer/src",
		"external/dx12_renderer/external/dx12_agility_sdk/build/native/include",
		"external/SDL2-2.30.3/include",
	}
	prebuildcommands {
		"{COPYDIR} " .. _WORKING_DIR .. "/shader %{cfg.buildtarget.directory}shader",
		"{COPYFILE} " .. _WORKING_DIR .. "/external/SDL2-2.30.3/lib/x64/SDL2.dll %{cfg.buildtarget.directory}",
	}

