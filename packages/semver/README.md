[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `semver`

Unified zero-heap SemVer 2.0.0 parser, comparator, npm range evaluator, and Rust Cargo `VersionReq` matcher for CP — a clean-room superset of both **npm `semver`** and **crates.io `semver`**.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registries** | `npm:semver` ([npm/node-semver](https://github.com/npm/node-semver), v7 parity) & `crates.io:semver` ([dtolnay/semver](https://github.com/dtolnay/semver), v1 parity) |
| **CP Package** | `semver@1.0.0` (`MIT`) |
| **Dependencies** | None (stdlib `cap:ascii`, `cap:fmt` only) |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm semver)
import semver from "semver";
semver.satisfies("1.5.0", "^1.2.0 || ~2.0.0");
semver.gt("2.0.0", "1.9.9");
```

```rust
// Rust (crates.io semver)
use semver::{Version, VersionReq};
let v = Version::parse("1.4.2")?;
let req = VersionReq::parse(">=1.2.0, <2.0.0")?;
req.matches(&v);
```

```cp
// CP — identical package name, both APIs unified
import { Version, VersionReq, satisfies, gt } from "semver";

const v = Version.parse("1.4.2");
const req = VersionReq.parse(">=1.2.0, <2.0.0");
const ok = req.matches(v);                              // true
const in_range = satisfies("1.5.0", "^1.2.0 || ~2.0.0"); // true
const higher = gt("2.0.0", "1.9.9");                     // true
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Architecture |
| :--- | :--- | --: | --: | :--- |
| `parseSemver` + `compareSemver` | 2× full prerelease + build | **156.40 ns/op** | **6,393,994 ops/s** | 16-lane SIMD digit/ident scan, zero-heap |

- **Zero heap allocations**: `SemVer` / `Version` stores `pre: [32]u8` and `build: [32]u8` inline on the stack.
- **16-lane SIMD scanning**: Accelerated identifier and digit validation with zero stack-clear overhead.

## Examples

### 1. Basic Parsing, Comparison & npm Ranges

```cp
import { parseSemver, compareSemver, satisfies, valid, gt, major, minor, patch } from "semver";

fn demoBasic() {
    const a = parseSemver("1.2.3-alpha.1+build.42");
    const b = parseSemver("1.2.3-alpha.2");
    if (a.valid && b.valid) {
        const ord = compareSemver(a, b); // -1 (a < b)
        const is_pre = a.isPrerelease(); // true
        _ = ord;
        _ = is_pre;
    }

    const ok = satisfies("1.5.0", "^1.2.0 || ~2.0.0"); // true
    const is_valid = valid("1.0.0-rc.1");              // true
    const newer = gt("2.0.0", "1.9.9");                // true
    const maj = major("3.4.5") orelse 0;               // 3
    const min = minor("3.4.5") orelse 0;               // 4
    const pat = patch("3.4.5") orelse 0;               // 5
    _ = ok; _ = is_valid; _ = newer; _ = maj; _ = min; _ = pat;
}
```

### 2. Rust Cargo `VersionReq`, Bumping & Formatting

```cp
import { Version, VersionReq, bumpMinor, formatVersion, matchesVersionReq, isCaretCompatible } from "semver";

fn demoCargoStyle() {
    const req = VersionReq.parse(">=1.2.0, <2.0.0");
    if (req.valid) {
        const matched = req.matchesStr("1.8.3"); // true
        _ = matched;
    }

    const v = Version.parse("1.4.2");
    const next = bumpMinor(v); // 1.5.0
    let out: [64]u8 = undefined;
    const n = formatVersion(next, out[0..]); // out[0..n] == "1.5.0"

    const quick = matchesVersionReq("1.4.2", "^1.2"); // true
    const compat = isCaretCompatible("1.4.2", "1.2.0"); // true
    _ = n; _ = quick; _ = compat;
}
```

### 3. Edge Cases: `clean`, `inc`, `maxSatisfying`, Invalid Input Handling

```cp
import { parseSemver, clean, inc, maxSatisfying, minSatisfying } from "semver";

fn demoEdgeCases() {
    // Invalid version returns SemVer with valid == false (no panic/allocation)
    const bad = parseSemver("not-a-version");
    if (!bad.valid) {
        // handle invalid input
    }

    // Caller-owned stack buffer for clean() and inc() — returns 0 on error or insufficient buffer
    let buf: [64]u8 = undefined;
    const clean_len = clean("  =v1.2.3  ", buf[0..]); // "1.2.3"
    const inc_len = inc("1.2.3", "minor", buf[0..]);  // "1.3.0"

    // Select best matching version index (-1 if none satisfy)
    const candidates = [_]string{ "1.0.0", "1.4.2", "1.9.0", "2.0.0" };
    const max_idx = maxSatisfying(candidates[0..], "^1.0.0"); // 2 ("1.9.0")
    const min_idx = minSatisfying(candidates[0..], "^1.0.0"); // 0 ("1.0.0")
    _ = clean_len; _ = inc_len; _ = max_idx; _ = min_idx;
}
```

## API Overview

- **Core Types**: `SemVer` (aliased as `Version`), `VersionReq`
- **npm API**: `parseSemver`, `compareSemver`, `satisfies`, `valid`, `compare`, `gt`, `gte`, `lt`, `lte`, `eq`, `neq`, `major`, `minor`, `patch`, `clean`, `inc`, `maxSatisfying`, `minSatisfying`, `incMajor`, `incMinor`, `incPatch`, `formatSemver`
- **Cargo API**: `Version.parse`, `VersionReq.parse`, `parseVersion`, `compareVersions`, `isPrerelease`, `matchesVersionReq`, `isCaretCompatible`, `formatVersion`, `bumpMajor`, `bumpMinor`, `bumpPatch`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
