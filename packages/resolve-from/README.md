[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `resolve-from`

> **Source Attribution**: `npm:resolve-from` ([sindresorhus/resolve-from](https://github.com/sindresorhus/resolve-from)) — License: MIT

Zero-heap Node.js / CP module specifier resolver from a base directory, supporting relative (`./`, `../`), absolute (`/`), bare, and scoped (`@scope/pkg`) specifiers with ancestor `node_modules` / `cap_modules` probing and configurable extensions.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`resolve-from`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: resolve-from)
import resolveFrom from "resolve-from";
const hit = resolveFrom.silent("/app/src", "./utils");
```

```cp
// CP (zero-heap, pure virtual/in-memory file table)
import { resolveFrom, resolveFromSilent } from "resolve-from";

let files: [2]string = ["/app/src/utils.cp", "/app/cap_modules/pad/index.cp"];
let out: [256]u8 = undefined;
let n_opt = resolveFromSilent("/app/src", "./utils", files[0..2], out[0..256]);
if (n_opt != null) {
    let resolved = out[0..(n_opt ?? 0)] as string; // "/app/src/utils.cp"
}
```

## Installation

Add `resolve-from` to your `cap.json`:

```json
{
  "dependencies": {
    "resolve-from": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `ResolveResult` | `class { found: bool, len: usize, err_code: i32 }` | Structured resolution result (`0`=OK, `1`=EMPTY, `2`=NOT_FOUND, `3`=BUF_TOO_SMALL) |
| `ResolveOptions` | `class { extensions: [8]string, ext_count: usize, include_node_modules: bool, include_cap_modules: bool, fn defaults(): ResolveOptions }` | Configurable extension list and module directory toggles |
| `resolveFrom` | `(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?` | Resolve specifier from `from_dir`; returns written length or `null` |
| `resolveFromSilent` | `(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?` | Alias matching npm `resolveFrom.silent` |
| `resolveFromWithOptions` | `(from_dir: string, module_id: string, existing_files: []string, opts: ResolveOptions, out: []u8): ResolveResult` | Resolve with custom extensions & module dir flags, returning error codes |
| `isRelativeSpecifier` | `(spec: string): bool` | True for `.`, `..`, `./*`, `../*` |
| `isScopedSpecifier` | `(spec: string): bool` | True for `@scope/name` specifiers |
| `isBareSpecifier` | `(spec: string): bool` | True for non-empty, non-relative, non-absolute specifiers |
| `rfExists` / `rfProbeBase` / `rfProbeBaseOpts` / `rfResolveRelativeBase` | Low-level helpers | Direct probing and relative base normalization helpers |

## Practical Examples

### 1. Resolving Relative, Bare, and Scoped Packages

```cp
import { resolveFrom, isRelativeSpecifier, isScopedSpecifier, isBareSpecifier } from "resolve-from";

let files: [3]string = [
    "/repo/apps/web/src/helper.cp",
    "/repo/node_modules/@acme/ui/index.js",
    "/repo/cap_modules/pad/index.cp",
];
let out: [256]u8 = undefined;

let r1 = resolveFrom("/repo/apps/web/src", "./helper", files[0..3], out[0..256]);
let r2 = resolveFrom("/repo/apps/web/src", "@acme/ui", files[0..3], out[0..256]);
let r3 = resolveFrom("/repo/apps/web/src", "pad", files[0..3], out[0..256]);

let rel = isRelativeSpecifier("../shared"); // true
let scp = isScopedSpecifier("@acme/ui");    // true
let bare = isBareSpecifier("pad");          // true
```

### 2. Custom Extensions & Structured Error Codes

```cp
import { ResolveOptions, resolveFromWithOptions } from "resolve-from";

let files: [1]string = ["/repo/cap_modules/foo/index.cp"];
let opts = ResolveOptions.defaults();
opts.include_node_modules = false;
opts.include_cap_modules = true;

let out: [256]u8 = undefined;
let res = resolveFromWithOptions("/repo/src", "foo", files[0..1], opts, out[0..256]);
if (res.found) {
    let resolved = out[0..res.len] as string;
}
```

## Notes & Edge Cases

- **Pure & deterministic**: Takes `existing_files: []string` instead of performing ambient filesystem I/O, making it ideal for compilers, bundlers, and sandboxes.
- **Default extensions**: `ResolveOptions.defaults()` probes `""`, `".cp"`, `".ts"`, `".js"`, `".json"`, `"/index.cp"`, `"/index.ts"`, `"/index.js"`.
- **Buffer bounds**: `resolveFromWithOptions` explicitly reports `err_code == 3` (`BUFFER_TOO_SMALL`) when `out` cannot hold the matched path.
