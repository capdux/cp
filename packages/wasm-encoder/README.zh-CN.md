[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# wasm-encoder

> 版本：`1.0.0` | 许可证：`Apache-2.0 WITH LLVM-exception` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation WebAssembly 1.0 binary module encoder with LEB128 and standard section builders (Bytecode Alliance wasm-encoder parity) for CP

## 安装与导入

```bash
cap add wasm-encoder
```

```cp
import { wencValTypeByte } from "wasm-encoder";
```

## 核心导出 API

- `export enum ValType`
- `export enum ExportKind`
- `export fn wencValTypeByte(vt: ValType): u8;`
- `export fn wencExportKindByte(kind: ExportKind): u8;`
- `export fn wencLow7Signed(v: i64): u8;`
- `export fn encodeUleb128(val: u64, out: []u8): usize;`
- `export fn encodeSleb128(val: i64, out: []u8): usize;`
- `export fn wencAppendByte(dst: []u8, pos: usize, b: u8): usize?;`
- `export fn wencAppendBytes(dst: []u8, pos: usize, src: &[]u8): usize?;`
- `export fn wencAppendUleb(dst: []u8, pos: usize, val: u64): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`wasm-encoder@1.0.0`)，许可证：Apache-2.0 WITH LLVM-exception
