# `resolve-from` — AI Agent Quick Reference

- **Package**: `resolve-from` (`1.0.0`, MIT)
- **Origin**: `npm:resolve-from`
- **Import**: `import { resolveFrom, resolveFromSilent, resolveFromWithOptions, ResolveOptions, ResolveResult, isRelativeSpecifier, isScopedSpecifier, isBareSpecifier } from "resolve-from";`

## Types

```cp
class ResolveResult {
    found: bool,
    len: usize,
    err_code: i32, // 0=OK, 1=EMPTY_INPUT, 2=NOT_FOUND, 3=BUFFER_TOO_SMALL
}
class ResolveOptions {
    extensions: [8]string,
    ext_count: usize,
    include_node_modules: bool,
    include_cap_modules: bool,
    fn defaults(): ResolveOptions,
}
```

## Core Signatures

```cp
fn resolveFrom(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?
fn resolveFromSilent(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?
fn resolveFromWithOptions(from_dir: string, module_id: string, existing_files: []string, opts: ResolveOptions, out: []u8): ResolveResult
fn isRelativeSpecifier(spec: string): bool
fn isScopedSpecifier(spec: string): bool
fn isBareSpecifier(spec: string): bool
fn rfExists(path: string, files: []string): bool
fn rfProbeBase(base: string, files: []string, out: []u8): usize?
fn rfProbeBaseOpts(base: string, files: []string, opts: ResolveOptions, out: []u8): ResolveResult
fn rfResolveRelativeBase(from_dir: string, rel: string, out: []u8): usize
```

## Minimal Pattern

```cp
let out: [256]u8 = undefined;
let opts = ResolveOptions.defaults();
let res = resolveFromWithOptions("/app/src", "pad", existing_files, opts, out[0..256]);
if (res.found) {
    let path = out[0..res.len] as string;
}
```

## Pitfalls

1. **No ambient FS I/O**: Caller must pass `existing_files: []string` (virtual file table).
2. **Path length limit**: Internal candidate scratch buffers are 256 bytes; keep resolved paths `< 256` bytes.
3. **Optional unwrap**: `resolveFrom` / `resolveFromSilent` return `usize?` (`null` on miss); prefer `resolveFromWithOptions` when you need `err_code`.
4. **Max 8 extensions**: `ResolveOptions.extensions` holds up to 8 entries controlled by `ext_count`.
