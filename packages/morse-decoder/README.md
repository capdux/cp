[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# morse-decoder

Zero-heap ITU-R M.1677 International Morse Code encoder and decoder supporting A-Z, 0-9, punctuation, and '/' word separators.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:morse-decoder` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:morse` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `morse-decoder` → CP 包名 `morse-decoder`）

| Metadata | Value |
| --- | --- |
| Package | `morse-decoder` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add morse-decoder
```

```cp
import { encodeChar, decodeSymbol, encodeText, decodeText } from "morse-decoder";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodeChar` | `(c: u8): string` | Serializes/formats data via `encodeChar` returning `string`. |
| `decodeSymbol` | `(sym: string): u8` | Parses input via `decodeSymbol` and returns `u8`. |
| `encodeText` | `(input: string, out: []u8): usize` | Serializes/formats data via `encodeText` returning `usize`. |
| `decodeText` | `(morse: string, out: []u8): usize` | Parses input via `decodeText` and returns `usize`. |

## Minimal Example

```cp
import { decodeSymbol } from "morse-decoder";

pub fn main() {
    let result = decodeSymbol("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
