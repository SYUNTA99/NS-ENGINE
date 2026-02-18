# 02: モジュール骨格ファイル

## 目的
D3D12RHIモジュールの最小ヘッダー群を作成する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part01_Architecture.md §1

## TODO
- [ ] D3D12RHI/Internal/D3D12ThirdParty.h — d3d12.h/dxgi1_6.h集約、COM smartポインタ
- [ ] D3D12RHI/Public/D3D12RHI.h — 公開定数（MAX_SRVS等）、エントリポイント宣言
- [ ] D3D12RHI/Private/D3D12RHIPrivate.h — 内部共通ヘッダー、ユーティリティマクロ

## 完了条件
- #include <d3d12.h> と #include <dxgi1_6.h> がコンパイル通過

## 見積もり
- 新規ファイル: 3
