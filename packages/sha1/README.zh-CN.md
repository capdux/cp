[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sha1

Zero-allocation FIPS 180-4 SHA-1 cryptographic hash streaming implementation (Rust sha1 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sha1` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `sha1` → CP 包名 `sha1`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `sha1` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sha1
```

```cp
import {
    Sha1Hasher,
    sha1Add32,
    sha1Add5,
    sha1Rotl1,
    sha1Rotl5,
    sha1Rotl30,
    sha1ReadBe32,
    sha1WriteBe32,
} from "sha1";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Sha1Hasher` | `class` | 字段: `h0: u32, h1: u32, h2: u32, h3: u32, h4: u32, buf: [64]u8, +2 项` · 方法: `new, reset, compressBuf, update, updateStr, finalize, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sha1Add32` | `(a: u32, b: u32): u32` | 调用 `sha1Add32` 并返回 `u32`。 |
| `sha1Add5` | `(a: u32, b: u32, c: u32, d: u32, e: u32): u32` | 调用 `sha1Add5` 并返回 `u32`。 |
| `sha1Rotl1` | `(x: u32): u32` | 调用 `sha1Rotl1` 并返回 `u32`。 |
| `sha1Rotl5` | `(x: u32): u32` | 调用 `sha1Rotl5` 并返回 `u32`。 |
| `sha1Rotl30` | `(x: u32): u32` | 调用 `sha1Rotl30` 并返回 `u32`。 |
| `sha1ReadBe32` | `(b: []u8, off: usize): u32` | 调用 `sha1ReadBe32` 并返回 `u32`。 |
| `sha1WriteBe32` | `(out: []u8, off: usize, v: u32)` | 就地执行 `sha1WriteBe32` 操作。 |
| `sha1HexDigit` | `(nibble: u8): u8` | 调用 `sha1HexDigit` 并返回 `u8`。 |
| `sha1Bytes` | `(data: []u8, out20: []u8)` | 就地执行 `sha1Bytes` 操作。 |
| `sha1Hex` | `(s: string, out40: []u8): usize` | 调用 `sha1Hex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { sha1Hex } from "sha1";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha1Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
