[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `make-dir`

> **Source Attribution**: `npm:make-dir` ([sindresorhus/make-dir](https://github.com/sindresorhus/make-dir)) — License: MIT

Zero-heap recursive directory creation planner (`mkdir -p`) with structured Windows/POSIX path validation, path normalization, octal umask calculation, and parent-directory iteration in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`make-dir`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: make-dir)
import { makeDirectorySync } from "make-dir";
makeDirectorySync("/var/data/cache/v1", { mode: 0o755 });
```

```cp
// CP (zero-heap deterministic creation plan)
import { MakeDirOptions, planMakeDir, applyUmask } from "make-dir";

let existing: [2]string = ["/var", "/var/data"];
let opts = MakeDirOptions.defaults();
opts.mode = applyUmask(0o777, 0o022); // 0o755

let plan = planMakeDir("/var/data/cache/v1", existing[0..2], opts);
// plan.valid == true, plan.count == 2
// plan.dirs[0] == "/var/data/cache", plan.dirs[1] == "/var/data/cache/v1"
```

## Installation

Add `make-dir` to your `cap.json`:

```json
{
  "dependencies": {
    "make-dir": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `MakeDirOptions` | `class { mode: u32, is_windows: bool, fn defaults(): MakeDirOptions }` | Creation mode (default `0o777` / `511`) & Windows rule toggle |
| `MakeDirPlan` | `class { valid: bool, mode: u32, dirs: [32]string, count: usize, fn empty(valid: bool, mode: u32): MakeDirPlan }` | Ordered list of missing directory prefixes to create |
| `DirPathValidation` | `class { valid: bool, err_code: i32, bad_index: usize }` | Structured validation result (`0`=OK, `1`=EMPTY, `2`=NUL, `3`=INVALID_CHAR, `4`=INVALID_COLON) |
| `ParentDirIter` | `class { path: string, cursor: usize, done: bool, fn next(_: auto): string? }` | Zero-allocation iterator over parent directory prefixes |
| `planMakeDir` / `makeDirPlan` | `(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan` | Compute missing directory prefixes in parent-to-child order |
| `isValidDirPath` | `(path: string, is_windows: bool): bool` | Quick boolean check for NUL bytes and Windows forbidden chars |
| `validateDirPath` | `(path: string, is_windows: bool): DirPathValidation` | Detailed validation with `err_code` and `bad_index` |
| `normalizeDirPath` | `(path: string, is_windows: bool, out: []u8): usize` | Normalize separators, collapse `//`, strip trailing `/` (preserving `/` and `C:/`) |
| `applyUmask` | `(mode: u32, umask: u32): u32` | Compute effective octal permission `mode & ~umask` |
| `iterParentDirs` | `(path: string): ParentDirIter` | Create an iterator yielding each parent directory prefix |
| `collectParentDirs` | `(path: string, out: []string): usize` | Collect all parent directory prefixes into `out` |
| `mkdStrEq` / `mkdExists` / `mkdIsAlpha` | Low-level helpers | Internal string and ASCII helpers |

## Practical Examples

### 1. Validating, Normalizing & Planning Recursive Creation

```cp
import {
    MakeDirOptions,
    validateDirPath,
    normalizeDirPath,
    makeDirPlan,
    applyUmask,
} from "make-dir";

let check = validateDirPath("C:\\work\\bad*dir", true);
// check.valid == false, check.err_code == 3, check.bad_index == 11

let norm_buf: [128]u8 = undefined;
let n = normalizeDirPath("C:\\work\\\\out\\bin\\", true, norm_buf[0..128]);
let clean = norm_buf[0..n] as string; // "C:/work/out/bin"

let existing: [1]string = ["C:/work"];
let opts = MakeDirOptions.defaults();
opts.mode = applyUmask(0o777, 0o022);
let plan = makeDirPlan(clean, existing[0..1], opts);
// plan.dirs[0] == "C:/work/out", plan.dirs[1] == "C:/work/out/bin"
```

### 2. Iterating & Collecting Parent Directories

```cp
import { iterParentDirs, collectParentDirs } from "make-dir";

let it = iterParentDirs("/a/b/c/d");
while (true) {
    let p = it.next();
    if (p == null) { break; }
    // yields "/a", "/a/b", "/a/b/c"
}

let parents: [8]string = [""; 8];
let count = collectParentDirs("/a/b/c/d", parents[0..8]); // 3
```

## Notes & Edge Cases

- **Maximum depth**: `MakeDirPlan.dirs` holds up to 32 missing segments per plan.
- **Windows validation**: Rejects `< > " | ? *`, NUL bytes, and `:` anywhere except index `1` (`C:\...`).
- **Deterministic & pure**: Computes the exact creation sequence without side effects so callers can execute `mkdir` syscalls or dry-run previews safely.
