[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# short-uuid

Zero-allocation 128-bit UUID to 22-char Flickr Base58 / Base62 / Cookie Base90 lossless short ID translator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:short-uuid` ([https://github.com/oculus42/short-uuid](https://github.com/oculus42/short-uuid))

| Metadata | Value |
| --- | --- |
| Package | `short-uuid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add short-uuid
```

```cp
import {
    parseHexNibble,
    hexNibbleLower,
    uuidToBytes,
    bytesToUuid,
    validateAlphabet,
    findInAlphabet,
    maxEncodedLen,
    shortenUuidWith,
} from "short-uuid";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseHexNibble` | `(c: u8): u8?` | Parses input via `parseHexNibble` and returns `u8?`. |
| `hexNibbleLower` | `(v: u8): u8` | Invokes `hexNibbleLower` and returns `u8`. |
| `uuidToBytes` | `(uuid: string, out16: []u8): bool` | Invokes `uuidToBytes` and returns `bool`. |
| `bytesToUuid` | `(bytes16: []u8, out36: []u8): string` | Invokes `bytesToUuid` and returns `string`. |
| `validateAlphabet` | `(alphabet: string): bool` | Validates input with `validateAlphabet` returning `bool`. |
| `findInAlphabet` | `(alphabet: string, c: u8): u32?` | Queries/retrieves data via `findInAlphabet` returning `u32?`. |
| `maxEncodedLen` | `(base: u32): usize` | Invokes `maxEncodedLen` and returns `usize`. |
| `shortenUuidWith` | `(uuid: string, alphabet: string, out: []u8): string?` | Invokes `shortenUuidWith` and returns `string?`. |
| `expandUuidWith` | `(short_id: string, alphabet: string, out36: []u8): string?` | Invokes `expandUuidWith` and returns `string?`. |
| `shortenUuid` | `(uuid: string, out: []u8): string?` | Invokes `shortenUuid` and returns `string?`. |
| `expandUuid` | `(short_id: string, out36: []u8): string?` | Invokes `expandUuid` and returns `string?`. |

## Minimal Example

```cp
import { validateAlphabet } from "short-uuid";

pub fn main() {
    let result = validateAlphabet("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
