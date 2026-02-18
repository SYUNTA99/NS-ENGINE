# 01: premake設定 + ディレクトリ構造

## 目的
premake5.luaにD3D12RHIプロジェクトを追加し、ディレクトリ構造を作成する。

## 参照
- premake5.lua（既存構成）
- docs/D3D12RHI/D3D12RHI_Part01_Architecture.md

## TODO
- [ ] premake5.lua: project "D3D12RHI" 定義（StaticLib）+ 依存設定
- [ ] インクルードパス: Public/, Internal/, Private/
- [ ] Windows SDK パス設定（d3d12.lib, dxgi.lib, dxguid.lib）
- [ ] Shipping構成: NS_RHI_STATIC_BACKEND=D3D12 define追加
- [ ] ディレクトリ構造作成: Public/ + Internal/ + Private/

## 完了条件
- premake再生成成功、空プロジェクト認識

## 見積もり
- 新規ファイル: 0（ディレクトリのみ）
- 変更ファイル: premake5.lua
