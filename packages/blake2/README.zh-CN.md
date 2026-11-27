[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# blake2

Zero-allocation RFC 7693 BLAKE2s-256 streaming cryptographic hash & keyed MAC (Rust blake2 crate parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:blake2` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `blake2` → CP 包名 `blake2`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `blake2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add blake2
```

```cp
import {
    Blake2s256,
    bl2sAdd2,
    bl2sAdd3,
    bl2sRotr16,
    bl2sRotr12,
    bl2sRotr8,
    bl2sRotr7,
    bl2sReadLe32,
} from "blake2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Blake2s256` | `class` | 字段: `h: [8]u32, t: u64, buf: [64]u8, buf_len: usize` · 方法: `new, newKeyed, update, updateStr, finalize, finalizeHex` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bl2sAdd2` | `(a: u32, b: u32): u32` | 调用 `bl2sAdd2` 并返回 `u32`。 |
| `bl2sAdd3` | `(a: u32, b: u32, c: u32): u32` | 调用 `bl2sAdd3` 并返回 `u32`。 |
| `bl2sRotr16` | `(x: u32): u32` | 调用 `bl2sRotr16` 并返回 `u32`。 |
| `bl2sRotr12` | `(x: u32): u32` | 调用 `bl2sRotr12` 并返回 `u32`。 |
| `bl2sRotr8` | `(x: u32): u32` | 调用 `bl2sRotr8` 并返回 `u32`。 |
| `bl2sRotr7` | `(x: u32): u32` | 调用 `bl2sRotr7` 并返回 `u32`。 |
| `bl2sReadLe32` | `(b: []u8, off: usize): u32` | 调用 `bl2sReadLe32` 并返回 `u32`。 |
| `bl2sWriteLe32` | `(out: []u8, off: usize, v: u32)` | 就地执行 `bl2sWriteLe32` 操作。 |
| `bl2sHexDigit` | `(nibble: u8): u8` | 调用 `bl2sHexDigit` 并返回 `u8`。 |
| `bl2sIv` | `(idx: usize): u32` | 调用 `bl2sIv` 并返回 `u32`。 |
| `bl2sMix` | `(v: []u32, ai: usize, bi: usize, ci: usize, di: usize, x: u32, y: u32)` | 就地执行 `bl2sMix` 操作。 |
| `bl2sCompress` | `(h: []u32, block: []u8, t: u64, is_last: bool)` | 就地执行 `bl2sCompress` 操作。 |
| `blake2s256Bytes` | `(data: []u8, out32: []u8)` | 就地执行 `blake2s256Bytes` 操作。 |
| `blake2s256Hex` | `(s: string, out64: []u8): usize` | 调用 `blake2s256Hex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { blake2s256Hex } from "blake2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = blake2s256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
