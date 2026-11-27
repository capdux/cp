[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rand_chacha

Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 block function and CSPRNG for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_chacha` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand)) / `crates.io:ppv-lite86` ([https://github.com/cryptocorrosion/cryptocorrosion](https://github.com/cryptocorrosion/cryptocorrosion))（原库名 `rand_chacha` → CP 包名 `rand_chacha`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rand_chacha` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rand_chacha
```

```cp
import {
    ChaCha20Rng,
    crAdd32,
    crRotl32,
    crReadLe32,
    crWriteLe32,
    crQuarterRound,
    chachaBlock,
} from "rand_chacha";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ChaCha20Rng` | `class` | 字段: `key: [8]u32, nonce: [3]u32, counter: u32, buf: [64]u8, pos: usize` · 方法: `fromSeed, fromU64, setWordPos, fillBytes, nextU32, nextU64, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `crAdd32` | `(a: u32, b: u32): u32` | 调用 `crAdd32` 并返回 `u32`。 |
| `crRotl32` | `(x: u32, bits: u32): u32` | 调用 `crRotl32` 并返回 `u32`。 |
| `crReadLe32` | `(data: &[]u8, offset: usize): u32` | 调用 `crReadLe32` 并返回 `u32`。 |
| `crWriteLe32` | `(out: []u8, offset: usize, v: u32)` | 就地执行 `crWriteLe32` 操作。 |
| `crQuarterRound` | `(st: []u32, a: usize, b: usize, c: usize, d: usize)` | 就地执行 `crQuarterRound` 操作。 |
| `chachaBlock` | `(key: [8]u32, counter: u32, nonce: [3]u32, rounds: u32, out: []u8)` | 就地执行 `chachaBlock` 操作。 |

## 最小调用示例

```cp
import { crAdd32 } from "rand_chacha";

pub fn main() {
    let result = crAdd32(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
