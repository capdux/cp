[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# crc16

Zero-heap industrial CRC-16 checksums (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC/IBM, USB) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc16` ([https://github.com/mrhooray/crc-rs](https://github.com/mrhooray/crc-rs))

| Metadata | Value |
| --- | --- |
| Package | `crc16` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add crc16
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Crc16Hasher` | `class` | Fields: `table: [256]u16, poly: u16, init_val: u16, xor_out: u16, ref_in: bool, ref_out: bool, +1 more` · Methods: `initCustom, initCcittFalse, initXmodem, initModbus, initKermit, initArc, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `reflectU16` | `(v: u16): u16` | Invokes `reflectU16` and returns `u16`. |
| `crc16BuildTable` | `(poly: u16, ref_in: bool): [256]u16` | Invokes `crc16BuildTable` and returns `[256]u16`. |
| `crc16CcittFalse` | `(data: &[]u8): u16` | Invokes `crc16CcittFalse` and returns `u16`. |
| `crc16Xmodem` | `(data: &[]u8): u16` | Invokes `crc16Xmodem` and returns `u16`. |
| `crc16Modbus` | `(data: &[]u8): u16` | Invokes `crc16Modbus` and returns `u16`. |
| `crc16Kermit` | `(data: &[]u8): u16` | Invokes `crc16Kermit` and returns `u16`. |
| `crc16Arc` | `(data: &[]u8): u16` | Invokes `crc16Arc` and returns `u16`. |
| `crc16Usb` | `(data: &[]u8): u16` | Invokes `crc16Usb` and returns `u16`. |

## Minimal Example

```cp
import { reflectU16 } from "crc16";

pub fn main() {
    let result = reflectU16(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
