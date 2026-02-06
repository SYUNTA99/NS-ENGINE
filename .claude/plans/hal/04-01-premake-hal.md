# 04-01: premake HALプロジェクト

## 目的

HALモジュールをpremake5.luaに追加する。

## 参考

[Platform_Abstraction_Layer_Part4.md](docs/Platform_Abstraction_Layer_Part4.md) - セクション2「モジュールビルドルール」

## 変更対象

変更:
- `premake5.lua`

## TODO

- [ ] HALプロジェクト定義追加
- [ ] インクルードパス設定
- [ ] プラットフォームマクロ定義（-DPLATFORM_WINDOWS=1等）
- [ ] 依存関係設定

## 実装内容

```lua
-- premake5.lua に追加

project "hal"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "source/engine/hal/Public/**.h",
        "source/engine/hal/Private/**.cpp"
    }

    includedirs {
        "source/engine/hal/Public",
        "source/common"  -- commonモジュールをインクルードパスに追加
    }

    links {
        "common"  -- commonモジュールへの依存
    }

    filter "system:windows"
        defines {
            "PLATFORM_WINDOWS=1",
            "NS_COMPILED_PLATFORM=Windows"
        }

    filter "configurations:Debug"
        defines { "NS_DEBUG=1" }
        symbols "On"

    filter "configurations:Development"
        defines { "NS_DEVELOPMENT=1" }
        symbols "On"
        optimize "On"

    filter "configurations:Release"
        defines { "NS_RELEASE=1" }
        optimize "On"
```

## 注意

- HALはcommonモジュールの型・マクロを基盤として使用する
- `NS_PLATFORM_*`, `NS_ARCH_*`, `NS_COMPILER_*` などはcommonで定義済み
- HALは互換性エイリアス（`PLATFORM_WINDOWS` 等）を提供

## 検証

- `premake5 vs2022` でソリューション生成成功
- HALプロジェクトがビルド可能
- インクルードパスが正しく設定される

## 必要なリンクライブラリ（Windows）

```lua
filter "system:windows"
    links {
        "dbghelp",  -- スタックウォーク用
        "ole32"     -- COM用
    }
```

## 次のサブ計画

→ Phase 2に進む（03-01a〜 並列実装可能）
