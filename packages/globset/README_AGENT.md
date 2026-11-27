# globset — AI Agent Quick Reference

- **Package**: `globset` (`v1.0.0`) | **Source**: crates.io `globset` v0.4 | **Import**: `import { ... } from "globset";`
- **Role**: Zero-heap multi-glob matcher (`GlobSet`, max 32 patterns) & `!`-negation override engine (`GlobOverride`).

## Core Exports (`src/index.d.cp`)

```cp
// Constants: GLOBSET_MAX_PATTERNS=32, OVERRIDE_NONE=0, OVERRIDE_WHITELIST=1, OVERRIDE_IGNORE=2
class GlobSetOptions {
    case_insensitive: bool, literal_separator: bool, backslash_escape: bool,
    fn defaults(): GlobSetOptions;
}

class GlobSet {
    fn new(): GlobSet;
    fn withOptions(opts: GlobSetOptions): GlobSet;
    fn add(_: auto, pattern: string): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn patternAt(idx: usize): string;
    fn isMatch(path: string): bool;
    fn firstMatch(path: string): i32;
    fn matches(path: string, out_indices: []usize): usize;
    fn matchCount(path: string): usize;
}

class GlobOverride {
    fn new(): GlobOverride;
    fn withOptions(opts: GlobSetOptions): GlobOverride;
    fn add(_: auto, rule: string): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn matched(path: string): i32;
    fn isAllowed(path: string): bool;
}
```

## Minimal Usage

```cp
import { GlobSet, GlobOverride } from "globset";

let set = GlobSet.new();
set.add("src/**/*.cp");
const hit = set.isMatch("src/app.cp"); // true

let ov = GlobOverride.new();
ov.add("src/**");
ov.add("!src/gen/**");
const ok = ov.isAllowed("src/gen/a.cp"); // false
```

## Pitfalls & Constraints

1. **Capacity limit**: Max `32` patterns per `GlobSet` / `GlobOverride`. `add()` returns `false` when full, empty, or `"!"` alone.
2. **Last-match-wins in `GlobOverride`**: Later rules override earlier ones. If **any** positive whitelist rule was added, unmatched paths return `isAllowed == false`; if **only** `!` ignore rules exist, unmatched paths return `isAllowed == true`.
3. **Path normalization**: Paths `<= 1024` bytes are automatically normalized via `normalize-path` before matching.
4. **Private helpers**: `gs*` functions in `index.d.cp` are internal helpers — use `GlobSet`, `GlobOverride`, `GlobSetOptions`.
