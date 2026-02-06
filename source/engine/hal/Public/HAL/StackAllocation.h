/// @file StackAllocation.h
/// @brief スタックアロケーションユーティリティ
#pragma once

#include "HAL/AlignmentTemplates.h"
#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"
#include <utility>

#if PLATFORM_WINDOWS
#include <malloc.h>
#define NS_ALLOCA(size) _alloca(size)
#else
#include <alloca.h>
#define NS_ALLOCA(size) alloca(size)
#endif

/// アライメント付きスタック確保
/// @param size 確保サイズ
/// @param alignment アライメント（2の累乗）
#define NS_ALLOCA_ALIGNED(size, alignment) NS::AlignPtr(NS_ALLOCA((size) + (alignment) - 1), alignment)

/// 型付きスタック確保
/// @param type 型
/// @param count 要素数
#define NS_ALLOCA_TYPED(type, count) static_cast<type*>(NS_ALLOCA_ALIGNED(sizeof(type) * (count), alignof(type)))

/// スコープ内でのみ有効な一時配列を確保
/// @param type 要素型
/// @param name 変数名
/// @param count 要素数
#define NS_TEMP_ARRAY(type, name, count) type* name = NS_ALLOCA_TYPED(type, count)

/// スコープ内でのみ有効な一時バッファを確保
/// @param name 変数名
/// @param size バイトサイズ
#define NS_TEMP_BUFFER(name, size) uint8* name = static_cast<uint8*>(NS_ALLOCA(size))

namespace NS
{
    /// スタック上の固定サイズ配列
    ///
    /// 小さな一時配列をヒープ確保せずに使用。
    /// サイズがコンパイル時に決定される場合に使用。
    ///
    /// @tparam T 要素型
    /// @tparam N 最大要素数
    template <typename T, SIZE_T N> class TArrayOnStack
    {
    public:
        TArrayOnStack() : m_count(0) {}

        /// 要素追加
        void Add(const T& item)
        {
            NS_CHECK(m_count < N);
            m_data[m_count++] = item;
        }

        /// 要素追加（ムーブ）
        void Add(T&& item)
        {
            NS_CHECK(m_count < N);
            m_data[m_count++] = std::move(item);
        }

        /// 要素数取得
        SIZE_T Num() const { return m_count; }

        /// 容量取得
        constexpr SIZE_T Max() const { return N; }

        /// 空かどうか
        bool IsEmpty() const { return m_count == 0; }

        /// インデックスアクセス
        T& operator[](SIZE_T index)
        {
            NS_CHECK(index < m_count);
            return m_data[index];
        }

        const T& operator[](SIZE_T index) const
        {
            NS_CHECK(index < m_count);
            return m_data[index];
        }

        /// 先頭ポインタ
        T* GetData() { return m_data; }
        const T* GetData() const { return m_data; }

        /// イテレータ
        T* begin() { return m_data; }
        T* end() { return m_data + m_count; }
        const T* begin() const { return m_data; }
        const T* end() const { return m_data + m_count; }

        /// クリア
        void Reset() { m_count = 0; }

        /// 最後の要素を削除
        void Pop()
        {
            NS_CHECK(m_count > 0);
            --m_count;
        }

        /// 最後の要素取得
        T& Last()
        {
            NS_CHECK(m_count > 0);
            return m_data[m_count - 1];
        }

        const T& Last() const
        {
            NS_CHECK(m_count > 0);
            return m_data[m_count - 1];
        }

    private:
        T m_data[N];
        SIZE_T m_count;
    };

    /// スタックマーカー（RAII）
    ///
    /// allocaはスコープ終了で自動解放されるため、
    /// このクラスは主に意図の明示化に使用。
    class StackMarker
    {
    public:
        StackMarker() = default;
        ~StackMarker() = default;
    };
} // namespace NS
