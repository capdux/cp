[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# html-encoding-sniffer

Zero-allocation WHATWG HTML 13.2.3.2 byte stream encoding sniffer and label normalizer (npm html-encoding-sniffer parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:html-encoding-sniffer` ([https://github.com/jsdom/html-encoding-sniffer](https://github.com/jsdom/html-encoding-sniffer))（原库名 `html-encoding-sniffer` → CP 包名 `html-encoding-sniffer`）

| Metadata | Value |
| --- | --- |
| Package | `html-encoding-sniffer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add html-encoding-sniffer
```

```cp
import {
    SniffOptions,
    hesToLower,
    hesEqIgnoreCase,
    hesIsAsciiWs,
    hesTrimAsciiWs,
    hesStartsWithIgnoreCase,
    normalizeEncodingLabel,
    hesMapMetaEncoding,
} from "html-encoding-sniffer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SniffOptions` | `class` | Fields: `transport_layer_encoding: string, default_encoding: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hesToLower` | `(ch: u8): u8` | Invokes `hesToLower` and returns `u8`. |
| `hesEqIgnoreCase` | `(a: string, b: string): bool` | Invokes `hesEqIgnoreCase` and returns `bool`. |
| `hesIsAsciiWs` | `(ch: u8): bool` | Invokes `hesIsAsciiWs` and returns `bool`. |
| `hesTrimAsciiWs` | `(s: string): string` | Invokes `hesTrimAsciiWs` and returns `string`. |
| `hesStartsWithIgnoreCase` | `(bytes: &[]u8, pos: usize, needle: string): bool` | Invokes `hesStartsWithIgnoreCase` and returns `bool`. |
| `normalizeEncodingLabel` | `(label: string): string` | Invokes `normalizeEncodingLabel` and returns `string`. |
| `hesMapMetaEncoding` | `(label: string): string` | Invokes `hesMapMetaEncoding` and returns `string`. |
| `hesExtractCharsetFromContent` | `(content: string): string` | Invokes `hesExtractCharsetFromContent` and returns `string`. |
| `hesPrescanMeta` | `(bytes: &[]u8): string` | Invokes `hesPrescanMeta` and returns `string`. |
| `sniffHtmlEncoding` | `(bytes: &[]u8, opts: SniffOptions): string` | Invokes `sniffHtmlEncoding` and returns `string`. |

## Minimal Example

```cp
import { hesEqIgnoreCase } from "html-encoding-sniffer";

pub fn main() {
    let result = hesEqIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
