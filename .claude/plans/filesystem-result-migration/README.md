# FileSystem Result型移行

## 目的

`engine/fs/` モジュールの独自Result型（FileError, FileReadResult, FileOperationResult）を `common/result` のResult型システムに移行し、エンジン全体で統一されたエラーハンドリングを実現する。

## 影響範囲

- `source/engine/fs/` - FileSystem モジュール全体
- FileSystemを利用する他モジュール（Texture, Mesh, Shader等）

## サブ計画

| # | 計画 | 状態 |
|---|------|------|
| 1 | [FileSystemResult拡張](01-filesystem-result-extension.md) | **complete** ✅ 確認済み |
| 2 | [FileError置き換え](02-file-error-replacement.md) | **complete** ✅ |
| 3 | [FileReadResult置き換え](03-file-read-result-replacement.md) | pending |
| 4 | [FileOperationResult置き換え](04-file-operation-result-replacement.md) | pending |

## 検証方法

1. 既存のFileSystemテストが全てパス
2. Texture/Mesh/Shaderのロードが正常動作
3. エラー発生時に適切なResult値が返される
4. 非同期読み込み（AsyncReadHandle）が正常動作

## 設計決定

| 決定 | 理由 |
|------|------|
| FileErrorを段階的に置き換え | 一度に全て変更すると影響範囲が大きすぎる |
| 変換ユーティリティを用意 | 移行期間中の互換性を確保 |
| AsyncReadStateは維持 | 状態enumであり、Result型とは概念が異なる |
| outパラメータでデータ返却 | Result<T>より既存コードとの親和性が高い |
| nativeError情報は移行時に喪失を許容 | ログ出力で対応。NS::Resultは32bit制約があり追加データを含められない |
| AsyncReadHandleは今回の移行対象外 | 非同期パターンの再設計が必要。将来的にテンプレート化を検討 |
| readAsText/readAsCharsはread()のデフォルト実装を利用 | read()の署名変更に伴い自動的に移行される |

## 移行戦略

```
Phase 1: FileSystemResult.h にエラーコード追加
         ↓
Phase 2: FileError → NS::Result 変換関数作成
         ↓
Phase 3: 内部実装を NS::Result に移行（FileError は互換レイヤーとして残す）
         ↓
Phase 4: 外部API を NS::Result に変更
         ↓
Phase 5: FileError/FileReadResult/FileOperationResult を削除
```
