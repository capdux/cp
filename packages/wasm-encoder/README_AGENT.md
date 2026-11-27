# wasm-encoder — AI Agent Quick Ref

Zero-allocation WebAssembly 1.0 binary module encoder with LEB128 and standard section builders (Bytecode Alliance wasm-encoder parity) for CP (`v1.0.0`, `Apache-2.0 WITH LLVM-exception`)

## Import

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

## Core Types & Constants

```cp
enum ValType { I32, I64, F32, F64 }
enum ExportKind { Func, Table, Mem, Global }
class FuncType { params: [8]ValType, param_count: usize, results: [4]ValType, result_count: usize } // methods: create(...)
class MemType { min_pages: u32, has_max: bool, max_pages: u32 } // methods: unbounded(...) +1
class ExportEntry { name: string, kind: ExportKind, index: u32 }
class FuncBody { local_counts: [8]u32, local_types: [8]ValType, local_group_count: usize, code: &[]u8 } // methods: simple(...) +1
class WasmModuleBuilder { buf: [4096]u8, len: usize, overflow: bool } // methods: init(...) +10
```

## API Signatures

```cp
fn wencValTypeByte(vt: ValType): u8;
fn wencExportKindByte(kind: ExportKind): u8;
fn wencLow7Signed(v: i64): u8;
fn encodeUleb128(val: u64, out: []u8): usize;
fn encodeSleb128(val: i64, out: []u8): usize;
fn wencAppendByte(dst: []u8, pos: usize, b: u8): usize?;
fn wencAppendBytes(dst: []u8, pos: usize, src: &[]u8): usize?;
fn wencAppendUleb(dst: []u8, pos: usize, val: u64): usize?;
```

## Minimal Usage

```cp
import { encodeUleb128 } from "wasm-encoder";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUleb128(0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wasm-encoder"`; install via `cap add wasm-encoder`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
