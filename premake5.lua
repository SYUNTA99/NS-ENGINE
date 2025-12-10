--============================================================================
-- premake5.lua
-- プロジェクト構成ファイル
--============================================================================

-- ワークスペース設定
workspace "HEW2026"
    configurations { "Debug", "Release" }
    platforms { "x64" }
    location "build"

    -- C++20
    language "C++"
    cppdialect "C++20"

    -- 文字セット
    characterset "Unicode"

    -- 共通設定
    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
        symbols "On"
        optimize "Off"
        runtime "Debug"

    filter "configurations:Release"
        defines { "NDEBUG" }
        symbols "Off"
        optimize "Full"
        runtime "Release"

    filter "platforms:x64"
        architecture "x64"

    filter {}

-- 出力ディレクトリ (build/配下に統一)
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
bindir = "build/bin/" .. outputdir
objdir_base = "build/obj/" .. outputdir

--============================================================================
-- 外部ライブラリ
--============================================================================
group "External"

project "DirectXTex"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    location "build/DirectXTex"

    targetdir (bindir .. "/%{prj.name}")
    objdir (objdir_base .. "/%{prj.name}")

    files {
        "external/DirectXTex/DirectXTex/*.h",
        "external/DirectXTex/DirectXTex/*.cpp"
    }

    removefiles {
        -- Xbox specific
        "external/DirectXTex/DirectXTex/*_xbox*",
        "external/DirectXTex/DirectXTex/BC*Xbox*",
        -- D3D12 (requires d3dx12.h)
        "external/DirectXTex/DirectXTex/DirectXTexD3D12.cpp",
        -- GPU compute (requires pre-compiled shader headers)
        "external/DirectXTex/DirectXTex/BCDirectCompute.cpp",
        "external/DirectXTex/DirectXTex/DirectXTexCompressGPU.cpp"
    }

    includedirs {
        "external/DirectXTex/DirectXTex"
    }

    defines {
        "_WIN32_WINNT=0x0A00",
        "WIN32_LEAN_AND_MEAN"
    }

    filter "configurations:Debug"
        runtime "Debug"

    filter "configurations:Release"
        runtime "Release"

    filter {}

--============================================================================
-- DirectXTK (外部依存)
--============================================================================
project "DirectXTK"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    location "build/DirectXTK"

    targetdir (bindir .. "/%{prj.name}")
    objdir (objdir_base .. "/%{prj.name}")

    files {
        "external/DirectXTK/Inc/*.h",
        "external/DirectXTK/Src/*.h",
        "external/DirectXTK/Src/*.cpp"
    }

    removefiles {
        -- Xbox specific
        "external/DirectXTK/Src/*Xbox*",
        -- D3D12
        "external/DirectXTK/Src/*12*",
        -- Effects (require compiled shaders)
        "external/DirectXTK/Src/*Effect*.cpp",
        "external/DirectXTK/Src/SpriteBatch.cpp",
        "external/DirectXTK/Src/SpriteFont.cpp",
        "external/DirectXTK/Src/*PostProcess*.cpp",
        "external/DirectXTK/Src/ToneMapPostProcess.cpp",
        "external/DirectXTK/Src/PrimitiveBatch.cpp",
        "external/DirectXTK/Src/GeometricPrimitive.cpp",
        "external/DirectXTK/Src/Model*.cpp",
        "external/DirectXTK/Src/DGSL*.cpp"
    }

    includedirs {
        "external/DirectXTK/Inc",
        "external/DirectXTK/Src"
    }

    defines {
        "_WIN32_WINNT=0x0A00",
        "WIN32_LEAN_AND_MEAN"
    }

    filter "configurations:Debug"
        runtime "Debug"

    filter "configurations:Release"
        runtime "Release"

    filter {}

group ""

--============================================================================
-- dx11ライブラリ
--============================================================================
project "dx11"
    kind "StaticLib"
    location "build/dx11"

    targetdir (bindir .. "/%{prj.name}")
    objdir (objdir_base .. "/%{prj.name}")

    files {
        "source/dx11/**.h",
        "source/dx11/**.cpp",
        "source/dx11/**.inl"
    }

    -- 除外ファイル
    removefiles {
        "source/dx11/compile/shader_reflection.cpp"
    }

    includedirs {
        "source",
        "external/DirectXTex/DirectXTex",
        "external/DirectXTK/Inc"
    }

    links {
        "d3d11",
        "d3dcompiler",
        "dxguid",
        "dxgi",
        "xinput",
        "DirectXTex",
        "DirectXTK"
    }

    defines {
        "_WIN32_WINNT=0x0A00",
        "WIN32_LEAN_AND_MEAN"
    }

    -- 警告設定
    warnings "Extra"
    flags { "FatalWarnings" }

    buildoptions { "/utf-8", "/permissive-" }

    -- リンカー警告を無視 (Windows SDK重複定義)
    linkoptions { "/ignore:4006" }

--============================================================================
-- エンジンライブラリ
--============================================================================
project "engine"
    kind "StaticLib"
    location "build/engine"

    targetdir (bindir .. "/%{prj.name}")
    objdir (objdir_base .. "/%{prj.name}")

    files {
        "source/engine/**.h",
        "source/engine/**.cpp"
    }

    includedirs {
        "source",
        "source/engine",
        "external/DirectXTK/Inc"
    }

    links {
        "dx11",
        "DirectXTK"
    }

    defines {
        "_WIN32_WINNT=0x0A00"
    }

    warnings "Extra"
    flags { "FatalWarnings" }

    buildoptions { "/utf-8", "/permissive-" }

--============================================================================
-- ゲーム実行ファイル
--============================================================================
project "game"
    kind "WindowedApp"
    location "build/game"

    targetdir (bindir .. "/%{prj.name}")
    objdir (objdir_base .. "/%{prj.name}")

    files {
        "source/game/**.h",
        "source/game/**.cpp"
    }

    includedirs {
        "source",
        "source/engine",
        "external/DirectXTK/Inc"
    }

    links {
        "engine",
        "dx11",
        "DirectXTex",
        "DirectXTK",
        "d3d11",
        "d3dcompiler",
        "dxguid",
        "dxgi",
        "xinput"
    }

    defines {
        "_WIN32_WINNT=0x0A00"
    }

    -- 作業ディレクトリ
    debugdir "."

    warnings "Extra"
    buildoptions { "/utf-8", "/permissive-" }

--============================================================================
-- テスト実行ファイル
--============================================================================
project "tests"
    kind "ConsoleApp"
    location "build/tests"

    targetdir (bindir .. "/%{prj.name}")
    objdir (objdir_base .. "/%{prj.name}")

    files {
        "tests/**.h",
        "tests/**.cpp"
    }

    removefiles {
        "tests/generate_test_textures.cpp"
    }

    includedirs {
        "source",
        "tests"
    }

    links {
        "engine",
        "dx11",
        "DirectXTex",
        "DirectXTK",
        "d3d11",
        "d3dcompiler",
        "dxguid",
        "dxgi",
        "xinput"
    }

    defines {
        "_WIN32_WINNT=0x0A00"
    }

    debugdir "."

    warnings "Extra"
    buildoptions { "/utf-8", "/permissive-" }
