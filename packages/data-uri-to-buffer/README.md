[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# data-uri-to-buffer

Zero-allocation RFC 2397 Data URI parser, Base64/percent decoder, and encoder for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:data-uri-to-buffer` ([https://github.com/TooTallNate/proxy-agents](https://github.com/TooTallNate/proxy-agents))（原库名 `data-uri-to-buffer` → CP 包名 `data-uri-to-buffer`）

| Metadata | Value |
| --- | --- |
| Package | `data-uri-to-buffer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add data-uri-to-buffer
```

```cp
import {
    DataUri,
    appendByte,
    appendStr,
    isUnreservedUriByte,
    parseDataUri,
    isDataUri,
    b64Val,
    decodeBase64Payload,
} from "data-uri-to-buffer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DataUri` | `class` | Fields: `ok: bool, media_type: string, charset: string, is_base64: bool, raw_payload: string` · Methods: `invalid` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `isUnreservedUriByte` | `(c: u8): bool` | Checks predicate `isUnreservedUriByte` and returns `bool`. |
| `parseDataUri` | `(uri: string): DataUri` | Parses input via `parseDataUri` and returns `DataUri`. |
| `isDataUri` | `(s: string): bool` | Checks predicate `isDataUri` and returns `bool`. |
| `b64Val` | `(c: u8): u8?` | Invokes `b64Val` and returns `u8?`. |
| `decodeBase64Payload` | `(payload: string, out: []u8): ([]u8)?` | Parses input via `decodeBase64Payload` and returns `([]u8)?`. |
| `decodePercentPayload` | `(payload: string, out: []u8): ([]u8)?` | Parses input via `decodePercentPayload` and returns `([]u8)?`. |
| `decodeDataUri` | `(uri: string, out: []u8): ([]u8)?` | Parses input via `decodeDataUri` and returns `([]u8)?`. |
| `encodeDataUriBase64` | `(media_type: string, payload: []u8, out: []u8): string` | Serializes/formats data via `encodeDataUriBase64` returning `string`. |
| `encodeDataUriText` | `(media_type: string, text: string, out: []u8): string` | Serializes/formats data via `encodeDataUriText` returning `string`. |

## Minimal Example

```cp
import { parseDataUri } from "data-uri-to-buffer";

pub fn main() {
    let result = parseDataUri("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
