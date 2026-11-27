[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bowser

Zero-allocation User-Agent parser for browser, OS, engine, and platform detection (npm bowser parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`bowser`](https://github.com/bowser-js/bowser)（`MIT`）（包名 `bowser`）。

| Metadata | Value |
| --- | --- |
| Package | `bowser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bowser
```

```cp
import {
    PlatformType,
    ParsedUA,
    BwsPair,
    bwsIsVersionChar,
    bwsFindSubstrCI,
    bwsContains,
    bwsContainsCI,
    bwsExtractVersionAt,
} from "bowser";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `PlatformType` | `Desktop, Mobile, Tablet, Bot, Unknown` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedUA` | `class` | Fields: `browser_name: string, browser_version: string, os_name: string, os_version: string, engine_name: string, platform: PlatformType` |
| `BwsPair` | `class` | Fields: `name: string, version: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bwsIsVersionChar` | `(c: u8): bool` | Invokes `bwsIsVersionChar` and returns `bool`. |
| `bwsFindSubstrCI` | `(haystack: string, needle: string): usize?` | Invokes `bwsFindSubstrCI` and returns `usize?`. |
| `bwsContains` | `(haystack: string, needle: string): bool` | Invokes `bwsContains` and returns `bool`. |
| `bwsContainsCI` | `(haystack: string, needle: string): bool` | Invokes `bwsContainsCI` and returns `bool`. |
| `bwsExtractVersionAt` | `(ua: string, start: usize): string` | Invokes `bwsExtractVersionAt` and returns `string`. |
| `bwsExtractAfter` | `(ua: string, token: string): string` | Invokes `bwsExtractAfter` and returns `string`. |
| `bwsDetectBrowser` | `(ua: string): BwsPair` | Invokes `bwsDetectBrowser` and returns `BwsPair`. |
| `bwsDetectOS` | `(ua: string): BwsPair` | Invokes `bwsDetectOS` and returns `BwsPair`. |
| `bwsDetectEngine` | `(ua: string): string` | Invokes `bwsDetectEngine` and returns `string`. |
| `bwsDetectPlatform` | `(ua: string, os_name: string): PlatformType` | Invokes `bwsDetectPlatform` and returns `PlatformType`. |
| `parseUserAgent` | `(ua: string): ParsedUA` | Parses input via `parseUserAgent` and returns `ParsedUA`. |

## Minimal Example

```cp
import { parseUserAgent } from "bowser";

pub fn main() {
    let result = parseUserAgent("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
