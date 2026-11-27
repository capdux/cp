[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# string_decoder

Zero-allocation streaming multi-byte UTF-8/UTF-16/Latin1/Base64 string decoder preserving incomplete boundary bytes across chunks (clean-room MIT implementation of npm string_decoder, #62 / 378M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string_decoder` ([https://github.com/nodejs/string_decoder](https://github.com/nodejs/string_decoder)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `string_decoder` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add string_decoder
```

```cp
import {
    DecoderEncoding,
    StringDecoder,
    sdecUtf8ExpectedLen,
    sdecPushReplacement,
    sdecPushCodePointUtf8,
} from "string_decoder";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `DecoderEncoding` | `Utf8, Latin1, Utf16Le` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StringDecoder` | `class` | Fields: `encoding: DecoderEncoding, last_char: [4]u8, last_need: usize, last_total: usize, high_surrogate: u16` · Methods: `new, utf8, pendingBytes, write, writeUtf16Le, emitUtf16Unit, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sdecUtf8ExpectedLen` | `(lead: u8): usize` | Invokes `sdecUtf8ExpectedLen` and returns `usize`. |
| `sdecPushReplacement` | `(out: []u8, pos: usize): usize` | Invokes `sdecPushReplacement` and returns `usize`. |
| `sdecPushCodePointUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | Invokes `sdecPushCodePointUtf8` and returns `usize`. |

## Minimal Example

```cp
import { sdecUtf8ExpectedLen } from "string_decoder";

pub fn main() {
    let result = sdecUtf8ExpectedLen(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
