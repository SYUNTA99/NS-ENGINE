# 01-03: ジオメトリ型

## 目的

ビューポート、矩形、エクステントなどのジオメトリ型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドコンテキスト)
- docs/RHI/RHI_Implementation_Guide_Part5.md (スワップチェーン)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHITypes.h` (部分)

## TODO

### 1. Extent2D / Extent3D: サイズ型

```cpp
namespace NS::RHI
{
    /// 2D範囲（幅・高さ）
    struct Extent2D
    {
        uint32 width = 0;
        uint32 height = 0;

        constexpr Extent2D() = default;
        constexpr Extent2D(uint32 w, uint32 h) : width(w), height(h) {}

        constexpr bool IsEmpty() const { return width == 0 || height == 0; }
        constexpr uint64 Area() const { return static_cast<uint64>(width) * height; }

        constexpr bool operator==(const Extent2D& other) const {
            return width == other.width && height == other.height;
        }
        constexpr bool operator!=(const Extent2D& other) const {
            return !(*this == other);
        }
    };

    /// 3D範囲（幅・高さ・深度）
    struct Extent3D
    {
        uint32 width = 0;
        uint32 height = 0;
        uint32 depth = 1;

        constexpr Extent3D() = default;
        constexpr Extent3D(uint32 w, uint32 h, uint32 d = 1)
            : width(w), height(h), depth(d) {}

        /// 2Dから変換
        explicit constexpr Extent3D(const Extent2D& e2d)
            : width(e2d.width), height(e2d.height), depth(1) {}

        constexpr bool IsEmpty() const {
            return width == 0 || height == 0 || depth == 0;
        }
        constexpr uint64 Volume() const {
            return static_cast<uint64>(width) * height * depth;
        }

        /// 2Dとして取得
        constexpr Extent2D ToExtent2D() const {
            return Extent2D(width, height);
        }

        constexpr bool operator==(const Extent3D& other) const {
            return width == other.width && height == other.height
                && depth == other.depth;
        }
    };
}
```

- [ ] Extent2D 構造体
- [ ] Extent3D 構造体
- [ ] 変換メソッド

### 2. RHIViewport: ビューポート

```cpp
namespace NS::RHI
{
    /// ビューポート定義
    /// 正規化されたデプス範囲 [minDepth, maxDepth]
    struct RHIViewport
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        constexpr RHIViewport() = default;

        constexpr RHIViewport(float inX, float inY, float inW, float inH,
                              float inMinD = 0.0f, float inMaxD = 1.0f)
            : x(inX), y(inY), width(inW), height(inH)
            , minDepth(inMinD), maxDepth(inMaxD) {}

        /// Extent2Dから作成
        explicit constexpr RHIViewport(const Extent2D& extent)
            : x(0.0f), y(0.0f)
            , width(static_cast<float>(extent.width))
            , height(static_cast<float>(extent.height))
            , minDepth(0.0f), maxDepth(1.0f) {}

        constexpr bool IsEmpty() const {
            return width <= 0.0f || height <= 0.0f;
        }

        /// アスペクト比
        constexpr float GetAspectRatio() const {
            return (height > 0.0f) ? (width / height) : 0.0f;
        }

        constexpr bool operator==(const RHIViewport& other) const {
            return x == other.x && y == other.y
                && width == other.width && height == other.height
                && minDepth == other.minDepth && maxDepth == other.maxDepth;
        }
    };
}
```

- [ ] RHIViewport 構造体
- [ ] Extent2D変換コンストラクタ
- [ ] アスペクト比計算

### 3. RHIRect: 整数矩形

```cpp
namespace NS::RHI
{
    /// 整数矩形（シザー矩形等）
    struct RHIRect
    {
        int32 left = 0;
        int32 top = 0;
        int32 right = 0;
        int32 bottom = 0;

        constexpr RHIRect() = default;

        constexpr RHIRect(int32 l, int32 t, int32 r, int32 b)
            : left(l), top(t), right(r), bottom(b) {}

        /// オフセット + サイズから作成
        static constexpr RHIRect FromExtent(int32 x, int32 y, uint32 w, uint32 h) {
            return RHIRect(x, y, x + static_cast<int32>(w), y + static_cast<int32>(h));
        }

        /// Extent2Dから作成（原点0,0）
        explicit constexpr RHIRect(const Extent2D& extent)
            : left(0), top(0)
            , right(static_cast<int32>(extent.width))
            , bottom(static_cast<int32>(extent.height)) {}

        constexpr int32 Width() const { return right - left; }
        constexpr int32 Height() const { return bottom - top; }
        constexpr bool IsEmpty() const { return Width() <= 0 || Height() <= 0; }

        /// Extent2Dとして取得
        constexpr Extent2D ToExtent2D() const {
            return Extent2D(
                static_cast<uint32>(Width() > 0 ? Width() : 0),
                static_cast<uint32>(Height() > 0 ? Height() : 0)
            );
        }

        constexpr bool operator==(const RHIRect& other) const {
            return left == other.left && top == other.top
                && right == other.right && bottom == other.bottom;
        }
    };
}
```

- [ ] RHIRect 構造体
- [ ] FromExtent ファクトリ
- [ ] 幅・高さ計算

### 4. RHIBox: 3Dボックス

```cpp
namespace NS::RHI
{
    /// 3Dボックス（テクスチャコピー領域等）
    struct RHIBox
    {
        uint32 left = 0;
        uint32 top = 0;
        uint32 front = 0;
        uint32 right = 0;
        uint32 bottom = 0;
        uint32 back = 0;

        constexpr RHIBox() = default;

        constexpr RHIBox(uint32 l, uint32 t, uint32 f,
                         uint32 r, uint32 b, uint32 bk)
            : left(l), top(t), front(f), right(r), bottom(b), back(bk) {}

        /// Extent3Dから作成（原点0,0,0）
        explicit constexpr RHIBox(const Extent3D& extent)
            : left(0), top(0), front(0)
            , right(extent.width), bottom(extent.height), back(extent.depth) {}

        constexpr uint32 Width() const { return right - left; }
        constexpr uint32 Height() const { return bottom - top; }
        constexpr uint32 Depth() const { return back - front; }

        constexpr bool IsEmpty() const {
            return Width() == 0 || Height() == 0 || Depth() == 0;
        }

        constexpr Extent3D ToExtent3D() const {
            return Extent3D(Width(), Height(), Depth());
        }

        constexpr bool operator==(const RHIBox& other) const {
            return left == other.left && top == other.top && front == other.front
                && right == other.right && bottom == other.bottom && back == other.back;
        }
    };
}
```

- [ ] RHIBox 構造体
- [ ] Extent3D変換

### 5. Offset2D / Offset3D: オフセット型

```cpp
namespace NS::RHI
{
    /// 2Dオフセット
    struct Offset2D
    {
        int32 x = 0;
        int32 y = 0;

        constexpr Offset2D() = default;
        constexpr Offset2D(int32 inX, int32 inY) : x(inX), y(inY) {}
    };

    /// 3Dオフセット
    struct Offset3D
    {
        int32 x = 0;
        int32 y = 0;
        int32 z = 0;

        constexpr Offset3D() = default;
        constexpr Offset3D(int32 inX, int32 inY, int32 inZ = 0)
            : x(inX), y(inY), z(inZ) {}
    };
}
```

- [ ] Offset2D / Offset3D 構造体

## 検証方法

- [ ] サイズ計算の正確性
- [ ] 変換の整合性
- [ ] 境界条件チェック
