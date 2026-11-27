# wasm-encoder@1.0.0 (Agent Quickref)

> Zero-allocation WebAssembly 1.0 binary module encoder with LEB128 and standard section builders (Bytecode Alliance wasm-encoder parity) for CP | License: Apache-2.0 WITH LLVM-exception
> Import: `import { ... } from "wasm-encoder";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ValType`
- `export enum ExportKind`

## Functions
- `export fn wencValTypeByte(vt: ValType): u8;`
- `export fn wencExportKindByte(kind: ExportKind): u8;`
- `export fn wencLow7Signed(v: i64): u8;`
- `export fn encodeUleb128(val: u64, out: []u8): usize;`
- `export fn encodeSleb128(val: i64, out: []u8): usize;`
- `export fn wencAppendByte(dst: []u8, pos: usize, b: u8): usize?;`
- `export fn wencAppendBytes(dst: []u8, pos: usize, src: &[]u8): usize?;`
- `export fn wencAppendUleb(dst: []u8, pos: usize, val: u64): usize?;`
