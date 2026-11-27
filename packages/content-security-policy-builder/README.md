[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# content-security-policy-builder

Zero-allocation W3C Content-Security-Policy Level 3 HTTP header builder, validator, and zero-copy policy inspector for CP.

Aligned with npm [`content-security-policy-builder`](https://github.com/helmetjs/content-security-policy-builder) (MIT).

## Installation & Import

```bash
cap add content-security-policy-builder
```

```cp
import {
    CspBuilder,
    CspDirectiveSpan,
    CspPolicy,
} from "content-security-policy-builder";
```

## Core API

### `CspBuilder` Class

| Member | Signature | Description |
| --- | --- | --- |
| Fields | `buf: []u8`, `pos: usize`, `count: usize`, `overflow: bool` | Caller-owned output buffer, current write offset, directive count, and overflow flag. |
| `CspBuilder.init` | `(buf: []u8): CspBuilder` | Creates a CSP header builder backed by `buf`. |
| `addDirective` | `(*self, name: &[]u8, sources: &[]&[]u8): bool` | Validates directive name and sources (rejecting header-injection characters such as `;`, `,`, CR/LF), appends `name src1 src2` separated by `; `, and returns `false` on invalid input or buffer overflow. |
| `written` | `(): &[]u8` | Returns immutable byte slice of the built CSP header. |
| `len` | `(): usize` | Returns current written byte length. |
| `isOverflow` | `(): bool` | Returns `true` if any `addDirective` call exceeded `buf.len`. |

### `CspPolicy` & `CspDirectiveSpan`

| Member | Signature | Description |
| --- | --- | --- |
| `CspDirectiveSpan` | `{ name_start, name_len, val_start, val_len: usize }` | Zero-copy byte-span offsets for a single parsed directive. |
| `CspPolicy.parse` | `(header: &[]u8): CspPolicy` | Parses up to 32 directives from a CSP header string without heap allocation. |
| `len` | `(): usize` | Returns number of parsed directives. |
| `getDirective` | `(header: &[]u8, name: &[]u8): &[]u8` | Case-insensitively looks up directive `name` and returns its value slice (empty slice if missing). |
| `hasDirective` | `(header: &[]u8, name: &[]u8): bool` | Returns `true` if directive `name` is present (case-insensitive). |
| `allowsUnsafeInline` | `(header: &[]u8, directive: &[]u8): bool` | Checks whether `directive` (falling back to `default-src`) contains `'unsafe-inline'`. |

## Examples

### 1. Building a Strict CSP Header

```cp
import { print } from "cap:io";
import { CspBuilder } from "content-security-policy-builder";

pub fn main() {
    let buf: [512]u8 = undefined;
    let b = CspBuilder.init(buf[0..512]);
    let def_srcs: [1][]const u8 = ["'self'"];
    let img_srcs: [2][]const u8 = ["'self'", "https://cdn.example.com"];

    let _ = b.addDirective("default-src", def_srcs[0..1]);
    let _ = b.addDirective("img-src", img_srcs[0..2]);

    if (!b.isOverflow()) {
        print("Content-Security-Policy: {}\n", b.written());
    }
}
```

### 2. Parsing & Auditing an Incoming CSP Header

```cp
import { print } from "cap:io";
import { CspPolicy } from "content-security-policy-builder";

pub fn main() {
    let header = "default-src 'self'; script-src 'self' 'unsafe-inline'";
    let policy = CspPolicy.parse(header);
    print("directives={}\n", policy.len());
    print("script-src={}\n", policy.getDirective(header, "script-src"));
    print("allows unsafe-inline={}\n", policy.allowsUnsafeInline(header, "script-src"));
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: Builds headers directly into a caller-provided `[]u8` buffer and parses up to 32 directives on the stack (`directives: [32]CspDirectiveSpan`).
- **Built-in parser & security auditor**: Extends npm `content-security-policy-builder` with `CspPolicy.parse`, `getDirective`, `hasDirective`, and `allowsUnsafeInline`.
- **Malformed header rejection**: `addDirective` validates directive names (ASCII alphanumeric + `-`) and rejects control characters, `;`, and `,` in source values to prevent header injection.
