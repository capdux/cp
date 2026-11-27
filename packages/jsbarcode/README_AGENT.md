# jsbarcode — AI Agent Quick Ref

Zero-allocation 1D barcode encoder & SVG renderer (JsBarcode / barcoders parity: EAN-13, UPC-A, Code 128B, Code 39 with checksum verification and crisp SVG output) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Barcode { modules: [MAX_MODULES]u8, len: usize, ok: bool } // methods: empty(...) +6
```

## API Signatures

```cp
fn ean13Checksum(digits12: string): i8;
fn isValidEan13(code: string): bool;
fn upcAChecksum(digits11: string): i8;
fn isValidUpcA(code: string): bool;
fn eanLCode(d: u8): string;
fn eanGCode(d: u8): string;
fn eanRCode(d: u8): string;
fn eanParityPattern(first_digit: u8): string;
fn encodeEan13(code: string): Barcode;
fn encodeUpcA(code: string): Barcode;
fn code128Pattern(val: usize): string;
fn encodeCode128B(text: string): Barcode;
fn code39Pattern(c: u8): string;
fn encodeCode39(text: string): Barcode;
```

## Minimal Usage

```cp
import { isValidEan13 } from "jsbarcode";

pub fn main() {
    let result = isValidEan13("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jsbarcode"`; install via `cap add jsbarcode`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
