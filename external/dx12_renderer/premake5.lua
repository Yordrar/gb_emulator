project "dx12_renderer"
	kind "StaticLib"
	pchheader "stdafx.h"
	pchsource "src/stdafx.cpp"
	forceincludes "stdafx.h"
	files {
		"src/**.h",
		"src/**.cpp",
	}
	removefiles {
		"src/imgui/**",
	}
	files {
		"src/imgui/*.cpp",
		"src/imgui/backends/imgui_impl_win32.cpp",
		"src/imgui/backends/imgui_impl_dx12.cpp",
	}
	libdirs { "external/dxc/lib/x64", "external/WinPixEventRuntime/bin/x64" }
	links { "d3d12", "dxgi", "dxguid", "dxcompiler", "WinPixEventRuntime" }
	defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN" }
	includedirs {
		"src",
		"src/imgui",
		"external",
		"external/dx12_agility_sdk/build/native/include",
		"external/dxc/inc",
		"external/WinPixEventRuntime/Include/WinPixEventRuntime",
		"external/imgui",
		"external/imgui/backends",
	}
	prebuildcommands {
		"{MKDIR} %{cfg.buildtarget.directory}/D3D12/",
		"{COPYFILE} " .. _WORKING_DIR .. "/external/dx12_renderer/external/dx12_agility_sdk/build/native/bin/x64/D3D12Core.dll %{cfg.buildtarget.directory}/D3D12/",
		"{COPYFILE} " .. _WORKING_DIR .. "/external/dx12_renderer/external/dx12_agility_sdk/build/native/bin/x64/d3d12SDKLayers.dll %{cfg.buildtarget.directory}/D3D12/",
		"{COPYFILE} " .. _WORKING_DIR .. "/external/dx12_renderer/external/dxc/bin/x64/dxcompiler.dll %{cfg.buildtarget.directory}",
		"{COPYFILE} " .. _WORKING_DIR .. "/external/dx12_renderer/external/dxc/bin/x64/dxil.dll %{cfg.buildtarget.directory}",
		"{COPYFILE} " .. _WORKING_DIR .. "/external/dx12_renderer/external/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll %{cfg.buildtarget.directory}",
		"{COPYDIR} " .. _WORKING_DIR .. "/external/dx12_renderer/shader %{cfg.buildtarget.directory}shader",
	}
