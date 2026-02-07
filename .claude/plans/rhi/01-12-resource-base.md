# 01-12: IRHIResource基底クラス

## 目的

全RHIリソースの基底クラスを定義する。

## 参照ドキュメント

- docs/Graphics_Abstraction_Coding_Patterns.md (リソースライフサイクル)
- docs/RHI/RHI_Implementation_Guide_Part3.md (リソース管理)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIResource.h`

## TODO

### 1. IRHIResource: 参照カウント

```cpp
#pragma once

#include "Common/Utility/Types.h"
#include "Common/STL/STLThreading.h"
#include "RHIMacros.h"
#include "RHIFwd.h"
#include <atomic>
#include <string>

namespace NS::RHI
{
    /// 前方宣言
    enum class ERHIResourceType : uint8;
    class RHIDeferredDeleteQueue;

    /// RHIリソース基底クラス
    /// 全てのRHIリソースはこのクラスを継承する
    class RHI_API IRHIResource
    {
    public:
        IRHIResource();
        virtual ~IRHIResource();

        NS_DISALLOW_COPY(IRHIResource);

    public:
        //=====================================================================
        // 参照カウント
        //=====================================================================

        /// 参照カウント増加
        /// @return 増加後の参照カウント
        /// @note スレッドセーフ
        uint32 AddRef() const noexcept;

        /// 参照カウント減少
        /// @return 減少後の参照カウント（0なら削除された）
        /// @note スレッドセーフ
        uint32 Release() const noexcept;

        /// 現在の参照カウント取得
        uint32 GetRefCount() const noexcept;

    private:
        mutable std::atomic<uint32> m_refCount;
```

- [ ] 参照カウントメンバ変数
- [ ] AddRef / Release メソッド

### 2. IRHIResource: リソース識別

```cpp
    public:
        //=====================================================================
        // リソース識別
        //=====================================================================

        /// リソースタイプ取得（派生クラスでオーバーライド）
        virtual ERHIResourceType GetResourceType() const = 0;

        /// リソースID取得
        ResourceId GetResourceId() const noexcept { return m_resourceId; }

    private:
        ResourceId m_resourceId;
```

- [ ] GetResourceType() 純粋仮想関数
- [ ] リソースID

### 3. IRHIResource: デバッグ名

```cpp
    public:
        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグ名設定
        /// @param name UTF-8文字列
        /// @note スレッドセーフ（m_debugNameMutexで保護）
        virtual void SetDebugName(const char* name) {
            std::lock_guard<std::mutex> lock(m_debugNameMutex);
            m_debugName = name ? name : "";
            // 派生クラスでネイティブAPIのデバッグ名も設定
        }

        /// デバッグ名取得（スレッドセーフ）
        /// @param outBuffer 出力バッファ
        /// @param bufferSize バッファサイズ
        /// @return 書き込まれた文字数
        size_t GetDebugName(char* outBuffer, size_t bufferSize) const {
            std::lock_guard<std::mutex> lock(m_debugNameMutex);
            const size_t len = std::min(m_debugName.size(), bufferSize - 1);
            std::memcpy(outBuffer, m_debugName.c_str(), len);
            outBuffer[len] = '\0';
            return len;
        }

        /// デバッグ名があるか
        bool HasDebugName() const {
            std::lock_guard<std::mutex> lock(m_debugNameMutex);
            return !m_debugName.empty();
        }

    private:
        std::string m_debugName;
        mutable std::mutex m_debugNameMutex;  ///< デバッグ名のスレッドセーフティ保護
```

- [ ] SetDebugName / GetDebugName
- [ ] デバッグ名格納

### 4. IRHIResource: 遅延削除サポート

```cpp
    public:
        //=====================================================================
        // 遅延削除
        //=====================================================================

        /// 遅延削除としてマーク
        /// GPU使用完了まで実際の削除を遅延する
        void MarkForDeferredDelete() const noexcept;

        /// 遅延削除待ちか
        bool IsPendingDelete() const noexcept;

    protected:
        /// 参照カウントが0になった時の処理
        /// デフォルトでは即座に削除
        /// 遅延削除が必要な場合はオーバーライド
        virtual void OnZeroRefCount() const;

    private:
        mutable bool m_pendingDelete = false;

        friend class RHIDeferredDeleteQueue;

        /// 遅延削除キューから呼び出し
        void ExecuteDeferredDelete() const;
```

- [ ] MarkForDeferredDelete
- [ ] OnZeroRefCount 仮想関数
- [ ] ExecuteDeferredDelete

### 5. IRHIResource: マーカー・ユーティリティ

```cpp
    public:
        //=====================================================================
        // マーカー（派生クラス識別用）
        //=====================================================================

        /// バッファか
        virtual bool IsBuffer() const { return false; }

        /// テクスチャか
        virtual bool IsTexture() const { return false; }

        /// ビューか
        virtual bool IsView() const { return false; }

        //=====================================================================
        // GPU常駐状態（Residency対応用）
        //=====================================================================

        /// GPUメモリに常駐しているか
        virtual bool IsResident() const { return true; }

        /// 常駐優先度設定
        virtual void SetResidencyPriority(uint32 priority) { (void)priority; }

    protected:
        /// 派生クラスのみ構築可能
        IRHIResource(ERHIResourceType type);

    private:
        ERHIResourceType m_resourceType;
    };

} // namespace NS::RHI
```

- [ ] 型判定仮想関数
- [ ] Residency対応インターフェース

### 6. RHIResourceLocation（追加）

```cpp
namespace NS::RHI
{
    /// リソースメモリロケーション
    /// GPUリソースの実際のメモリ位置を示す
    struct RHI_API RHIResourceLocation
    {
        /// 基盤リソースへの参照
        IRHIResource* resource = nullptr;

        /// リソース内オフセット（サブアロケーション時）
        uint64 offset = 0;

        /// 割り当てサイズ
        uint64 size = 0;

        /// GPU仮想アドレス
        uint64 gpuVirtualAddress = 0;

        /// 有効か
        bool IsValid() const { return resource != nullptr; }

        /// GPU仮想アドレス取得（オフセット適用済み）
        uint64 GetGPUVirtualAddress() const {
            return gpuVirtualAddress + offset;
        }
    };
}
```

- [ ] RHIResourceLocation 構造体
- [ ] サブアロケーション対応

## 検証方法

- [ ] 参照カウントのスレッドセーフティ
- [ ] 遅延削除フローの正常動作
- [ ] メモリリーク検出（デバッグビルド）
- [ ] デバッグ名の並行アクセステスト（複数スレッドからSetDebugName/GetDebugNameを同時呼び出し）
