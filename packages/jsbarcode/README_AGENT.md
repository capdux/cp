# jsbarcode@1.0.0 (Agent Quickref)

> Zero-allocation 1D barcode encoder & SVG renderer (JsBarcode / barcoders parity: EAN-13, UPC-A, Code 128B, Code 39 with checksum verification and crisp SVG output) for CP | License: MIT
> Import: `import { ... } from "jsbarcode";` | Full types: `src/index.d.cp`

## Functions
- `export fn ean13Checksum(digits12: string): i8;`
- `export fn isValidEan13(code: string): bool;`
- `export fn upcAChecksum(digits11: string): i8;`
- `export fn isValidUpcA(code: string): bool;`
- `export fn encodeEan13(code: string): Barcode;`
- `export fn encodeUpcA(code: string): Barcode;`
- `export fn encodeCode128B(text: string): Barcode;`
- `export fn encodeCode39(text: string): Barcode;`
