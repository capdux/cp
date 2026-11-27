[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# untrusted

Zero-heap big-endian/little-endian binary codecs & bounds-checked untrusted ByteReader/ByteWriter (inspired by Rust byteorder + untrusted)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:untrusted` ([https://github.com/briansmith/untrusted](https://github.com/briansmith/untrusted)) / `crates.io:byteorder` ([https://github.com/BurntSushi/byteorder](https://github.com/BurntSushi/byteorder))（原库名 `untrusted` → CP 包名 `untrusted`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `untrusted` |
| 版本 | `1.0.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add untrusted
```

```cp
import {
    ByteReader,
    ByteWriter,
    readU16Be,
    readU16Le,
    readU32Be,
    readU32Le,
    readU64Be,
    readU64Le,
} from "untrusted";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ByteReader` | `class` | 字段: `data: string, pos: usize` · 方法: `init, remaining, atEnd, peekU8, readU8, readU16Be, +8 项` |
| `ByteWriter` | `class` | 字段: `out: []u8, pos: usize, overflow: bool` · 方法: `init, asSlice, writeU8, writeU16Be, writeU16Le, writeU32Be, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `readU16Be` | `(buf: string, off: usize): u16?` | 调用 `readU16Be` 并返回 `u16?`。 |
| `readU16Le` | `(buf: string, off: usize): u16?` | 调用 `readU16Le` 并返回 `u16?`。 |
| `readU32Be` | `(buf: string, off: usize): u32?` | 调用 `readU32Be` 并返回 `u32?`。 |
| `readU32Le` | `(buf: string, off: usize): u32?` | 调用 `readU32Le` 并返回 `u32?`。 |
| `readU64Be` | `(buf: string, off: usize): u64?` | 调用 `readU64Be` 并返回 `u64?`。 |
| `readU64Le` | `(buf: string, off: usize): u64?` | 调用 `readU64Le` 并返回 `u64?`。 |
| `writeU16Be` | `(out: []u8, off: usize, v: u16): bool` | 调用 `writeU16Be` 并返回 `bool`。 |
| `writeU16Le` | `(out: []u8, off: usize, v: u16): bool` | 调用 `writeU16Le` 并返回 `bool`。 |
| `writeU32Be` | `(out: []u8, off: usize, v: u32): bool` | 调用 `writeU32Be` 并返回 `bool`。 |
| `writeU32Le` | `(out: []u8, off: usize, v: u32): bool` | 调用 `writeU32Le` 并返回 `bool`。 |
| `writeU64Be` | `(out: []u8, off: usize, v: u64): bool` | 调用 `writeU64Be` 并返回 `bool`。 |
| `writeU64Le` | `(out: []u8, off: usize, v: u64): bool` | 调用 `writeU64Le` 并返回 `bool`。 |

## 最小调用示例

```cp
import { readU16Be } from "untrusted";

pub fn main() {
    let result = readU16Be("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
