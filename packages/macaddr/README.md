[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# macaddr

Zero-heap pure-CP IEEE 802 MAC-48/EUI-48 (MacAddr6) and EUI-64 (MacAddr8) hardware address parser, formatter, and RFC 4291 converter (crates.io macaddr equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:macaddr` ([https://github.com/svartalf/rust-macaddr](https://github.com/svartalf/rust-macaddr))

| Metadata | Value |
| --- | --- |
| Package | `macaddr` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add macaddr
```

```cp
import { MacAddr6, MacAddr8, macParseHexByte } from "macaddr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MacAddr6` | `class` | Fields: `bytes: [6]u8` · Methods: `new, fromArray, nil, broadcast, parse, isNil, +10 more` |
| `MacAddr8` | `class` | Fields: `bytes: [8]u8` · Methods: `fromArray, nil, broadcast, fromMac6Eui64, parse, isNil, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `macParseHexByte` | `(hi_ch: u8, lo_ch: u8): u8?` | Invokes `macParseHexByte` and returns `u8?`. |

## Minimal Example

```cp
import { macParseHexByte } from "macaddr";

pub fn main() {
    let result = macParseHexByte(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
