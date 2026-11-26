# @cap/semver-crate

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:semver` ([https://github.com/dtolnay/semver](https://github.com/dtolnay/semver))（原库名 `semver` → CP 包名 `@cap/semver-crate`）

Clean-room MIT, zero-heap Rust Cargo/crates.io SemVer 2.0 `Version` & comma-separated `VersionReq` matcher for CP — modeled on crates.io #42 `semver` v1 (1.55B+ downloads).

## Dependencies
- `@cap/semver-range` (`1.0.0`) — reused for SemVer 2.0 parsing, comparison, range evaluation, and formatting.

## API
- `parseVersion`, `compareVersions`, `isPrerelease`, `formatVersion`
- `matchesVersionReq(version_str, req)` — Cargo comma-separated AND requirements + bare-version caret semantics
- `isCaretCompatible`, `bumpMajor`, `bumpMinor`, `bumpPatch`
