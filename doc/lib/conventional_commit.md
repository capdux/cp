# `cap:conventional_commit` — Conventional Commits 1.0.0 Parser & Bump Advisor

Zero-allocation Conventional Commits 1.0.0 commit message parser supporting `<type>[(scope)][!]: <description>`, `BREAKING CHANGE:` footer detection, and SemVer bump level recommendation.

## Exports

- `ReleaseBump` (`none` | `patch` | `minor` | `major`)
- `ConventionalCommit` — `parse(msg): ConventionalCommit?`, `recommendedBump(): ReleaseBump`, `isFeature(): bool`, `isFix(): bool`.
