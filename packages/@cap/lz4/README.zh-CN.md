[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/lz4

Extremely fast lossless compression (LZ4 & LZ4-HC) native C binding for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/lz4` |
| 版本 | `1.10.0` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/lz4
```

```cp
import {
    Lz4Error,
    Lz4Buffer,
    LZ4_versionNumber,
    LZ4_compressBound,
    LZ4_compress_default,
    LZ4_compress_fast,
    LZ4_compress_HC,
    LZ4_decompress_safe,
} from "@cap/lz4";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `Lz4Error` | `InputTooLarge, OutputTooSmall, CompressFailed, DecompressFailed, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Lz4Buffer` | `class` | 字段: `data: []u8, allocator: std.mem.Allocator` · 方法: `compress, compressHC, decompress, slice, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `LZ4_versionNumber` | `(): i32` | 调用 `LZ4_versionNumber` 并返回 `i32`。 |
| `LZ4_compressBound` | `(inputSize: i32): i32` | 调用 `LZ4_compressBound` 并返回 `i32`。 |
| `LZ4_compress_default` | `(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32` | 调用 `LZ4_compress_default` 并返回 `i32`。 |
| `LZ4_compress_fast` | `(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32` | 调用 `LZ4_compress_fast` 并返回 `i32`。 |
| `LZ4_compress_HC` | `(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32` | 调用 `LZ4_compress_HC` 并返回 `i32`。 |
| `LZ4_decompress_safe` | `(src: [*]&u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32` | 调用 `LZ4_decompress_safe` 并返回 `i32`。 |
| `versionNumber` | `(): i32` | 调用 `versionNumber` 并返回 `i32`。 |
| `compressBound` | `(input_len: usize): !usize` | 调用 `compressBound` 并返回 `!usize`。 |
| `compressInto` | `(src: &[]u8, dst: []u8): !&[]u8` | 调用 `compressInto` 并返回 `!&[]u8`。 |
| `compressFastInto` | `(src: &[]u8, dst: []u8, acceleration: i32): !&[]u8` | 调用 `compressFastInto` 并返回 `!&[]u8`。 |
| `compressHCInto` | `(src: &[]u8, dst: []u8, level: i32): !&[]u8` | 调用 `compressHCInto` 并返回 `!&[]u8`。 |
| `decompressInto` | `(src: &[]u8, dst: []u8): !&[]u8` | 调用 `decompressInto` 并返回 `!&[]u8`。 |

## 最小调用示例

```cp
import { versionNumber } from "@cap/lz4";

pub fn main() {
    let result = versionNumber();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
