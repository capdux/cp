[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sfv

Zero-allocation W3C User-Agent Client Hints (Sec-CH-UA*) RFC 8941 structured header parser and formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sfv` ([https://github.com/undef1nd/sfv](https://github.com/undef1nd/sfv)) / `npm:ua-client-hints` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `sfv` → CP 包名 `sfv`）

| Metadata | Value |
| --- | --- |
| Package | `sfv` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sfv
```

```cp
import {
    BrandVersion,
    uahToLower,
    uahIsOws,
    uahTrimOws,
    uahEqIgnoreCase,
    uahFindIgnoreCaseFrom,
    uahScanQuoted,
    uahSkipToNextItem,
} from "sfv";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BrandVersion` | `class` | Fields: `brand: string, version: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uahToLower` | `(c: u8): u8` | Invokes `uahToLower` and returns `u8`. |
| `uahIsOws` | `(c: u8): bool` | Invokes `uahIsOws` and returns `bool`. |
| `uahTrimOws` | `(s: &[]u8): &[]u8` | Invokes `uahTrimOws` and returns `&[]u8`. |
| `uahEqIgnoreCase` | `(a: &[]u8, b: &[]u8): bool` | Invokes `uahEqIgnoreCase` and returns `bool`. |
| `uahFindIgnoreCaseFrom` | `(haystack: &[]u8, needle: &[]u8, start_idx: usize): usize` | Invokes `uahFindIgnoreCaseFrom` and returns `usize`. |
| `uahScanQuoted` | `(header: &[]u8, pos: &mut usize, innerStart: &mut usize, innerEnd: &mut usize): bool` | Invokes `uahScanQuoted` and returns `bool`. |
| `uahSkipToNextItem` | `(header: &[]u8, pos: &mut usize)` | Executes `uahSkipToNextItem` in-place. |
| `uahParseNextBrand` | `(header: &[]u8, pos: &mut usize, item: &mut BrandVersion): bool` | Invokes `uahParseNextBrand` and returns `bool`. |
| `parseBrands` | `(header: string, out: []BrandVersion): usize` | Parses input via `parseBrands` and returns `usize`. |
| `isGreaseBrand` | `(brand: string): bool` | Checks predicate `isGreaseBrand` and returns `bool`. |
| `findBrandVersion` | `(header: string, brand: string): string` | Queries/retrieves data via `findBrandVersion` returning `string`. |
| `parseMobile` | `(header: string): i32` | Parses input via `parseMobile` and returns `i32`. |
| `parsePlatform` | `(header: string): string` | Parses input via `parsePlatform` and returns `string`. |
| `uahQuotedEncodedLen` | `(s: &[]u8): usize` | Invokes `uahQuotedEncodedLen` and returns `usize`. |
| `uahWriteQuoted` | `(out: []u8, pos: &mut usize, s: &[]u8)` | Executes `uahWriteQuoted` in-place. |
| `formatBrands` | `(brands: &[]BrandVersion, out: []u8): usize` | Serializes/formats data via `formatBrands` returning `usize`. |

## Minimal Example

```cp
import { parseBrands } from "sfv";

pub fn main() {
    let result = parseBrands("example", /* out: []BrandVersion */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
