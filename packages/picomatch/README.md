[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `picomatch`

Zero-allocation glob matcher & static base scanner for CP — clean-room parity with **npm `picomatch` v4**, supporting globstar (`**`), character classes (`[a-z]`, `[!0-9]`), brace expansion (`{a,b}`), dotfile protection, and leading negation (`!`).

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `npm:picomatch` ([micromatch/picomatch](https://github.com/micromatch/picomatch), v4 parity) |
| **CP Package** | `picomatch@1.0.0` (`MIT`) |
| **Dependencies** | `normalize-path@1.0.0`, `is-extglob@1.0.0` |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm picomatch)
import picomatch from "picomatch";
const isMatch = picomatch("src/**/codegen/*.ts");
isMatch("src/a/b/codegen/emit.ts"); // true
```

```cp
// CP — identical package name, zero-allocation matching
import { isMatch } from "picomatch";

const ok = isMatch("src/a/b/codegen/emit.cp", "src/**/codegen/*.cp"); // true
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Architecture |
| :--- | :--- | --: | --: | :--- |
| `isMatch` | 47 B path vs `src/**/codegen/*.cp` | **343.42 ns/op** | **2,911,885 ops/s** | 16-lane SIMD segment match, zero-alloc |

- **Zero heap allocations**: No regex compilation or AST heap allocation; recursive segment matcher runs completely on the stack with depth bounds.
- **16-lane SIMD scanning**: Fast delimiter and literal segment scanning via `cap:ascii`.

## Examples

### 1. Basic Glob Matching (Globstar, Braces, Character Classes, Negation)

```cp
import { isMatch } from "picomatch";

fn demoBasic() {
    const a = isMatch("src/core/codegen/emit.cp", "src/**/codegen/*.cp"); // true
    const b = isMatch("app.tsx", "*.{ts,tsx,cp}");                         // true
    const c = isMatch("file2.log", "file[0-9].log");                       // true
    const d = isMatch("src/main.cp", "!*.md");                             // true
    const hidden = isMatch(".env", "*");                                   // false (dotfile protected)
    _ = a; _ = b; _ = c; _ = d; _ = hidden;
}
```

### 2. Custom `PicoOptions` & Multi-Pattern Matching (`isMatchAny`)

```cp
import { PicoOptions, isMatchWithOptions, isMatchAny } from "picomatch";

fn demoOptions() {
    let opts = PicoOptions.defaults();
    opts.dot = true;     // allow matching dotfiles like .env
    opts.nocase = true;  // case-insensitive ASCII matching

    const matched_dot = isMatchWithOptions(".github/workflows/ci.YML", "**/*.yml", opts); // true

    const patterns = [_]string{ "src/**/*.cp", "lib/**/*.cp", "cap.json" };
    const any_hit = isMatchAny("lib/net/socket.cp", patterns[0..], PicoOptions.defaults()); // true
    _ = matched_dot; _ = any_hit;
}
```

### 3. Edge Cases: Static Base Extraction (`scanBase`) & Basename Mode

```cp
import { PicoOptions, isMatchWithOptions, scanBase } from "picomatch";

fn demoEdgeCases() {
    // Extract static directory prefix before first glob token for fast fs traversal
    let base_buf: [128]u8 = undefined;
    const n = scanBase("packages/net/src/**/*.cp", base_buf[0..]);
    // base_buf[0..n] == "packages/net/src"

    // Match against filename only when pattern has no slashes
    let opts = PicoOptions.defaults();
    opts.basename = true;
    const hit = isMatchWithOptions("a/b/c/README.md", "*.md", opts); // true

    // Disable globstar (** treated as single-segment *)
    let no_gs = PicoOptions.defaults();
    no_gs.noglobstar = true;
    const flat = isMatchWithOptions("a/b/c.cp", "**/*.cp", no_gs); // false
    _ = n; _ = hit; _ = flat;
}
```

## API Overview

- `PicoOptions` (`dot`, `nocase`, `basename`, `noglobstar`, `PicoOptions.defaults()`)
- `isMatch(path, pattern)`
- `isMatchWithOptions(path, pattern, opts)`
- `isMatchAny(path, patterns, opts)`
- `scanBase(pattern, out)`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
