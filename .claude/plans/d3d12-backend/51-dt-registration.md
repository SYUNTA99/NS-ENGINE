# 51: DispatchTable登録 + STATIC_BACKEND

## 目的
GRHIDispatchTableへの全関数ポインタ登録とNS_RHI_STATIC_BACKEND設定。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h
- source/engine/RHI/Private/RHIDispatchTable.cpp

## TODO
- [ ] GRHIDispatchTable全関数ポインタ登録 + IsValid()検証
- [ ] オプション機能の条件付き登録（RT/Mesh/WG/VRS）
- [ ] NS_RHI_STATIC_BACKEND=D3D12 定義（D3D12RHI.h）
- [ ] マクロ展開検証: NS_RHI_DISPATCH(Draw,...) → D3D12_Draw(...)
- [ ] premake5.lua Shipping構成 + LTO有効化確認

## 完了条件
- Dev: 全ポインタ登録、Shipping: 直接呼出しコンパイル成功

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12RHIModule, D3D12RHI.h, premake5.lua
