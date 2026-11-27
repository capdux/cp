[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wasm-encoder

Zero-allocation WebAssembly 1.0 binary module encoder with LEB128 and standard section builders (Bytecode Alliance wasm-encoder parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wasm-encoder` ([https://github.com/bytecodealliance/wasm-tools](https://github.com/bytecodealliance/wasm-tools))

| 元信息 | 值 |
| --- | --- |
| 包名 | `wasm-encoder` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0 WITH LLVM-exception` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ValType` | `I32, I64, F32, F64` |
| `enum` | `ExportKind` | `Func, Table, Mem, Global` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FuncType` | `class` | 字段: `params: [8]ValType, param_count: usize, results: [4]ValType, result_count: usize` · 方法: `create` |
| `MemType` | `class` | 字段: `min_pages: u32, has_max: bool, max_pages: u32` · 方法: `unbounded, bounded` |
| `ExportEntry` | `class` | 字段: `name: string, kind: ExportKind, index: u32` |
| `FuncBody` | `class` | 字段: `local_counts: [8]u32, local_types: [8]ValType, local_group_count: usize, code: &[]u8` · 方法: `simple, addLocals` |
| `WasmModuleBuilder` | `class` | 字段: `buf: [4096]u8, len: usize, overflow: bool` · 方法: `init, writeSection, addTypeSection, addFunctionSection, addMemorySection, addExportSection, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wencValTypeByte` | `(vt: ValType): u8` | 调用 `wencValTypeByte` 并返回 `u8`。 |
| `wencExportKindByte` | `(kind: ExportKind): u8` | 调用 `wencExportKindByte` 并返回 `u8`。 |
| `wencLow7Signed` | `(v: i64): u8` | 调用 `wencLow7Signed` 并返回 `u8`。 |
| `encodeUleb128` | `(val: u64, out: []u8): usize` | 通过 `encodeUleb128` 编码或格式化输出，返回 `usize`。 |
| `encodeSleb128` | `(val: i64, out: []u8): usize` | 通过 `encodeSleb128` 编码或格式化输出，返回 `usize`。 |
| `wencAppendByte` | `(dst: []u8, pos: usize, b: u8): usize?` | 调用 `wencAppendByte` 并返回 `usize?`。 |
| `wencAppendBytes` | `(dst: []u8, pos: usize, src: &[]u8): usize?` | 调用 `wencAppendBytes` 并返回 `usize?`。 |
| `wencAppendUleb` | `(dst: []u8, pos: usize, val: u64): usize?` | 调用 `wencAppendUleb` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { encodeUleb128 } from "wasm-encoder";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUleb128(0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
