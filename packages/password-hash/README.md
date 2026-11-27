[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# password-hash

Zero-allocation pure CP PHC string format parser, serializer, and constant-time comparator (RustCrypto password-hash parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:password-hash` ([https://github.com/RustCrypto/password-hashes](https://github.com/RustCrypto/password-hashes))

| Metadata | Value |
| --- | --- |
| Package | `password-hash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add password-hash
```

```cp
import {
    PhcParam,
    PhcHash,
    phchEmptyHash,
    phchIsLowerAlnumHyphen,
    phchIsParamValueChar,
    phchIsB64Char,
    phchIsValidAlgo,
    phchIsValidParamKey,
} from "password-hash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PhcParam` | `class` | Fields: `key: string, value: string` |
| `PhcHash` | `class` | Fields: `algorithm: string, version: u32, has_version: bool, params: [8]PhcParam, param_count: usize, salt: string, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `phchEmptyHash` | `(): PhcHash` | Invokes `phchEmptyHash` and returns `PhcHash`. |
| `phchIsLowerAlnumHyphen` | `(c: u8): bool` | Invokes `phchIsLowerAlnumHyphen` and returns `bool`. |
| `phchIsParamValueChar` | `(c: u8): bool` | Invokes `phchIsParamValueChar` and returns `bool`. |
| `phchIsB64Char` | `(c: u8): bool` | Invokes `phchIsB64Char` and returns `bool`. |
| `phchIsValidAlgo` | `(s: string): bool` | Invokes `phchIsValidAlgo` and returns `bool`. |
| `phchIsValidParamKey` | `(s: string): bool` | Invokes `phchIsValidParamKey` and returns `bool`. |
| `phchIsValidParamValue` | `(s: string): bool` | Invokes `phchIsValidParamValue` and returns `bool`. |
| `phchIsValidSalt` | `(s: string): bool` | Invokes `phchIsValidSalt` and returns `bool`. |
| `phchIsValidHash` | `(s: string): bool` | Invokes `phchIsValidHash` and returns `bool`. |
| `phchContainsByte` | `(s: string, target: u8): bool` | Invokes `phchContainsByte` and returns `bool`. |
| `phchParseU32` | `(s: string): i64` | Invokes `phchParseU32` and returns `i64`. |
| `phchStrEq` | `(a: string, b: string): bool` | Invokes `phchStrEq` and returns `bool`. |
| `phchAppendBytes` | `(out: []u8, pos: usize, src: string): usize` | Invokes `phchAppendBytes` and returns `usize`. |
| `phchAppendU32` | `(out: []u8, pos: usize, val: u32): usize` | Invokes `phchAppendU32` and returns `usize`. |
| `parsePhc` | `(phc_str: string): PhcHash` | Parses input via `parsePhc` and returns `PhcHash`. |
| `formatPhc` | `(h: PhcHash, out: []u8): usize` | Serializes/formats data via `formatPhc` returning `usize`. |
| `getPhcParam` | `(h: PhcHash, key: string): string?` | Queries/retrieves data via `getPhcParam` returning `string?`. |
| `ctEqual` | `(a: string, b: string): bool` | Invokes `ctEqual` and returns `bool`. |

## Minimal Example

```cp
import { parsePhc } from "password-hash";

pub fn main() {
    let result = parsePhc("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
