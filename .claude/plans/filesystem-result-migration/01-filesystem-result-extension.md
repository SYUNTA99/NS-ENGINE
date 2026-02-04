# サブ計画 1: FileSystemResult拡張

## 目的

`common/result/Module/FileSystemResult.h` に不足しているエラーコードを追加し、FileError との完全なマッピングを可能にする。

## 変更対象ファイル

- `source/common/result/Module/FileSystemResult.h`

## TODO

- [ ] `ResultInvalidMount()` を追加（InvalidMount エラー対応）
- [ ] `ResultIsNotDirectory()` を追加（IsNotDirectory エラー対応）
- [ ] エラーコードの説明コメントを追加
- [ ] FileError::Code との対応表をコメントで記載

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
