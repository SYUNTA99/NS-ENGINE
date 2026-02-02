---
description: "コードレビュー（セキュリティ・品質・パフォーマンス・テスト）"
---

# Context
- Current diff: !`git diff`
- Staged diff: !`git diff --staged`

# Task
現在の変更をレビュー:

## Security
- [ ] ハードコードされた秘密情報や認証情報がない
- [ ] 入力バリデーションがある
- [ ] SQLインジェクション対策

## Code Quality
- [ ] 関数が小さく焦点が絞られている
- [ ] コードの重複がない
- [ ] 適切なエラーハンドリング

## Performance
- [ ] N+1クエリがない
- [ ] 効率的なアルゴリズム
- [ ] 必要な箇所にキャッシュ

## Testing
- [ ] 新機能にユニットテストがある
- [ ] エッジケースを考慮
