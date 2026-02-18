# 19: ディスクリプタヒープ

## 目的
D3D12の4タイプディスクリプタヒープとアロケータ。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md
- source/engine/RHI/Public/RHIDescriptorHeap.h

## TODO
- [ ] D3D12Descriptors.h/.cpp 作成
- [ ] 4タイプヒープ: CBV_SRV_UAV, SAMPLER, RTV, DSV（サイズ制約下記参照）
- [ ] フリーリスト方式CPUディスクリプタアロケータ + オフライン→オンラインCopyDescriptors
- [ ] シェーダー可視ヒープ線形アロケータ（フレーム単位リセット）
- [ ] Null/Defaultディスクリプタ作成（デバイス初期化時: null SRV/UAV/RTV/DSV、未バインドスロット用）

## 制約
- ヒープサイズ上限: CBV_SRV_UAV 最大1,000,000、SAMPLER 最大2,048（D3D12ハード制約）
- 推奨初期サイズ: CBV_SRV_UAV ~500K、RTV ~256、DSV ~256、SAMPLER ~2048
- ディスクリプタサイズはデバイス依存: GetDescriptorHandleIncrementSize()で取得
- 未バインドスロットにもvalid descriptorが必要（D3D12デバッグレイヤーエラー回避）

## 完了条件
- ディスクリプタ確保/解放/コピー動作、nullディスクリプタ作成済み

## 見積もり
- 新規ファイル: 2 (D3D12Descriptors.h/.cpp)
