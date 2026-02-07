# 01-11: TRefCountPtrテンプレート

## 目的

RHIリソースの参照カウント管理用スマートポインタテンプレートを定義する。

## 参照ドキュメント

- docs/Graphics_Abstraction_Coding_Patterns.md (参照カウントパターン)
- Source/Common/Platform/ComPtr.h (既存COMポインタ参照)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIRefCountPtr.h`

## TODO

### 1. TRefCountPtr: 基本実装

```cpp
#pragma once

#include "Common/Utility/Types.h"
#include "RHIMacros.h"

namespace NS::RHI
{
    /// 参照カウントスマートポインタ
    /// T は AddRef() / Release() メソッドを持つ必要がある
    template<typename T>
    class TRefCountPtr
    {
    public:
        //=====================================================================
        // コンストラクタ / デストラクタ
        //=====================================================================

        /// デフォルト（null）
        TRefCountPtr() noexcept : m_ptr(nullptr) {}

        /// nullptrから
        TRefCountPtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

        /// 生ポインタから（参照カウント増加）
        explicit TRefCountPtr(T* ptr) noexcept : m_ptr(ptr)
        {
            if (m_ptr) {
                m_ptr->AddRef();
            }
        }

        /// コピーコンストラクタ
        TRefCountPtr(const TRefCountPtr& other) noexcept : m_ptr(other.m_ptr)
        {
            if (m_ptr) {
                m_ptr->AddRef();
            }
        }

