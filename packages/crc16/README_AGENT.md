# crc16 — AI Agent Quick Ref

Zero-heap industrial CRC-16 checksums (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC/IBM, USB) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Crc16Hasher,
    reflectU16,
    crc16BuildTable,
    crc16CcittFalse,
    crc16Xmodem,
    crc16Modbus,
    crc16Kermit,
    crc16Arc,
} from "crc16";
```

## Core Types & Constants

```cp
class Crc16Hasher { table: [256]u16, poly: u16, init_val: u16, xor_out: u16, /* +3 fields */ } // methods: initCustom(...) +9
```

## API Signatures

```cp
fn reflectU16(v: u16): u16;
fn crc16BuildTable(poly: u16, ref_in: bool): [256]u16;
fn crc16CcittFalse(data: &[]u8): u16;
fn crc16Xmodem(data: &[]u8): u16;
fn crc16Modbus(data: &[]u8): u16;
fn crc16Kermit(data: &[]u8): u16;
fn crc16Arc(data: &[]u8): u16;
fn crc16Usb(data: &[]u8): u16;
```

## Minimal Usage

```cp
import { reflectU16 } from "crc16";

pub fn main() {
    let result = reflectU16(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crc16"`; install via `cap add crc16`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
