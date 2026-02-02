---
description: Create a git commit with a conventional message
allowed-tools: Bash(git add:*), Bash(git status:*), Bash(git commit:*), Bash(git diff:*)
model: haiku
---
# Context
- Current git status: !`git status`
- Current git diff: !`git diff HEAD`
- Current branch: !`git branch --show-current`
- Recent commits: !`git log --oneline -10`

# Task
1. Analyze the diff and generate 3 commit message candidates
2. Follow Conventional Commits format: type(scope): description
3. Keep subject ≤50 chars
4. After approval, run git add -A && git commit
