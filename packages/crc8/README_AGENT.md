# crc8 — AI Agent Quick Ref

Zero-allocation CRC-8 checksum library (SMBus, Maxim 1-Wire, SAE-J1850, AUTOSAR, CDMA2000, Bluetooth HEC) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Crc8Hasher,
    reflect8,
    crc8Smbus,
    crc8Maxim,
    crc8SaeJ1850,
    crc8Autosar,
    crc8Cdma2000,
    crc8Bluetooth,
} from "crc8";
```

## Core Types & Constants

```cp
class Crc8Hasher { table: [256]u8, state: u8, init_val: u8, ref_in: bool, /* +2 fields */ } // methods: initCustom(...) +9
```

## API Signatures

```cp
fn reflect8(b: u8): u8;
fn crc8Smbus(data: &[]u8): u8;
fn crc8Maxim(data: &[]u8): u8;
fn crc8SaeJ1850(data: &[]u8): u8;
fn crc8Autosar(data: &[]u8): u8;
fn crc8Cdma2000(data: &[]u8): u8;
fn crc8Bluetooth(data: &[]u8): u8;
```

## Minimal Usage

```cp
import { reflect8 } from "crc8";

pub fn main() {
    let result = reflect8(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crc8"`; install via `cap add crc8`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
