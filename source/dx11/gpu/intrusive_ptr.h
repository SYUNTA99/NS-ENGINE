//----------------------------------------------------------------------------
//! @file   intrusive_ptr.h
//! @brief  侵入型参照カウント基底クラスとスマートポインタ
//----------------------------------------------------------------------------
#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace mutra {

//===========================================================================
//! @brief 侵入型参照カウント基底クラス
//! @note  Layout (16 bytes): vtable(8) + refCount_(4) + pad_(4)
//===========================================================================
class RefCounted {
public:
    RefCounted() : refCount_(0), pad_(0) {}
    virtual ~RefCounted() = default;

    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;
    RefCounted(RefCounted&&) = delete;
    RefCounted& operator=(RefCounted&&) = delete;

    //! 参照カウントを増加
    void AddRef() const noexcept {
        refCount_.fetch_add(1, std::memory_order_relaxed);
    }

    //! 参照カウントを減少（0になったら破棄）
    void Release() const noexcept {
        if (refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            Destroy();
        }
    }

    //! 現在の参照カウントを取得
    [[nodiscard]] uint32_t RefCount() const noexcept {
        return refCount_.load(std::memory_order_relaxed);
    }

protected:
    //! オブジェクト破棄（派生クラスでオーバーライド可能）
    virtual void Destroy() const {
        delete this;
    }

private:
    mutable std::atomic<uint32_t> refCount_;
    uint32_t pad_;
};

static_assert(sizeof(RefCounted) == 16, "RefCounted must be 16 bytes");

//===========================================================================
//! @brief 侵入型スマートポインタ
//! @tparam T RefCounted派生クラス
//===========================================================================
template<typename T>
class IntrusivePtr {
public:
    using ElementType = T;

    IntrusivePtr() noexcept : ptr_(nullptr) {}
    IntrusivePtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

    //! 生ポインタから構築
    //! @param p ポインタ
    //! @param addRef trueなら参照カウント増加
    explicit IntrusivePtr(T* p, bool addRef = true) noexcept : ptr_(p) {
        if (ptr_ && addRef) ptr_->AddRef();
    }

    //! コピーコンストラクタ
    IntrusivePtr(const IntrusivePtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_) ptr_->AddRef();
    }

    //! 変換コピーコンストラクタ
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    IntrusivePtr(const IntrusivePtr<U>& other) noexcept : ptr_(other.Get()) {
        if (ptr_) ptr_->AddRef();
    }

    //! ムーブコンストラクタ
    IntrusivePtr(IntrusivePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    //! 変換ムーブコンストラクタ
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    IntrusivePtr(IntrusivePtr<U>&& other) noexcept : ptr_(other.Detach()) {}

    //! デストラクタ
    ~IntrusivePtr() {
        if (ptr_) ptr_->Release();
    }

    //! コピー代入
    IntrusivePtr& operator=(const IntrusivePtr& other) noexcept {
        IntrusivePtr(other).Swap(*this);
        return *this;
    }

    //! ムーブ代入
    IntrusivePtr& operator=(IntrusivePtr&& other) noexcept {
        IntrusivePtr(std::move(other)).Swap(*this);
        return *this;
    }

    //! nullptr代入
    IntrusivePtr& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    }

    //! 生ポインタを取得
    [[nodiscard]] T* Get() const noexcept { return ptr_; }

    //! アロー演算子
    T* operator->() const noexcept { return ptr_; }

    //! 間接参照演算子
    T& operator*() const noexcept { return *ptr_; }

    //! 有効性チェック
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    //! 参照を解放
    void Reset() noexcept {
        if (ptr_) {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }

    //! 所有権を放棄して生ポインタを返す
    [[nodiscard]] T* Detach() noexcept {
        T* p = ptr_;
        ptr_ = nullptr;
        return p;
    }

    //! 交換
    void Swap(IntrusivePtr& other) noexcept {
        T* tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

private:
    T* ptr_;
};

// 比較演算子
template<typename T, typename U>
bool operator==(const IntrusivePtr<T>& a, const IntrusivePtr<U>& b) noexcept {
    return a.Get() == b.Get();
}

template<typename T, typename U>
bool operator!=(const IntrusivePtr<T>& a, const IntrusivePtr<U>& b) noexcept {
    return a.Get() != b.Get();
}

template<typename T>
bool operator==(const IntrusivePtr<T>& p, std::nullptr_t) noexcept { return !p; }

template<typename T>
bool operator!=(const IntrusivePtr<T>& p, std::nullptr_t) noexcept { return static_cast<bool>(p); }

//! 静的キャスト
template<typename T, typename U>
IntrusivePtr<T> StaticPointerCast(const IntrusivePtr<U>& p) noexcept {
    return IntrusivePtr<T>(static_cast<T*>(p.Get()));
}

//! 動的キャスト
template<typename T, typename U>
IntrusivePtr<T> DynamicPointerCast(const IntrusivePtr<U>& p) noexcept {
    return IntrusivePtr<T>(dynamic_cast<T*>(p.Get()));
}

} // namespace mutra
