[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rand_hc

Zero-allocation HC-128 eSTREAM cryptographic stream cipher PRNG (Hongjun Wu specification) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_hc` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand), `MIT OR Apache-2.0`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `rand_hc` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rand_hc
```

```cp
import {
    Hc128Rng,
    rhcRotr32,
    rhcRotl32,
    rhcF1,
    rhcF2,
    rhcG1,
    rhcG2,
    rhcReadU32Le,
} from "rand_hc";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Hc128Rng` | `class` | 字段: `p: [512]u32, q: [512]u32, counter: usize` · 方法: `h1, h2, fromSeed, nextU32, nextU64, fillBytes` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rhcRotr32` | `(x: u32, n: usize): u32` | 调用 `rhcRotr32` 并返回 `u32`。 |
| `rhcRotl32` | `(x: u32, n: usize): u32` | 调用 `rhcRotl32` 并返回 `u32`。 |
| `rhcF1` | `(x: u32): u32` | 调用 `rhcF1` 并返回 `u32`。 |
| `rhcF2` | `(x: u32): u32` | 调用 `rhcF2` 并返回 `u32`。 |
| `rhcG1` | `(x: u32, y: u32, z: u32): u32` | 调用 `rhcG1` 并返回 `u32`。 |
| `rhcG2` | `(x: u32, y: u32, z: u32): u32` | 调用 `rhcG2` 并返回 `u32`。 |
| `rhcReadU32Le` | `(seed: [32]u8, offset: usize): u32` | 调用 `rhcReadU32Le` 并返回 `u32`。 |

## 最小调用示例

```cp
import { rhcRotr32 } from "rand_hc";

pub fn main() {
    let result = rhcRotr32(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
