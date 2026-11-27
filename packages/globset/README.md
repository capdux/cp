[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# globset

Zero-allocation multi-glob set matcher (`GlobSet`) and whitelist/blacklist override engine (`GlobOverride`) for CP (Rust `globset` v0.4 & `ignore::overrides` parity).

Composes `picomatch` (`scanBase` + `isMatchWithOptions`) and `normalize-path` to match up to 32 glob patterns in one stack-allocated set with static-base prefix prefiltering and last-match-wins `!` negation rules.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `globset` v0.4 (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/BurntSushi/ripgrep` |
| License | MIT |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: globset)
let mut builder = GlobSetBuilder::new();
builder.add(Glob::new("src/**/*.rs")?);
builder.add(Glob::new("*.toml")?);
let set = builder.build()?;
let matched = set.is_match("src/main.rs");
```

```cp
// CP (cap: globset) — zero-allocation multi-glob set + override engine
import { GlobSet, GlobOverride, GlobSetOptions } from "globset";
let set = GlobSet.new();
set.add("src/**/*.cp");
set.add("*.json");
const matched = set.isMatch("src/main.cp");
```

## Installation

```bash
cap add globset
```

## Usage

### Basic Usage: Multi-Pattern Matching & Collecting All Matching Indices

```cp
import { GlobSet, GlobSetOptions } from "globset";

let set = GlobSet.new();
set.add("src/**/*.cp");
set.add("**/*.md");
set.add("src/core/*");

// Boolean check & first matching pattern index (-1 if none)
const ok = set.isMatch("src/core/app.cp");      // true
const first = set.firstMatch("src/core/app.cp"); // 0

// Collect ALL matching pattern indices into a stack buffer
let hits: [32]usize = undefined;
const n = set.matches("src/core/app.cp", hits[0..32]); // n == 2, hits = [0, 2]
const total = set.matchCount("src/core/app.cp");       // 2

// Case-insensitive matching via GlobSetOptions
let opts = GlobSetOptions.defaults();
opts.case_insensitive = true;
let ciSet = GlobSet.withOptions(opts);
ciSet.add("*.README");
const ciHit = ciSet.isMatch("docs/guide.readme"); // true
```

### Advanced & Security Usage: `.gitignore`-Style Whitelist & `!` Ignore Overrides

```cp
import { GlobOverride, OVERRIDE_WHITELIST, OVERRIDE_IGNORE, OVERRIDE_NONE } from "globset";

// Positive rules whitelist; `!`-prefixed rules ignore. Last matching rule wins!
let ov = GlobOverride.new();
ov.add("src/**");          // whitelist everything under src/
ov.add("!src/generated/**"); // ignore generated files
ov.add("src/generated/keep.cp"); // re-whitelist one specific file

const a = ov.isAllowed("src/main.cp");           // true  (OVERRIDE_WHITELIST)
const b = ov.isAllowed("src/generated/tmp.cp");  // false (OVERRIDE_IGNORE)
const c = ov.isAllowed("src/generated/keep.cp"); // true  (last match wins!)
const d = ov.isAllowed("README.md");             // false (unmatched rejected when any whitelist exists)

const status = ov.matched("src/generated/tmp.cp"); // OVERRIDE_IGNORE (2)
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `GlobSetOptions` | `class { case_insensitive, literal_separator, backslash_escape, fn defaults() }` | Matching options (default: case-sensitive, literal `/`, `\` escape). |
| `GlobSet` | `class { new(), withOptions(opts), add(*_, pat): bool, len(), isEmpty(), clear(*_), patternAt(idx) }` | Fixed-capacity (max 32) multi-glob set with static-base pruning. |
| `GlobSet.isMatch` | `fn(path: string): bool` | Returns `true` if any pattern matches `path`. |
| `GlobSet.firstMatch` | `fn(path: string): i32` | Returns 0-based index of first matching pattern, or `-1`. |
| `GlobSet.matches` | `fn(path: string, out_indices: []usize): usize` | Writes all matching 0-based indices into `out_indices`; returns count. |
| `GlobSet.matchCount` | `fn(path: string): usize` | Returns total number of patterns matching `path`. |
| `GlobOverride` | `class { new(), withOptions(opts), add(*_, rule): bool, len(), isEmpty(), clear(*_) }` | Whitelist (`pat`) & ignore (`!pat`) matcher with last-match-wins semantics. |
| `GlobOverride.matched` | `fn(path: string): i32` | Returns `OVERRIDE_NONE` (`0`), `OVERRIDE_WHITELIST` (`1`), or `OVERRIDE_IGNORE` (`2`). |
| `GlobOverride.isAllowed` | `fn(path: string): bool` | Evaluates final allow/deny decision (unmatched paths denied if any whitelist exists). |

## Limits & Performance

- **Static-base prefiltering**: Pre-scans up to 64 bytes of literal prefix per pattern via `picomatch.scanBase` to prune non-matching paths in O(prefix) before running glob matching.
- **Zero heap allocation**: Fixed capacity of `32` patterns (`GLOBSET_MAX_PATTERNS`) and `1024`-byte path normalization buffer.

## License

MIT — Clean-room CP implementation with Rust `globset` v0.4 parity.
