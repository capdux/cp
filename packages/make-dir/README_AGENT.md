# `make-dir` — AI Agent Quick Reference

- **Package**: `make-dir` (`1.0.0`, MIT)
- **Origin**: `npm:make-dir`
- **Import**: `import { MakeDirOptions, MakeDirPlan, DirPathValidation, ParentDirIter, planMakeDir, makeDirPlan, isValidDirPath, validateDirPath, normalizeDirPath, applyUmask, iterParentDirs, collectParentDirs } from "make-dir";`

## Types

```cp
class MakeDirOptions { mode: u32, is_windows: bool, fn defaults(): MakeDirOptions }
class MakeDirPlan { valid: bool, mode: u32, dirs: [32]string, count: usize, fn empty(valid: bool, mode: u32): MakeDirPlan }
class DirPathValidation { valid: bool, err_code: i32, bad_index: usize } // 0=OK, 1=EMPTY, 2=NUL, 3=BAD_CHAR, 4=BAD_COLON
class ParentDirIter { path: string, cursor: usize, done: bool, fn next(_: auto): string? }
```

## Core Signatures

```cp
fn planMakeDir(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan
fn makeDirPlan(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan
fn isValidDirPath(path: string, is_windows: bool): bool
fn validateDirPath(path: string, is_windows: bool): DirPathValidation
fn normalizeDirPath(path: string, is_windows: bool, out: []u8): usize
fn applyUmask(mode: u32, umask: u32): u32
fn iterParentDirs(path: string): ParentDirIter
fn collectParentDirs(path: string, out: []string): usize
```

## Minimal Pattern

```cp
let opts = MakeDirOptions.defaults();
opts.mode = applyUmask(0o777, 0o022);
let plan = planMakeDir("/a/b/c", existing_dirs, opts);
if (plan.valid) {
    // iterate plan.dirs[0..plan.count]
}
```

## Pitfalls

1. **No filesystem syscalls**: `planMakeDir` / `makeDirPlan` are pure planners; pass `existing_dirs: []string` and create `plan.dirs[0..plan.count]` via your FS layer.
2. **Segment cap**: `MakeDirPlan.dirs` holds at most 32 missing prefixes.
3. **Separator normalization**: `planMakeDir` splits on `/`; for Windows backslash inputs, call `normalizeDirPath(path, true, buf)` first.
4. **Zero-copy slices**: `plan.dirs[i]` and `ParentDirIter.next()` slice directly into `target` / `path`.
