# サブ計画 1: FileSystemResult拡張

## 目的

`common/result/Module/FileSystemResult.h` に不足しているエラーコードを追加し、FileError との完全なマッピングを可能にする。

## 変更対象ファイル

- `source/common/result/Module/FileSystemResult.h`

## TODO

- [x] `ResultMountNotFound()` を追加（InvalidMount エラー対応） — FileSystemResult.h L114, error code 500
- [x] `ResultNotADirectory()` を追加（IsNotDirectory エラー対応） — FileSystemResult.h L44, error code 7
- [x] エラーコードの説明コメントを追加 — 各関数にドキュメントコメント記載済み
- [x] FileError::Code との対応表をコメントで記載 — FileSystemResult.h 冒頭（L4-20）に完全な対応表

## アプローチ

既存の `FileSystemResult.h` のパターンに従い、新しいエラーコードを追加:

```cpp
/// マウントが無効または見つからない
[[nodiscard]] inline ::NS::Result ResultInvalidMount() noexcept
{
    return MakeResult(FileSystemDescription::InvalidMount);
}

/// パスがディレクトリではない（ディレクトリ操作に対して）
[[nodiscard]] inline ::NS::Result ResultIsNotDirectory() noexcept
{
    return MakeResult(FileSystemDescription::IsNotDirectory);
}
```

## 検証方法

- ビルドが成功すること
- 新しいエラーコードが正しいモジュールID/説明IDを持つこと

## 影響範囲

- 追加のみ、既存コードへの影響なし
