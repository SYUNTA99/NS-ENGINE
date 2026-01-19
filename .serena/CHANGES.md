# Serena MCP Configuration Changes

## 2025-12-13: ツール制限の修正

### 問題
`included_tools`(ホワイトリスト方式)はSerenaでサポートされていない。
設定しても無視され、全ツールが公開されていた。

### 解決策
`excluded_tools`(ブラックリスト方式)に変更。
不要なツールを明示的に除外。

### 変更内容

**Before (動作しない):**
```yaml
included_tools:
  - get_symbols_overview
  - find_symbol
  - ...
```

**After (動作する):**
```yaml
excluded_tools:
  - read_file
  - list_dir
  - find_file
  - search_for_pattern
  - check_onboarding_performed
  - onboarding
  - think_about_collected_information
  - think_about_task_adherence
  - think_about_whether_you_are_done
  - prepare_for_new_conversation
  - initial_instructions
  - write_memory
  - read_memory
  - list_memories
  - delete_memory
  - edit_memory
  - get_current_config
```

### 残るツール (7個)
| ツール | 用途 |
|--------|------|
| `get_symbols_overview` | ファイル内シンボル一覧 |
| `find_symbol` | シンボル検索 |
| `find_referencing_symbols` | 参照箇所検索 |
| `replace_symbol_body` | シンボル本体の置換 |
| `insert_after_symbol` | シンボル後に挿入 |
| `insert_before_symbol` | シンボル前に挿入 |
| `rename_symbol` | シンボル名変更 |

### 理由
Claude Codeは独自のファイル操作・検索ツール(Glob, Grep, Read, Edit等)を持つため、
Serenaからはシンボル操作ツールのみ提供すれば十分。
重複を排除してトークン消費を削減。

### 適用方法
Claude Code再起動でMCPサーバーが再読み込みされる。
