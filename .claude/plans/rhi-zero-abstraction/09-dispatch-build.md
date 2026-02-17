# 09: DispatchTable照合 + Build Shipping

## 対象ファイル
- `source/engine/RHI/Public/RHIDispatchTable.h`
- `source/common/utility/macros.h`
- `premake5.lua`

## Part A: DispatchTable照合 + 名前整合

### 1. エントリ全数照合

01-06で全Context HOTメソッドがNS_RHI_DISPATCHを使うようになった。
DispatchTableのエントリが全NS_RHI_DISPATCH呼び出しをカバーしているか照合。

照合方法:
```bash
# NS_RHI_DISPATCH で使われる全関数名を抽出
grep -oP 'NS_RHI_DISPATCH\(\K[^,]+' IRHICommandContextBase.h IRHIComputeContext.h IRHICommandContext.h IRHIUploadContext.h | sort -u

# DispatchTable 全エントリを抽出
grep -oP '\(\*\K\w+(?=\))' RHIDispatchTable.h | sort -u

# 差分確認
diff <(list1) <(list2)
```

### 2. 名前マッピング表の確定

| Context メソッド名 | NS_RHI_DISPATCH引数 | DispatchTableエントリ |
|-------------------|--------------------|--------------------|
| `SetComputeRootConstantBufferView` | `SetComputeRootCBV` | `SetComputeRootCBV` |
| `SetComputeRootShaderResourceView` | `SetComputeRootSRV` | `SetComputeRootSRV` |
| `SetComputeRootUnorderedAccessView` | `SetComputeRootUAV` | `SetComputeRootUAV` |
| `SetGraphicsRootConstantBufferView` | `SetGraphicsRootCBV` | `SetGraphicsRootCBV` |
| `SetGraphicsRootShaderResourceView` | `SetGraphicsRootSRV` | `SetGraphicsRootSRV` |
| `SetGraphicsRootUnorderedAccessView` | `SetGraphicsRootUAV` | `SetGraphicsRootUAV` |

### 3. IsValid() 更新

不足エントリ追加後、IsValid() の必須チェックリストを更新。

### 4. オプショナルチェック確認

- `HasMeshShaderSupport()` — Mesh系4エントリ
- `HasRayTracingSupport()` — RT系4エントリ
- `HasWorkGraphSupport()` — WG系3エントリ
- `HasVariableRateShadingSupport()` — VRS系2エントリ

## Part B: Build system — Shipping config

### 1. NS_BUILD_SHIPPING マクロ追加

```cpp
// macros.h — 既存 NS_BUILD_DEBUG/NS_BUILD_RELEASE の隣に追加
#if defined(NS_SHIPPING)
    #define NS_BUILD_SHIPPING 1
#else
    #define NS_BUILD_SHIPPING 0
#endif
```

### 2. premake5.lua に Shipping 構成追加

```lua
configurations { "Debug", "Release", "Burst", "Shipping" }

filter "configurations:Shipping"
    defines { "NS_SHIPPING", "NDEBUG" }
    optimize "Full"
    flags { "LinkTimeOptimization" }  -- LTO有効化
```

### 3. NS_RHI_STATIC_BACKEND 連携文書化

RHIDispatchTable.h のコメントに手順追記。

## TODO
- [ ] NS_RHI_DISPATCH使用関数名 vs DispatchTableエントリの完全照合
- [ ] 不足エントリがあれば追加
- [ ] IsValid() 更新
- [ ] オプショナルチェック4種の整合確認
- [ ] macros.h に NS_BUILD_SHIPPING 追加
- [ ] premake5.lua に Shipping 構成追加
- [ ] 既存3構成（Debug/Release/Burst）が影響を受けないこと確認
- [ ] NS_RHI_STATIC_BACKEND 連携手順をコメント文書化
- [ ] 全4構成ビルド確認
