[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/semver`

Unified zero-heap SemVer 2.0.0 engine for CP, combining **100% of npm `semver` (`node-semver`)** and **100% of Rust `crates.io/semver` (`Version` / `VersionReq`)** on top of a single shared zero-copy parser and range evaluator.

## Highlights
- **Zero-heap**: fixed-size stack buffers, zero dynamic allocations.
- **Rust Cargo `semver` API**: `Version`, `VersionReq` (`parse`, `matches`, `matchesStr`), `parseVersion`, `compareVersions`, `isPrerelease`, `matchesVersionReq`, `isCaretCompatible`, `formatVersion`, `bumpMajor`/`bumpMinor`/`bumpPatch`.
- **npm `semver` API**: `SemVer`, `parseSemver`, `compareSemver`, `satisfies`, `valid`, `compare`, `gt`/`gte`/`lt`/`lte`/`eq`/`neq`, `major`/`minor`/`patch`, `clean`, `inc`, `maxSatisfying`, `minSatisfying`, `incMajor`/`incMinor`/`incPatch`, `formatSemver`.

## Usage
```cp
import { Version, VersionReq, satisfies, gt, inc, maxSatisfying } from "@cap/semver";

// Rust Cargo style
const v = Version.parse("1.4.2");
const req = VersionReq.parse(">=1.2.0, <2.0.0");
const ok = req.matches(v); // true

// npm node-semver style
const s = satisfies("1.5.0", "^1.2.0 || ~2.0.0"); // true
const higher = gt("2.0.0", "1.9.9"); // true
let out: [32]u8 = undefined;
const n = inc("1.2.3", "minor", out[0..]); // "1.3.0"
```
