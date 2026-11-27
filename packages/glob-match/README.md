[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `glob-match`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:glob-match` ([https://github.com/devongovett/glob-match](https://github.com/devongovett/glob-match))

Zero-allocation, anti-backtracking path glob and brace alternation matching engine for CP (clean-room MIT, inspired by `glob-match` v0.2).

## Features

- **Zero Heap Allocations**: All matching, brace alternation, and capture extraction run purely on stack slices.
- **Anti-Backtracking O(n·m) Core**: Uses an iterative dual-anchor (`*` and `**`) state machine immune to catastrophic exponential backtracking (e.g., `a*a*a*a*a*a*b`).
- **Full Glob Syntax**:
  - `*`: single-segment wildcard (does not cross `/`)
  - `**`: cross-directory wildcard (matches zero or more path segments, including `**/file`, `dir/**/file`, `dir/**`)
  - `?`: single non-`/` character
  - `[abc]`, `[a-z]`, `[!0-9]`, `[^0-9]`: character classes (never match `/`)
  - `{rs,cp,zig}`: single-level brace alternation (multiple brace groups supported)
  - `\x`: backslash escape for literal characters (`\*`, `\?`, `\[`, `\{`, `\\`)
- **Wildcard Captures**: `globMatchCaptures` extracts exact byte `[start, end)` spans for `*`, `**`, and `?`.

## API

```cp
pub struct GlobSpan {
    pub start: usize,
    pub end: usize,
}

pub fn globMatch(pattern: string, path: string): bool
pub fn globMatchAny(patterns: []const string, path: string): bool
pub fn globMatchCaptures(pattern: string, path: string, out_caps: []GlobSpan): usize
pub fn isStaticGlob(pattern: string): bool
```

## Usage

```cp
import {
    GlobSpan,
    globMatch,
    globMatchAny,
    globMatchCaptures,
    isStaticGlob,
} from "glob-match";

fn example() {
    const ok = globMatch("src/**/*.{cp,zig}", "src/net/http/server.cp"); // true

    let caps: [4]GlobSpan = undefined;
    const n = globMatchCaptures("pkg/*/v?.cp", "pkg/core/v2.cp", caps[0..4]);
    // n == 2, caps[0] => "core", caps[1] => "2"

    const static_path = isStaticGlob("src/main.cp"); // true
    const dynamic_path = isStaticGlob("src/*.cp");   // false
}
```

## License

MIT
