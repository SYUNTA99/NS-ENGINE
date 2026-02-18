# 42: RT PSO + シェーダー識別子

## 目的
レイトレーシングPipeline State Objectとシェーダー識別子管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §RTPSO

## TODO
- [ ] D3D12RayTracingPSO.h/.cpp — RTPSO管理
- [ ] ID3D12StateObject (RAYTRACING) + Collection-basedキャッシュ
- [ ] シェーダーライブラリ登録 + HitGroup定義（エントリ名衝突回避: {Name}_{Hash}リネーム）
- [ ] GetShaderIdentifier → 32バイトID取得（全ビット~0 = invalid、IsValid()検証必須）
- [ ] グローバル/ローカルRoot Signature

## 制約
- RayGen: 空ローカルRoot Signature必須（非空だとRTPSO作成失敗）
- HitGroup: CHS(必須) + AHS(任意) + IS(任意)、最大3エントリ/HitGroup
- エントリ名衝突: 同名シェーダー複数登録時は{ShaderName}_{UniqueHash}でリネーム必須
- MaxPayloadSize/MaxAttributeSize: RTPSO内全シェーダーで一致必須

## 完了条件
- RT PSO作成、シェーダー識別子取得成功、制約遵守

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingPSO.h/.cpp)
