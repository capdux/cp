[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# serve-static

Zero-heap HTTP static file request resolver, dotfile policy evaluator, Cache-Control builder, and MIME/ETag helper (clean-room port of npm serve-static)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:serve-static` ([https://github.com/expressjs/serve-static](https://github.com/expressjs/serve-static))

| Metadata | Value |
| --- | --- |
| Package | `serve-static` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add serve-static
```

```cp
import {
    ServeStaticOptions,
    StaticResolution,
    sstPush,
    sstIsGetOrHead,
    hasDotfileSegment,
    resolveStaticRequest,
    formatCacheControl,
    formatStaticEtag,
} from "serve-static";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ServeStaticOptions` | `class` | Fields: `index_file: string, dotfiles: u8, max_age_secs: u64, immutable: bool, redirect_dirs: bool` · Methods: `defaultOpts` |
| `StaticResolution` | `class` | Fields: `status: u16, rel_path_len: usize, mime_type: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sstPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `sstPush` and returns `usize`. |
| `sstIsGetOrHead` | `(method: string): bool` | Invokes `sstIsGetOrHead` and returns `bool`. |
| `hasDotfileSegment` | `(pathname: string): bool` | Checks predicate `hasDotfileSegment` and returns `bool`. |
| `resolveStaticRequest` | `(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution` | Invokes `resolveStaticRequest` and returns `StaticResolution`. |
| `formatCacheControl` | `(max_age_secs: u64, immutable: bool, out: []u8): usize` | Serializes/formats data via `formatCacheControl` returning `usize`. |
| `formatStaticEtag` | `(size_bytes: u64, mtime_ms: u64, out: []u8): usize` | Serializes/formats data via `formatStaticEtag` returning `usize`. |
| `formatStaticContentType` | `(file_path: string, out: []u8): usize` | Serializes/formats data via `formatStaticContentType` returning `usize`. |

## Minimal Example

```cp
import { formatStaticContentType } from "serve-static";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatStaticContentType("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
