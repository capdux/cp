[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jsbarcode

Zero-allocation 1D barcode encoder & SVG renderer (JsBarcode / barcoders parity: EAN-13, UPC-A, Code 128B, Code 39 with checksum verification and crisp SVG output) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsbarcode` ([https://github.com/lindell/JsBarcode](https://github.com/lindell/JsBarcode))（原库名 `jsbarcode` → CP 包名 `jsbarcode`）

| Metadata | Value |
| --- | --- |
| Package | `jsbarcode` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jsbarcode
```

```cp
import {
    Barcode,
    ean13Checksum,
    isValidEan13,
    upcAChecksum,
    isValidUpcA,
    eanLCode,
    eanGCode,
    eanRCode,
} from "jsbarcode";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Barcode` | `class` | Fields: `modules: [MAX_MODULES]u8, len: usize, ok: bool` · Methods: `empty, pushBit, pushBits, pushWidths, bitAt, renderAscii, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ean13Checksum` | `(digits12: string): i8` | Invokes `ean13Checksum` and returns `i8`. |
| `isValidEan13` | `(code: string): bool` | Checks predicate `isValidEan13` and returns `bool`. |
| `upcAChecksum` | `(digits11: string): i8` | Invokes `upcAChecksum` and returns `i8`. |
| `isValidUpcA` | `(code: string): bool` | Checks predicate `isValidUpcA` and returns `bool`. |
| `eanLCode` | `(d: u8): string` | Invokes `eanLCode` and returns `string`. |
| `eanGCode` | `(d: u8): string` | Invokes `eanGCode` and returns `string`. |
| `eanRCode` | `(d: u8): string` | Invokes `eanRCode` and returns `string`. |
| `eanParityPattern` | `(first_digit: u8): string` | Invokes `eanParityPattern` and returns `string`. |
| `encodeEan13` | `(code: string): Barcode` | Serializes/formats data via `encodeEan13` returning `Barcode`. |
| `encodeUpcA` | `(code: string): Barcode` | Serializes/formats data via `encodeUpcA` returning `Barcode`. |
| `code128Pattern` | `(val: usize): string` | Invokes `code128Pattern` and returns `string`. |
| `encodeCode128B` | `(text: string): Barcode` | Serializes/formats data via `encodeCode128B` returning `Barcode`. |
| `code39Pattern` | `(c: u8): string` | Invokes `code39Pattern` and returns `string`. |
| `encodeCode39` | `(text: string): Barcode` | Serializes/formats data via `encodeCode39` returning `Barcode`. |

## Minimal Example

```cp
import { isValidEan13 } from "jsbarcode";

pub fn main() {
    let result = isValidEan13("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
