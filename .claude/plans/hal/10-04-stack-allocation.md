# 10-04: スタックアロケーション

## 目的

スタック上での一時メモリ確保ユーティリティを実装する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション4「スタックアロケーション」
UE5.7: `FMemory_Alloca`, スタック配列マクロ

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`SIZE_T`）
- `common/utility/macros.h` - プラットフォーム検出

## 依存（HAL）

- 01-04 プラットフォーム型
- 02-03 関数呼び出し規約（`FORCEINLINE`）
- 10-02a アライメントテンプレート（`Align`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/StackAllocation.h`

## TODO

- [ ] `NS_ALLOCA` マクロ（プラットフォーム抽象化）
- [ ] `NS_ALLOCA_ALIGNED` マクロ（アライメント付き）
- [ ] `TArrayOnStack<T, N>` テンプレート
- [ ] スタックマーカーユーティリティ

## 実装内容

```cpp
// StackAllocation.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"
#include "HAL/PlatformMacros.h"
#include "HAL/AlignmentTemplates.h"

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
#define NS_ALLOCA_ALIGNED(size, alignment) \
    NS::AlignPtr(NS_ALLOCA((size) + (alignment) - 1), alignment)

/// 型付きスタック確保
/// @param type 型
/// @param count 要素数
#define NS_ALLOCA_TYPED(type, count) \
    static_cast<type*>(NS_ALLOCA_ALIGNED(sizeof(type) * (count), alignof(type)))

namespace NS
{
    /// スタック上の固定サイズ配列
    ///
    /// 小さな一時配列をヒープ確保せずに使用。
    /// サイズがコンパイル時に決定される場合に使用。
    ///
    /// @tparam T 要素型
    /// @tparam N 最大要素数
    template<typename T, SIZE_T N>
    class TArrayOnStack
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

    private:
        T m_data[N];
        SIZE_T m_count;
    };

    /// スタックマーカー（RAII）
    ///
    /// スコープ終了時にスタックポインタを復元。
    /// alloca使用時の安全性向上用。
    class StackMarker
    {
    public:
        StackMarker() : m_marker(nullptr)
        {
            // スタック位置を記録（実装はプラットフォーム依存）
        }

        ~StackMarker()
        {
            // スタック位置を復元（allocaはスコープ終了で自動解放）
        }

    private:
        void* m_marker;
    };
}

// ========== 使用例マクロ ==========

/// スコープ内でのみ有効な一時配列を確保
/// @param type 要素型
/// @param name 変数名
/// @param count 要素数
#define NS_TEMP_ARRAY(type, name, count) \
    type* name = NS_ALLOCA_TYPED(type, count)

/// スコープ内でのみ有効な一時バッファを確保
/// @param name 変数名
/// @param size バイトサイズ
#define NS_TEMP_BUFFER(name, size) \
    uint8* name = static_cast<uint8*>(NS_ALLOCA(size))
```

## 使用例

```cpp
void ProcessItems(const Item* items, SIZE_T count)
{
    // ヒープ確保なしで一時配列を作成
    NS_TEMP_ARRAY(ProcessedItem, processed, count);

    for (SIZE_T i = 0; i < count; ++i)
    {
        processed[i] = Process(items[i]);
    }

    // スコープ終了で自動解放
}

void SmallArrayExample()
{
    // コンパイル時サイズの固定配列
    TArrayOnStack<int32, 16> stack;
    stack.Add(1);
    stack.Add(2);
    stack.Add(3);

    for (int32 value : stack)
    {
        // 処理
    }
}
```

## 検証

### テストファイル

`tests/engine/hal/StackAllocationTest.cpp`

### ユニットテスト

```cpp
TEST(StackAllocation, AllocaBasic)
{
    // 基本確保
    void* ptr = NS_ALLOCA(64);
    EXPECT_NE(ptr, nullptr);
}

TEST(StackAllocation, AllocaAligned)
{
    void* ptr = NS_ALLOCA_ALIGNED(64, 32);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 32, 0);  // 32バイト境界
}

TEST(TArrayOnStack, BasicOperations)
{
    NS::TArrayOnStack<int32, 8> arr;
    EXPECT_TRUE(arr.IsEmpty());
    EXPECT_EQ(arr.Max(), 8);

    arr.Add(10);
    arr.Add(20);
    EXPECT_EQ(arr.Num(), 2);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 11);

    arr.Pop();
    EXPECT_EQ(arr.Num(), 1);
}

TEST(TArrayOnStack, BoundaryCheck)
{
    NS::TArrayOnStack<int32, 2> arr;
    arr.Add(1);
    arr.Add(2);
    // arr.Add(3);  // NS_CHECK失敗（Debugビルドでassert）
}

TEST(TArrayOnStack, Iterator)
{
    NS::TArrayOnStack<int32, 4> arr;
    arr.Add(1);
    arr.Add(2);
    arr.Add(3);

    int32 sum = 0;
    for (int32 v : arr) { sum += v; }
    EXPECT_EQ(sum, 6);
}
```

### エラーケース

| 操作 | 期待動作 | ビルド |
|------|----------|--------|
| 容量超過Add | NS_CHECK失敗 | Debug: assert, Release: 未定義 |
| 範囲外アクセス | NS_CHECK失敗 | Debug: assert, Release: 未定義 |
| 空配列でPop | NS_CHECK失敗 | Debug: assert |

## 注意事項

- スタックサイズには制限がある（通常1MB程度）
- 大きなサイズには使用しない（数KB以下推奨）
- ループ内での使用は避ける（スタックオーバーフロー）
- 関数から返さない（スコープ終了で無効化）

## 次のサブ計画

（Part10完了）
