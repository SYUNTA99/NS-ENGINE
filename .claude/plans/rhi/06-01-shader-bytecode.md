# 06-01: シェーダーバイトコード

## 目的

コンパイル済みシェーダーバイトコードの管理構造を定義する。

## 参照ドキュメント

- 01-06-enums-shader.md (EShaderFrequency)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIShader.h` (部分

## TODO

### 1. シェーダーバイトコード構造体

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// シェーダーバイトコード
    /// コンパイル済みシェーダープログラムを保持
    struct RHI_API RHIShaderBytecode
    {
        /// バイトコードデータ
        const void* data = nullptr;

        /// バイトコードサイズ（バイト）
        MemorySize size = 0;

        /// 有効か
        bool IsValid() const { return data != nullptr && size > 0; }

        /// 空のバイトコード
        static RHIShaderBytecode Empty() { return RHIShaderBytecode{}; }

        /// データから作成
        static RHIShaderBytecode FromData(const void* d, MemorySize s) {
            RHIShaderBytecode bc;
            bc.data = d;
            bc.size = s;
            return bc;
        }

        /// 配列から作成
        template<size_t N>
        static RHIShaderBytecode FromArray(const uint8 (&arr)[N]) {
            return FromData(arr, N);
        }

        /// コンテンツから作成
        template<typename Container>
        static RHIShaderBytecode FromContainer(const Container& c) {
            return FromData(c.data(), c.size());
        }
    };
}
```

- [ ] RHIShaderBytecode 構造体
- [ ] FromData / FromArray / FromContainer

### 2. シェーダーコンパイル結果

```cpp
namespace NS::RHI
{
    /// シェーダーコンパイルエラー情報
    struct RHI_API RHIShaderCompileError
    {
        /// エラーメッセージ
        std::string message;

        /// ファイル名（該当する場合）
        std::string filename;

        /// 行番号（0 = 不明）
        uint32 line = 0;

        /// 列番号（0 = 不明）
        uint32 column = 0;

        /// 警告か（falseならエラー）。
        bool isWarning = false;
    };

    /// シェーダーコンパイル結果
    struct RHI_API RHIShaderCompileResult
    {
        /// コンパイル成功か
        bool success = false;

        /// バイトコード（成功時）。
        std::vector<uint8> bytecode;

        /// エラー/警告リスト
        std::vector<RHIShaderCompileError> errors;

        /// コンパイル時間（ミリ秒）
        float compileTimeMs = 0.0f;

        /// バイトコード取得
        RHIShaderBytecode GetBytecode() const {
            return success ? RHIShaderBytecode::FromContainer(bytecode)
                           : RHIShaderBytecode::Empty();
        }

        /// エラーメッセージ取得（連結）
        std::string GetErrorString() const {
            std::string result;
            for (const auto& err : errors) {
                if (!err.isWarning) {
                    if (!result.empty()) result += "\n";
                    result += err.message;
                }
            }
            return result;
        }

        /// 警告があるか
        bool HasWarnings() const {
            for (const auto& err : errors) {
                if (err.isWarning) return true;
            }
            return false;
        }
    };
}
```

- [ ] RHIShaderCompileError 構造体
- [ ] RHIShaderCompileResult 構造体

### 3. シェーダーモデル

```cpp
namespace NS::RHI
{
    /// シェーダーモデル
    struct RHI_API RHIShaderModel
    {
        /// メジャーバージョン
        uint8 major = 6;

        /// マイナのバージョン
        uint8 minor = 0;

        constexpr RHIShaderModel() = default;
        constexpr RHIShaderModel(uint8 maj, uint8 min) : major(maj), minor(min) {}

        /// 比較
        constexpr bool operator>=(const RHIShaderModel& other) const {
            return major > other.major || (major == other.major && minor >= other.minor);
        }

        constexpr bool operator<(const RHIShaderModel& other) const {
            return !(*this >= other);
        }

        constexpr bool operator==(const RHIShaderModel& other) const {
            return major == other.major && minor == other.minor;
        }

        /// 文字列の表現取得
        const char* ToString() const;

        /// 定義済みシェーダーモデル
        static constexpr RHIShaderModel SM_5_0() { return {5, 0}; }
        static constexpr RHIShaderModel SM_5_1() { return {5, 1}; }
        static constexpr RHIShaderModel SM_6_0() { return {6, 0}; }
        static constexpr RHIShaderModel SM_6_1() { return {6, 1}; }
        static constexpr RHIShaderModel SM_6_2() { return {6, 2}; }
        static constexpr RHIShaderModel SM_6_3() { return {6, 3}; }  // DXR 1.0
        static constexpr RHIShaderModel SM_6_4() { return {6, 4}; }
        static constexpr RHIShaderModel SM_6_5() { return {6, 5}; }  // DXR 1.1
        static constexpr RHIShaderModel SM_6_6() { return {6, 6}; }
        static constexpr RHIShaderModel SM_6_7() { return {6, 7}; }
    };

    /// シェーダーステージごとのターゲット名取得
    /// @param[out] outBuffer 結果格納先（16バイト以上）
    /// @param bufferSize outBufferのサイズ
    /// @return outBuffer
    inline const char* GetShaderTargetName(
        EShaderFrequency frequency, RHIShaderModel model,
        char* outBuffer, uint32 bufferSize)
    {
        static const char* prefixes[] = {
            "vs", // Vertex
            "ps", // Pixel
            "gs", // Geometry
            "hs", // Hull
            "ds", // Domain
            "cs", // Compute
            "lib", // RayTracing Library
            nullptr, nullptr, nullptr, nullptr // RT shader types (use lib)
        };

        uint32 index = static_cast<uint32>(frequency);
        if (index >= sizeof(prefixes) / sizeof(prefixes[0]) || !prefixes[index]) {
            snprintf(outBuffer, bufferSize, "lib_%u_%u", model.major, model.minor);
        } else {
            snprintf(outBuffer, bufferSize, "%s_%u_%u", prefixes[index], model.major, model.minor);
        }
        return outBuffer;
    }
}
```

