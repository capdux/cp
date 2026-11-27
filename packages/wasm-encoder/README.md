[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# wasm-encoder

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wasm-encoder` ([https://github.com/bytecodealliance/wasm-tools](https://github.com/bytecodealliance/wasm-tools))

Zero-allocation WebAssembly 1.0 binary module encoder with unsigned/signed LEB128 and standard section builders for CP — clean-room parity with Bytecode Alliance `wasm-encoder` (248M+ downloads on crates.io).

纯 CP 零堆分配 WebAssembly 1.0 二进制模块编码器，内置完整 ULEB128/SLEB128 编码以及 Type、Function、Memory、Export、Code、Custom 段构造器，可直接生成合法 `.wasm` 二进制字节流。

## Install / 安装

```bash
cap add wasm-encoder
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ValType` | `enum: u8 { I32 = 0x7F, I64 = 0x7E, F32 = 0x7D, F64 = 0x7C }` | WebAssembly numeric value types |
| `ExportKind` | `enum: u8 { Func = 0, Table = 1, Mem = 2, Global = 3 }` | WebAssembly export descriptor kinds |
| `encodeUleb128` | `(val: u64, out: []u8) -> usize` | Unsigned LEB128 encoder |
| `encodeSleb128` | `(val: i64, out: []u8) -> usize` | Signed LEB128 encoder |
| `FuncType` | `struct` + `FuncType.create(params, results)` | Function signature `(params) -> (results)` |
| `MemType` | `struct` + `MemType.unbounded(min)` / `MemType.bounded(min, max)` | Linear memory limits |
| `ExportEntry` | `struct { name: string, kind: ExportKind, index: u32 }` | Export section entry |
| `FuncBody` | `struct` + `FuncBody.simple(code)` / `addLocals(*_, count, vt)` | Function body with optional locals |
| `WasmModuleBuilder` | `init()`, `addTypeSection`, `addFunctionSection`, `addMemorySection`, `addExportSection`, `addCodeSection`, `addCustomSection`, `finish(out)` | 4096-byte static buffer `.wasm` encoder |

## Usage / 使用示例

```cp
import {
    ValType, ExportKind, FuncType, ExportEntry, FuncBody, WasmModuleBuilder,
    OP_LOCAL_GET, OP_I32_ADD, OP_END,
} from "wasm-encoder";

let m = WasmModuleBuilder.init();
const params: [2]ValType = [ValType.I32, ValType.I32];
const results: [1]ValType = [ValType.I32];
const types: [1]FuncType = [FuncType.create(params[0..2], results[0..1])];
* = m.addTypeSection(types[0..1]);

const funcs: [1]u32 = [0];
* = m.addFunctionSection(funcs[0..1]);

const exports: [1]ExportEntry = [ExportEntry{ name: "add", kind: ExportKind.Func, index: 0 }];
* = m.addExportSection(exports[0..1]);

const code: [6]u8 = [OP_LOCAL_GET, 0, OP_LOCAL_GET, 1, OP_I32_ADD, OP_END];
const bodies: [1]FuncBody = [FuncBody.simple(code[0..6])];
* = m.addCodeSection(bodies[0..1]);

let out: [256]u8 = undefined;
const n = m.finish(out[0..256]);
```
