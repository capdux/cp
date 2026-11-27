[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ws

Zero-heap RFC 6455 WebSocket binary frame header encoder/decoder, XOR masking, and close status codec for CP (compatible with npm ws & Rust tungstenite)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ws` ([https://github.com/websockets/ws](https://github.com/websockets/ws))（原库名 `ws` → CP 包名 `ws`）

| Metadata | Value |
| --- | --- |
| Package | `ws` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ws
```

```cp
import {
    FrameHeader,
    isControlOpcode,
    isValidOpcode,
    applyMask,
    decodeHeader,
    encodeHeader,
    encodeFrame,
    encodeClosePayload,
} from "ws";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FrameHeader` | `class` | Fields: `fin: bool, rsv: u8, opcode: u8, masked: bool, mask_key: [4]u8, payload_len: u64, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isControlOpcode` | `(opcode: u8): bool` | Checks predicate `isControlOpcode` and returns `bool`. |
| `isValidOpcode` | `(opcode: u8): bool` | Checks predicate `isValidOpcode` and returns `bool`. |
| `applyMask` | `(payload: []u8, mask_key: [4]u8)` | Executes `applyMask` in-place. |
| `decodeHeader` | `(data: &[]u8, out: &mut FrameHeader): i32` | Parses input via `decodeHeader` and returns `i32`. |
| `encodeHeader` | `(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload_len: u64, out: []u8): usize` | Serializes/formats data via `encodeHeader` returning `usize`. |
| `encodeFrame` | `(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload: &[]u8, out: []u8): usize` | Serializes/formats data via `encodeFrame` returning `usize`. |
| `encodeClosePayload` | `(code: u16, reason: string, out: []u8): usize` | Serializes/formats data via `encodeClosePayload` returning `usize`. |
| `isValidCloseCode` | `(code: u16): bool` | Checks predicate `isValidCloseCode` and returns `bool`. |

## Minimal Example

```cp
import { encodeClosePayload } from "ws";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeClosePayload(0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
