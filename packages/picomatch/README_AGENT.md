# picomatch@1.0.0 — AI Agent Quick Ref

- **Import**: `import { PicoOptions, isMatch, isMatchWithOptions, isMatchAny, scanBase } from "picomatch";`
- **Provenance**: npm `picomatch` v4 parity | Zero-heap | License: MIT
- **Deps**: `normalize-path`, `is-extglob`
- **Perf (ReleaseFast)**: `isMatch` (47 B path vs `src/**/codegen/*.cp`) = **2.91M ops/s** (343.4 ns/op, 16-lane SIMD)
- **Types**: `src/index.d.cp`

## Core Types & Functions
```cp
class PicoOptions {
    dot: bool,        // match dotfiles (default: false)
    nocase: bool,     // case-insensitive ASCII (default: false)
    basename: bool,   // match basename only when pattern has no '/' (default: false)
    noglobstar: bool, // disable '**' multi-segment matching (default: false)
    fn defaults(): PicoOptions;
}

export fn isMatch(path: string, pattern: string): bool;
export fn isMatchWithOptions(path: string, pattern: string, opts: PicoOptions): bool;
export fn isMatchAny(path: string, patterns: &[]string, opts: PicoOptions): bool;
export fn scanBase(pattern: string, out: []u8): usize;
```

## Supported Glob Syntax
- Wildcards: `*` (single segment), `?` (single non-`/` char), `**` (globstar, zero or more segments)
- Char classes: `[abc]`, `[a-z]`, `[!0-9]`, `[^0-9]`
- Braces: `{a,b,c}` (nested braces supported)
- Negation: leading `!pattern` (odd count negates, even count double-negates)
- Windows backslashes are normalized automatically via `normalize-path`.

## Minimal Example
```cp
import { PicoOptions, isMatch, isMatchAny, scanBase } from "picomatch";

const ok = isMatch("src/a/b/emit.cp", "src/**/*.cp"); // true
let opts = PicoOptions.defaults();
opts.dot = true;
const pats = [_]string{ "*.cp", ".env" };
const hit = isMatchAny(".env", pats[0..], opts);      // true
let base: [128]u8 = undefined;
const n = scanBase("packages/net/**/*.cp", base[0..]); // "packages/net"
```

## Gotchas
1. Dotfiles (e.g., `.env`, `.git/config`) are **not** matched by `*` or `**` unless `opts.dot = true` or pattern explicitly starts with `.`.
2. `scanBase(pattern, out)` writes into caller-owned `out: []u8` and returns length written (clamped to `out.len`).
3. Prefer `PicoOptions.defaults()` then mutate needed fields rather than relying on struct zero-values.
4. Internal helpers prefixed with `pm*` are implementation details; stick to `isMatch`, `isMatchWithOptions`, `isMatchAny`, `scanBase`.
