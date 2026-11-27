[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# md-5

Zero-allocation RFC 1321 MD5 cryptographic hash streaming implementation (Rust md-5/md5 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:md-5` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `md-5` → CP 包名 `md-5`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `md-5` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add md-5
```

```cp
import {
    Md5Hasher,
    md5Add32,
    md5Add4,
    md5Rotl32,
    md5ShiftAt,
    md5ConstAt,
    md5ReadLe32,
    md5WriteLe32,
} from "md-5";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Md5Hasher` | `class` | 字段: `a0: u32, b0: u32, c0: u32, d0: u32, buf: [64]u8, buf_len: usize, +1 项` · 方法: `new, reset, compressBuf, update, updateStr, finalize, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `md5Add32` | `(a: u32, b: u32): u32` | 调用 `md5Add32` 并返回 `u32`。 |
| `md5Add4` | `(a: u32, b: u32, c: u32, d: u32): u32` | 调用 `md5Add4` 并返回 `u32`。 |
| `md5Rotl32` | `(x: u32, bits: u32): u32` | 调用 `md5Rotl32` 并返回 `u32`。 |
| `md5ShiftAt` | `(idx: usize): u32` | 调用 `md5ShiftAt` 并返回 `u32`。 |
| `md5ConstAt` | `(idx: usize): u32` | 调用 `md5ConstAt` 并返回 `u32`。 |
| `md5ReadLe32` | `(b: []u8, off: usize): u32` | 调用 `md5ReadLe32` 并返回 `u32`。 |
| `md5WriteLe32` | `(out: []u8, off: usize, v: u32)` | 就地执行 `md5WriteLe32` 操作。 |
| `md5HexDigit` | `(nibble: u8): u8` | 调用 `md5HexDigit` 并返回 `u8`。 |
| `md5Bytes` | `(data: []u8, out16: []u8)` | 就地执行 `md5Bytes` 操作。 |
| `md5Hex` | `(s: string, out32: []u8): usize` | 调用 `md5Hex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { md5Hex } from "md-5";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = md5Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