        /// ムーブコンストラクタ
        TRefCountPtr(TRefCountPtr&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        /// 派生クラスからのコピー
        template<typename U>
        TRefCountPtr(const TRefCountPtr<U>& other) noexcept : m_ptr(other.Get())
        {
            if (m_ptr) {
                m_ptr->AddRef();
            }
        }

        /// 派生クラスからのムーブ
        template<typename U>
        TRefCountPtr(TRefCountPtr<U>&& other) noexcept : m_ptr(other.Detach())
        {
        }

        /// デストラクタ
        ~TRefCountPtr() noexcept
        {
            InternalRelease();
        }
```

- [ ] コンストラクタ群
- [ ] デストラクタ

### 2. TRefCountPtr: 代入演算子

```cpp
        //=====================================================================
        // 代入演算子
        //=====================================================================

        /// nullptr代入
        TRefCountPtr& operator=(std::nullptr_t) noexcept
        {
            InternalRelease();
            return *this;
        }

        /// 生ポインタ代入
        TRefCountPtr& operator=(T* ptr) noexcept
        {
            if (m_ptr != ptr) {
                InternalRelease();
                m_ptr = ptr;
                if (m_ptr) {
                    m_ptr->AddRef();
                }
            }
            return *this;
        }

        /// コピー代入
        TRefCountPtr& operator=(const TRefCountPtr& other) noexcept
        {
            if (m_ptr != other.m_ptr) {
                InternalRelease();
                m_ptr = other.m_ptr;
                if (m_ptr) {
                    m_ptr->AddRef();
                }
            }
            return *this;
        }

        /// ムーブ代入
        TRefCountPtr& operator=(TRefCountPtr&& other) noexcept
        {
            if (this != &other) {
                InternalRelease();
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        /// 派生クラスからのコピー代入
        template<typename U>
        TRefCountPtr& operator=(const TRefCountPtr<U>& other) noexcept
        {
            if (m_ptr != other.Get()) {
                InternalRelease();
                m_ptr = other.Get();
                if (m_ptr) {
                    m_ptr->AddRef();
                }
            }
            return *this;
        }
```

- [ ] 代入演算子群

### 3. TRefCountPtr: アクセス演算子

```cpp
        //=====================================================================
        // アクセス
        //=====================================================================

        /// ポインタアクセス
        T* operator->() const noexcept
        {
            RHI_CHECK(m_ptr != nullptr);
            return m_ptr;
        }

        /// 参照アクセス
        T& operator*() const noexcept
        {
            RHI_CHECK(m_ptr != nullptr);
            return *m_ptr;
        }

        /// 生ポインタ取得
        T* Get() const noexcept { return m_ptr; }

        /// ポインタのアドレス取得（COM互換）
        /// 既存ポインタはリリースされる
        T** GetAddressOf() noexcept
        {
            InternalRelease();
            return &m_ptr;
        }

        /// ポインタのアドレス取得（リリースなし）
        T** GetAddressOfNoRelease() noexcept
        {
            return &m_ptr;
        }

        /// bool変換
        explicit operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }
```

- [ ] アクセス演算子
- [ ] GetAddressOf()

### 4. TRefCountPtr: ユーティリティメソッド

```cpp
        //=====================================================================
        // ユーティリティ
        //=====================================================================

        /// ポインタを切り離して返す（参照カウント維持）
        T* Detach() noexcept
        {
            T* ptr = m_ptr;
            m_ptr = nullptr;
            return ptr;
        }

        /// ポインタを付け替える（AddRefなし）
        /// 既にAddRefされたポインタを受け取る用
        void Attach(T* ptr) noexcept
        {
            InternalRelease();
            m_ptr = ptr;
        }

        /// リセット
        void Reset(T* ptr = nullptr) noexcept
        {
            if (m_ptr != ptr) {
                InternalRelease();
                m_ptr = ptr;
                if (m_ptr) {
                    m_ptr->AddRef();
                }
            }
        }

        /// スワップ
        void Swap(TRefCountPtr& other) noexcept
        {
            T* temp = m_ptr;
            m_ptr = other.m_ptr;
            other.m_ptr = temp;
        }

        /// 現在の参照カウント取得（デバッグ用）
        uint32 GetRefCount() const noexcept
        {
            return m_ptr ? m_ptr->GetRefCount() : 0;
        }

        /// 有効か
        bool IsValid() const noexcept { return m_ptr != nullptr; }

    private:
        void InternalRelease() noexcept
        {
            if (m_ptr) {
                m_ptr->Release();
                m_ptr = nullptr;
            }
        }

        T* m_ptr;

        // 他のTRefCountPtrからのアクセス用
        template<typename U> friend class TRefCountPtr;
    };
```

- [ ] Detach / Attach
- [ ] Reset / Swap
- [ ] GetRefCount (デバッグ用)

### 5. TRefCountPtr: 比較演算子・ヘルパー

```cpp
    //=========================================================================
    // 比較演算子
    //=========================================================================

    template<typename T, typename U>
    bool operator==(const TRefCountPtr<T>& a, const TRefCountPtr<U>& b) noexcept {
        return a.Get() == b.Get();
    }

    template<typename T, typename U>
    bool operator!=(const TRefCountPtr<T>& a, const TRefCountPtr<U>& b) noexcept {
        return a.Get() != b.Get();
    }

    template<typename T>
    bool operator==(const TRefCountPtr<T>& a, std::nullptr_t) noexcept {
        return a.Get() == nullptr;
    }

    template<typename T>
    bool operator!=(const TRefCountPtr<T>& a, std::nullptr_t) noexcept {
        return a.Get() != nullptr;
    }

    //=========================================================================
    // ヘルパー関数
    //=========================================================================

    /// AddRefせずにポインタをラップ（ファクトリ関数用）
    template<typename T>
    TRefCountPtr<T> MakeRefCountPtr(T* ptr) noexcept
    {
        TRefCountPtr<T> result;
        result.Attach(ptr);
        return result;
    }

    /// 静的キャスト
    template<typename To, typename From>
    TRefCountPtr<To> StaticCast(const TRefCountPtr<From>& from)
    {
        return TRefCountPtr<To>(static_cast<To*>(from.Get()));
    }

    /// 動的キャスト
    template<typename To, typename From>
    TRefCountPtr<To> DynamicCast(const TRefCountPtr<From>& from)
    {
        return TRefCountPtr<To>(dynamic_cast<To*>(from.Get()));
    }

} // namespace NS::RHI
```

- [ ] 比較演算子
- [ ] MakeRefCountPtr ヘルパー
- [ ] キャストヘルパー

## 検証方法

- [ ] コピー/ムーブの参照カウント正確性
- [ ] Detach/Attach のリーク防止
- [ ] 派生クラス変換の正確性
- [ ] スレッドセーフティ（AddRef/Releaseに依存）
