[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# smart-buffer

Zero-heap cursor-based binary reader/writer buffer with big/little-endian integer and string operations

| 元信息 | 值 |
| --- | --- |
| 包名 | `smart-buffer` |
| 版本 | `4.2.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add smart-buffer
```

```cp
import {
    SmartBuffer,
    sbufMin,
    sbufW16BE,
    sbufW16LE,
    sbufW32BE,
    sbufW32LE,
    sbufR16BE,
    sbufR16LE,
} from "smart-buffer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SmartBuffer` | `class` | 字段: `buf: [1024]u8, read_offset: usize, write_offset: usize` · 方法: `new, fromSlice, writeU8, writeU16BE, writeU16LE, writeU32BE, +12 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sbufMin` | `(a: usize, b: usize): usize` | 调用 `sbufMin` 并返回 `usize`。 |
| `sbufW16BE` | `(dst: []u8, off: usize, v: u16): bool` | 调用 `sbufW16BE` 并返回 `bool`。 |
| `sbufW16LE` | `(dst: []u8, off: usize, v: u16): bool` | 调用 `sbufW16LE` 并返回 `bool`。 |
| `sbufW32BE` | `(dst: []u8, off: usize, v: u32): bool` | 调用 `sbufW32BE` 并返回 `bool`。 |
| `sbufW32LE` | `(dst: []u8, off: usize, v: u32): bool` | 调用 `sbufW32LE` 并返回 `bool`。 |
| `sbufR16BE` | `(src: &[]u8, off: usize): u16?` | 调用 `sbufR16BE` 并返回 `u16?`。 |
| `sbufR16LE` | `(src: &[]u8, off: usize): u16?` | 调用 `sbufR16LE` 并返回 `u16?`。 |
| `sbufR32BE` | `(src: &[]u8, off: usize): u32?` | 调用 `sbufR32BE` 并返回 `u32?`。 |
| `sbufR32LE` | `(src: &[]u8, off: usize): u32?` | 调用 `sbufR32LE` 并返回 `u32?`。 |

## 最小调用示例

```cp
import { sbufMin } from "smart-buffer";

pub fn main() {
    let result = sbufMin(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
