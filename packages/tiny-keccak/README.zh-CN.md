[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tiny-keccak

Zero-allocation Keccak-f[1600] sponge with SHA3-256 (FIPS 202) and Keccak-256 (Ethereum) (Rust tiny-keccak parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tiny-keccak` ([https://github.com/debris/tiny-keccak](https://github.com/debris/tiny-keccak))

| 元信息 | 值 |
| --- | --- |
| 包名 | `tiny-keccak` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tiny-keccak
```

```cp
import {
    Sha3_256,
    Keccak256,
    kckU64,
    kckRotl64,
    kckReadLe64,
    kckWriteLe64,
    kckHexDigit,
    kckRoundConst,
} from "tiny-keccak";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Sha3_256` | `class` | 字段: `st: [25]u64, buf: [136]u8, buf_len: usize` · 方法: `new, update, updateStr, finalize, finalizeHex` |
| `Keccak256` | `class` | 字段: `st: [25]u64, buf: [136]u8, buf_len: usize` · 方法: `new, update, updateStr, finalize, finalizeHex` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `kckU64` | `(hi: u32, lo: u32): u64` | 调用 `kckU64` 并返回 `u64`。 |
| `kckRotl64` | `(x: u64, n: u32): u64` | 调用 `kckRotl64` 并返回 `u64`。 |
| `kckReadLe64` | `(b: []u8, off: usize): u64` | 调用 `kckReadLe64` 并返回 `u64`。 |
| `kckWriteLe64` | `(out: []u8, off: usize, v: u64)` | 就地执行 `kckWriteLe64` 操作。 |
| `kckHexDigit` | `(nibble: u8): u8` | 调用 `kckHexDigit` 并返回 `u8`。 |
| `kckRoundConst` | `(round: usize): u64` | 调用 `kckRoundConst` 并返回 `u64`。 |
| `kckPermute` | `(st: []u64)` | 就地执行 `kckPermute` 操作。 |
| `kckAbsorbBlock` | `(st: []u64, block: []u8)` | 就地执行 `kckAbsorbBlock` 操作。 |
| `kckFinalizeSponge` | `(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8)` | 就地执行 `kckFinalizeSponge` 操作。 |
| `kckEncodeHex32` | `(digest: []u8, out64: []u8): usize` | 调用 `kckEncodeHex32` 并返回 `usize`。 |
| `sha3_256Hex` | `(s: string, out64: []u8): usize` | 调用 `sha3_256Hex` 并返回 `usize`。 |
| `keccak256Hex` | `(s: string, out64: []u8): usize` | 调用 `keccak256Hex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { sha3_256Hex } from "tiny-keccak";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha3_256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
