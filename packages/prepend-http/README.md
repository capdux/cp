[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# prepend-http

Zero-allocation utility to prepend `https://` or `http://` to humanized URLs for CP, matching npm [`prepend-http`](https://github.com/sindresorhus/prepend-http) v4.

## Source Alignment

- **Upstream**: npm `prepend-http` v4 (`sindresorhus/prepend-http`, MIT)
- **CP Package**: `prepend-http` v1.0.0 (clean-room MIT in pure CP)
- **Rules**: Trims leading/trailing whitespace and prepends `https://` (when `https == true`) or `http://` (when `https == false`), leaving URLs untouched if they already have an RFC 3986 `scheme://`, `mailto:`, protocol-relative `//`, or relative path prefix (`./`, `../`, `/`).

## Installation & Import

```bash
cap add prepend-http
```

```cp
import {
    isProtocolRelative,
    hasScheme,
    prependHttp,
} from "prepend-http";
```

## Core API

| Function | Signature | Description |
| :--- | :--- | :--- |
| `prependHttp` | `fn prependHttp(url: string, https: bool, out: []u8): usize` | Trims `url`, prepends `https://` or `http://` when missing a scheme/relative prefix, writes into `out`, and returns bytes written |
| `hasScheme` | `fn hasScheme(url: string): bool` | Returns `true` if trimmed `url` already has an explicit RFC 3986 `scheme://` or case-insensitive `mailto:` prefix |
| `isProtocolRelative` | `fn isProtocolRelative(url: string): bool` | Returns `true` if trimmed `url` starts with protocol-relative `//` |

## Examples

### 1. Prepending `https://` vs `http://`

```cp
import { prependHttp } from "prepend-http";
import { print } from "cap:io";

pub fn main() {
    let buf: [128]u8 = [0; 128];

    const n1 = prependHttp("cp-lang.org", true, buf[0..128]);
    print(buf[0..n1] as string); // "https://cp-lang.org"

    const n2 = prependHttp("localhost:3000", false, buf[0..128]);
    print(buf[0..n2] as string); // "http://localhost:3000"
}
```

### 2. Preserving Existing Schemes & Relative Paths

```cp
import { prependHttp } from "prepend-http";

pub fn normalizeLink(raw: string, out: []u8): usize {
    return prependHttp(raw, true, out);
    // "http://example.com"  -> "http://example.com"
    // "//cdn.example.com"   -> "//cdn.example.com"
    // "mailto:a@b.com"      -> "mailto:a@b.com"
    // "./docs/index.html"   -> "./docs/index.html"
}
```

### 3. Inspecting Before Formatting

```cp
import { hasScheme, isProtocolRelative } from "prepend-http";

pub fn needsProtocol(url: string): bool {
    return !hasScheme(url) && !isProtocolRelative(url);
}
```

## Differences from npm `prepend-http` & Safety Highlights

- **Explicit `https` Parameter**: Instead of an optional options object `{ https: true }`, CP takes an explicit `https: bool` argument for zero-overhead clarity.
- **Zero-Heap Buffer Output**: Writes directly into caller-owned `[]u8` slices; worst-case required buffer size is `url.len + 8` (`"https://"`).
- **No Exceptions**: Empty or whitespace-only inputs return `0` without throwing.
