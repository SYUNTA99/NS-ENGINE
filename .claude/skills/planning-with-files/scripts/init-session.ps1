# Initialize planning files for a new session
# Usage: .\init-session.ps1 [project-name] [plan-dir]

param(
    [string]$ProjectName = "project",
    [string]$PlanDir = ".claude/plans/current"
)

$DATE = Get-Date -Format "yyyy-MM-dd"

Write-Host "Initializing planning files for: $ProjectName"
Write-Host "Plan directory: $PlanDir"

# Create plan directory if it doesn't exist
if (-not (Test-Path $PlanDir)) {
    New-Item -ItemType Directory -Path $PlanDir -Force | Out-Null
    Write-Host "Created directory: $PlanDir"
}

# Create task_plan.md if it doesn't exist
if (-not (Test-Path "$PlanDir/task_plan.md")) {
    @"
# Task Plan: [Brief Description]

## Goal
[One sentence describing the end state]

## Current Phase
Phase 1

## Phases

### Phase 1: Requirements & Discovery
- [ ] Understand user intent
- [ ] Identify constraints
- [ ] Document in findings.md
- **Status:** in_progress

### Phase 2: Planning & Structure
- [ ] Define approach
- [ ] Create project structure
- **Status:** pending

### Phase 3: Implementation
- [ ] Execute the plan
- [ ] Write to files before executing
- **Status:** pending

### Phase 4: Testing & Verification
- [ ] Verify requirements met
- [ ] Document test results
- **Status:** pending

### Phase 5: Delivery
- [ ] Review outputs
- [ ] Deliver to user
- **Status:** pending

## Decisions Made
| Decision | Rationale |
|----------|-----------|

## Errors Encountered
| Error | Resolution |
|-------|------------|
"@ | Out-File -FilePath "$PlanDir/task_plan.md" -Encoding UTF8
    Write-Host "Created $PlanDir/task_plan.md"
} else {
    Write-Host "$PlanDir/task_plan.md already exists, skipping"
}

# Create findings.md if it doesn't exist
if (-not (Test-Path "$PlanDir/findings.md")) {
    @"
# Findings & Decisions

## Requirements
-

## Research Findings
-

## Technical Decisions
| Decision | Rationale |
|----------|-----------|

## Issues Encountered
| Issue | Resolution |
|-------|------------|

## Resources
-
"@ | Out-File -FilePath "$PlanDir/findings.md" -Encoding UTF8
    Write-Host "Created $PlanDir/findings.md"
} else {
    Write-Host "$PlanDir/findings.md already exists, skipping"
}

# Create progress.md if it doesn't exist
if (-not (Test-Path "$PlanDir/progress.md")) {
    @"
# Progress Log

## Session: $DATE

### Current Status
- **Phase:** 1 - Requirements & Discovery
- **Started:** $DATE

### Actions Taken
-

### Test Results
| Test | Expected | Actual | Status |
|------|----------|--------|--------|

### Errors
| Error | Resolution |
|-------|------------|
"@ | Out-File -FilePath "$PlanDir/progress.md" -Encoding UTF8
    Write-Host "Created $PlanDir/progress.md"
} else {
    Write-Host "$PlanDir/progress.md already exists, skipping"
}

Write-Host ""
Write-Host "Planning files initialized!"
Write-Host "Files: $PlanDir/task_plan.md, $PlanDir/findings.md, $PlanDir/progress.md"
