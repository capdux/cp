# `cap:code128` — Zero-Allocation ISO/IEC 15417 Code 128 Barcode Codec

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

Pure CP, zero-heap-allocation **ISO/IEC 15417 Code 128** linear barcode library supporting all 107 standard bar/space patterns, Mod-103 checksum calculation, Code 128B / 128C / Auto (B+C) encoders, `0`/`1` module-stream rendering, and direct symbol/module decoding.

## Constants

- Start/Stop & Control Symbols: `CODE128_START_A (103)`, `CODE128_START_B (104)`, `CODE128_START_C (105)`, `CODE128_STOP (106)`, `CODE128_FNC1 (102)`, `CODE128_CODE_C (99)`, `CODE128_CODE_B (100)`, `CODE128_CODE_A (101)`, `CODE128_SHIFT (98)`
- Module Widths: `CODE128_MOD_PER_SYMBOL (11)`, `CODE128_STOP_MODULES (13)`

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `code128Pattern(symbol: u8): u16?` | Returns MSB-first bar/space bitmask (`0..105`: 11-bit, `106` Stop: 13-bit `0x18EB`). |
| `code128SymbolFromPattern(pat11: u16): u8?` | Reverse-looks up an 11-bit pattern into symbol value `0..105`. |
| `code128Checksum(symbols: []const u8): u8?` | Computes ISO/IEC 15417 Mod-103 checksum over `[Start, data...]`. |
| `encodeCode128B(text: []const u8, out_symbols: *[]u8): usize?` | Encodes ASCII `0x20..0x7F` into Code 128B symbol stream with Mod-103 and Stop. |
| `encodeCode128C(digits: []const u8, out_symbols: *[]u8): usize?` | Encodes even-length digits into compact Code 128C pairs (`00..99`). |
| `encodeCode128Auto(text: []const u8, out_symbols: *[]u8): usize?` | Auto-switches between Code B and Code C (compressing >= 4 consecutive digits). |
| `renderCode128Modules(symbols: []const u8, out_modules: *[]u8): usize?` | Renders validated symbol stream to `0`/`1` module bytes (`(len - 1) * 11 + 13`). |
| `decodeCode128Symbols(symbols: []const u8, out_text: *[]u8): usize?` | Validates Start/Mod-103/Stop and decodes A/B/C symbol streams back to text. |
| `decodeCode128Modules(modules: []const u8, out_text: *[]u8): usize?` | Decodes `0`/`1` module stream directly to text with O(1) stack memory. |

## Example

```cp
import {
    encodeCode128Auto,
    renderCode128Modules,
    decodeCode128Modules,
} from "cap:codec";

fn roundTrip(text: []const u8, syms: *[]u8, mods: *[]u8, out: *[]u8) {
    if (sn := encodeCode128Auto(text, syms)) {
        if (mn := renderCode128Modules(syms.*[0..sn], mods)) {
            * = decodeCode128Modules(mods.*[0..mn], out);
        }
    }
}
```
