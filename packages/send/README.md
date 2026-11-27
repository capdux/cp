[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# send

Zero-heap pure CP static file transfer planner with traversal protection, dotfiles policy, ETag/304, Range/206, and redirect URL encoding (clean-room MIT implementation of npm send v1)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:send` ([https://github.com/pillarjs/send](https://github.com/pillarjs/send))

| Metadata | Value |
| --- | --- |
| Package | `send` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add send
```

```cp
import {
    SendOptions,
    SendPlan,
    sndStrEq,
    sndIsWs,
    sndTrimWs,
    sndHasMaliciousPath,
    sndContainsDotfile,
    sndIfNoneMatchMatches,
} from "send";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SendOptions` | `class` | Fields: `root: string, dotfiles: u8, etag: bool, accept_ranges: bool, max_age_secs: u64, immutable: bool` · Methods: `defaults` |
| `SendPlan` | `class` | Fields: `status: u16, content_type: string, is_partial: bool, range_start: u64, range_end: u64, content_length: u64, +1 more` · Methods: `errorPlan` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sndStrEq` | `(a: string, b: string): bool` | Invokes `sndStrEq` and returns `bool`. |
| `sndIsWs` | `(b: u8): bool` | Invokes `sndIsWs` and returns `bool`. |
| `sndTrimWs` | `(s: string): string` | Invokes `sndTrimWs` and returns `string`. |
| `sndHasMaliciousPath` | `(req_path: string): bool` | Invokes `sndHasMaliciousPath` and returns `bool`. |
| `sndContainsDotfile` | `(req_path: string): bool` | Invokes `sndContainsDotfile` and returns `bool`. |
| `sndIfNoneMatchMatches` | `(if_none_match: string, current_etag: string): bool` | Invokes `sndIfNoneMatchMatches` and returns `bool`. |
| `planSendFile` | `(req_path: string, file_size: u64, mtime_ms: u64, range_header: string, if_none_match: string, opts: SendOptions, etag_out: []u8): SendPlan` | Invokes `planSendFile` and returns `SendPlan`. |
| `formatRedirectUrl` | `(path: string, out: []u8): usize` | Serializes/formats data via `formatRedirectUrl` returning `usize`. |

## Minimal Example

```cp
import { formatRedirectUrl } from "send";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatRedirectUrl("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