- [ ] RHIShaderModel 構造体
- [ ] 定義済みシェーダーモデル
- [ ] GetShaderTargetName

### 4. シェーダーコンパイルオプション

```cpp
namespace NS::RHI
{
    /// シェーダーコンパイル最適化レベル
    enum class ERHIShaderOptimization : uint8
    {
        None,       // 最適化なし（デバッグ用）。
        Level1,     // 軽量最適化
        Level2,     // バランス（デフォルト）
        Level3,     // 最大最適化
    };

    /// シェーダーコンパイルオプション
    struct RHI_API RHIShaderCompileOptions
    {
        /// シェーダーモデル
        RHIShaderModel shaderModel = RHIShaderModel::SM_6_0();

        /// 最適化レベル
        ERHIShaderOptimization optimization = ERHIShaderOptimization::Level2;

        /// デバッグ情報を含める
        bool includeDebugInfo = false;

        /// 警告をエラーとして扱うか
        bool warningsAsErrors = false;

        /// マトリクス行優先レイアウト
        bool rowMajorMatrices = true;

        /// 厳格モード（HLSLのみ）。
        bool strictMode = false;

        /// IEEE厳格モード
        bool ieeeStrictness = false;

        /// 16ビット型を有効化
        bool enable16BitTypes = false;

        /// プリプロセット定義
        std::vector<std::pair<std::string, std::string>> defines;

        /// インクルードパス
        std::vector<std::string> includePaths;

        /// 定義追加
        RHIShaderCompileOptions& Define(const char* name, const char* value = "1") {
            defines.emplace_back(name, value);
            return *this;
        }

        /// インクルードパス追加
        RHIShaderCompileOptions& AddIncludePath(const char* path) {
            includePaths.emplace_back(path);
            return *this;
        }
    };
}
```

- [ ] ERHIShaderOptimization 列挙型
- [ ] RHIShaderCompileOptions 構造体

### 5. シェーダーハッシュ

```cpp
namespace NS::RHI
{
    /// シェーダーハッシュ
    /// シェーダーの一意識別に使用
    struct RHI_API RHIShaderHash
    {
        uint64 hash[2] = {0, 0};  // 128bit hash

        bool operator==(const RHIShaderHash& other) const {
            return hash[0] == other.hash[0] && hash[1] == other.hash[1];
        }

        bool operator!=(const RHIShaderHash& other) const {
            return !(*this == other);
        }

        bool operator<(const RHIShaderHash& other) const {
            return hash[0] < other.hash[0] ||
                   (hash[0] == other.hash[0] && hash[1] < other.hash[1]);
        }

        bool IsValid() const { return hash[0] != 0 || hash[1] != 0; }

        /// 文字列の表現取得（6進数）。
        std::string ToString() const;

        /// 文字列のからパのス
        static RHIShaderHash FromString(const char* str);

        /// バイトコードからハッシュ計算
        static RHIShaderHash Compute(const void* data, MemorySize size);

        static RHIShaderHash Compute(const RHIShaderBytecode& bytecode) {
            return Compute(bytecode.data, bytecode.size);
        }
    };

    /// RHIShaderHash用ハッシュ関数（std::unordered_map用）。
    struct RHIShaderHashHasher
    {
        size_t operator()(const RHIShaderHash& h) const {
            return static_cast<size_t>(h.hash[0] ^ h.hash[1]);
        }
    };
}
```

- [ ] RHIShaderHash 構造体
- [ ] Compute
- [ ] RHIShaderHashHasher

## 検証方法

- [ ] バイトコード構造体の正常動作
- [ ] コンパイル結果の情報保持
- [ ] シェーダーモデル比較の正確性
- [ ] ハッシュ計算の一貫性
