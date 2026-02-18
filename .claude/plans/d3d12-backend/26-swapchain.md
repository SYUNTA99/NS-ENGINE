# 26: SwapChain + バックバッファ

## 目的
DXGI SwapChain作成とバックバッファ管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part12_ViewportShaders.md
- source/engine/RHI/Public/IRHISwapChain.h

## TODO
- [ ] D3D12SwapChain.h/.cpp + IRHISwapChain実装
- [ ] IDXGISwapChain4作成: FLIP_DISCARD + SCALING_NONE + BufferCount=3 + ALLOW_TEARING(条件付き)
- [ ] バックバッファ取得 → D3D12Texture化 + RTV作成（SRVも作成）
- [ ] フレームインデックス管理（GetCurrentBackBufferIndex） + Resize対応
- [ ] Tearing対応判定 + HDRカラースペース: CheckFeatureSupport(ALLOW_TEARING)、SwapChain4::CheckColorSpaceSupport() → SetColorSpace1()

## 制約
- DXGI_SWAP_EFFECT_FLIP_DISCARD必須（Windows 10+で他のスワップエフェクトは非推奨/エラー）
- Usage: RENDER_TARGET_OUTPUT | SHADER_INPUT（両方必要）
- Resize時: 全バックバッファ解放 → ResizeBuffers → 再取得（GPU idle待機必須）
- HDR検出: GetContainingOutput() → IDXGIOutput6::GetDesc1() → G2084トランスファーファンクション確認
- カラースペースマッピング: SDR→RGB_FULL_G22_NONE_P709、HDR ST2084→RGB_FULL_G2084_NONE_P2020、HDR ScRGB→RGB_FULL_G10_NONE_P709

## 完了条件
- SwapChain作成成功、バックバッファRTV取得可能、FLIP_DISCARD動作

## 見積もり
- 新規ファイル: 2 (D3D12SwapChain.h/.cpp)
