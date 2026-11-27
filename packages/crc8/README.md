[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# crc8

Zero-allocation CRC-8 checksum library (SMBus, Maxim 1-Wire, SAE-J1850, AUTOSAR, CDMA2000, Bluetooth HEC) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc8` ([https://github.com/mrhooray/crc-rs](https://github.com/mrhooray/crc-rs))

| Metadata | Value |
| --- | --- |
| Package | `crc8` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add crc8
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Crc8Hasher` | `class` | Fields: `table: [256]u8, state: u8, init_val: u8, ref_in: bool, ref_out: bool, xor_out: u8` · Methods: `initCustom, initSmbus, initMaxim, initSaeJ1850, initAutosar, initCdma2000, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `reflect8` | `(b: u8): u8` | Invokes `reflect8` and returns `u8`. |
| `crc8Smbus` | `(data: &[]u8): u8` | Invokes `crc8Smbus` and returns `u8`. |
| `crc8Maxim` | `(data: &[]u8): u8` | Invokes `crc8Maxim` and returns `u8`. |
| `crc8SaeJ1850` | `(data: &[]u8): u8` | Invokes `crc8SaeJ1850` and returns `u8`. |
| `crc8Autosar` | `(data: &[]u8): u8` | Invokes `crc8Autosar` and returns `u8`. |
| `crc8Cdma2000` | `(data: &[]u8): u8` | Invokes `crc8Cdma2000` and returns `u8`. |
| `crc8Bluetooth` | `(data: &[]u8): u8` | Invokes `crc8Bluetooth` and returns `u8`. |

## Minimal Example

```cp
import { reflect8 } from "crc8";

pub fn main() {
    let result = reflect8(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
