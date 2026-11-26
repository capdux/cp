# @cap/barcode

Zero-allocation 1D barcode encoder & SVG renderer for CP — clean-room MIT parity with npm `jsbarcode` and Rust `barcoders`.

## Install

```bash
cap add @cap/barcode
```

## Supported Symbologies

- **EAN-13** (`encodeEan13`, `ean13Checksum`, `isValidEan13`): GS1 13-digit retail standard (95 modules, auto-computes check digit from 12 digits).
- **UPC-A** (`encodeUpcA`, `upcAChecksum`, `isValidUpcA`): North American 12-digit retail standard.
- **Code 128B** (`encodeCode128B`): High-density logistics & warehouse standard supporting full printable ASCII (32–126) with Mod-103 checksum.
- **Code 39** (`encodeCode39`): Alphanumeric industrial barcode (`0-9`, `A-Z`, `- . $ / + % space`).
- **Output Renderers**: `renderSvg(bar_width, height, quiet_zone, out)` & `renderAscii(out)`.

## Usage

```cp
import { encodeEan13, encodeCode128B } from "@cap/barcode";

const bc = encodeEan13("4006381333931");
let svg_buf: [8192]u8 = undefined;
const svg = bc.renderSvg(2, 80, 10, svg_buf[0..8192]);
```
