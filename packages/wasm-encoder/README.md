[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wasm-encoder

Zero-allocation WebAssembly 1.0 binary module encoder with LEB128 and standard section builders (Bytecode Alliance wasm-encoder parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wasm-encoder` ([https://github.com/bytecodealliance/wasm-tools](https://github.com/bytecodealliance/wasm-tools))

| Metadata | Value |
| --- | --- |
| Package | `wasm-encoder` |
| Version | `1.0.0` |
| License | `Apache-2.0 WITH LLVM-exception` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wasm-encoder
```

```cp
import {
    ValType,
    ExportKind,
    FuncType,
    MemType,
    ExportEntry,
    FuncBody,
    WasmModuleBuilder,
    wencValTypeByte,
} from "wasm-encoder";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ValType` | `I32, I64, F32, F64` |
| `enum` | `ExportKind` | `Func, Table, Mem, Global` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FuncType` | `class` | Fields: `params: [8]ValType, param_count: usize, results: [4]ValType, result_count: usize` · Methods: `create` |
| `MemType` | `class` | Fields: `min_pages: u32, has_max: bool, max_pages: u32` · Methods: `unbounded, bounded` |
| `ExportEntry` | `class` | Fields: `name: string, kind: ExportKind, index: u32` |
| `FuncBody` | `class` | Fields: `local_counts: [8]u32, local_types: [8]ValType, local_group_count: usize, code: &[]u8` · Methods: `simple, addLocals` |
| `WasmModuleBuilder` | `class` | Fields: `buf: [4096]u8, len: usize, overflow: bool` · Methods: `init, writeSection, addTypeSection, addFunctionSection, addMemorySection, addExportSection, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wencValTypeByte` | `(vt: ValType): u8` | Invokes `wencValTypeByte` and returns `u8`. |
| `wencExportKindByte` | `(kind: ExportKind): u8` | Invokes `wencExportKindByte` and returns `u8`. |
| `wencLow7Signed` | `(v: i64): u8` | Invokes `wencLow7Signed` and returns `u8`. |
| `encodeUleb128` | `(val: u64, out: []u8): usize` | Serializes/formats data via `encodeUleb128` returning `usize`. |
| `encodeSleb128` | `(val: i64, out: []u8): usize` | Serializes/formats data via `encodeSleb128` returning `usize`. |
| `wencAppendByte` | `(dst: []u8, pos: usize, b: u8): usize?` | Invokes `wencAppendByte` and returns `usize?`. |
| `wencAppendBytes` | `(dst: []u8, pos: usize, src: &[]u8): usize?` | Invokes `wencAppendBytes` and returns `usize?`. |
| `wencAppendUleb` | `(dst: []u8, pos: usize, val: u64): usize?` | Invokes `wencAppendUleb` and returns `usize?`. |

## Minimal Example

```cp
import { encodeUleb128 } from "wasm-encoder";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUleb128(0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
