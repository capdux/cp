# `normalize-path` — AI Agent Quick Reference

- **Package**: `normalize-path` (`1.0.0`, MIT)
- **Origin**: `npm:normalize-path`
- **Import**: `import { normalizePath, normalizePathDefault, normalizePathWith, NormalizePathOptions, toPosixPath, stripTrailingSlash, isUncPath, isWindowsDrivePath, hasBackslash } from "normalize-path";`

## Types

```cp
class NormalizePathOptions {
    strip_trailing: bool,
    resolve_dots: bool,
    fn defaults(): NormalizePathOptions,
}
```

## Core Signatures

```cp
fn normalizePath(path: string, strip_trailing: bool, out: []u8): usize
fn normalizePathDefault(path: string, out: []u8): usize
fn normalizePathWith(path: string, opts: NormalizePathOptions, out: []u8): usize
fn toPosixPath(path: string, out: []u8): usize
fn stripTrailingSlash(path: string): string
fn hasBackslash(path: string): bool
fn isUncPath(path: string): bool
fn isWindowsDrivePath(path: string): bool
fn npIsSlash(ch: u8): bool
fn npIsAsciiAlpha(ch: u8): bool
```

## Minimal Pattern

```cp
let out: [256]u8 = undefined;
let opts = NormalizePathOptions.defaults();
opts.resolve_dots = true;
let n = normalizePathWith("a\\b\\..\\c/", opts, out[0..256]);
let normalized = out[0..n] as string; // "a/c"
```

## Pitfalls

1. **Output buffer size**: `out.len >= path.len` is always sufficient because normalization never expands path length.
2. **Dot resolution is opt-in**: `normalizePath` / `normalizePathDefault` do not resolve `..`; use `normalizePathWith` with `opts.resolve_dots = true`.
3. **UNC prefix preserved**: `\\host\share` becomes `//host/share` (leading `//` kept).
4. **Zero-copy helper**: `stripTrailingSlash` returns a slice of `path` and preserves root `"/"`.
