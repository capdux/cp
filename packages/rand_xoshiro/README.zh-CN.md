[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rand_xoshiro

Zero-allocation Blackman & Vigna xoshiro256** PRNG with SplitMix64 seeding, unbiased nextRange, 2^128 jump, and Fisher-Yates shuffle in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_xoshiro` ([https://github.com/rust-random/rngs](https://github.com/rust-random/rngs))（原库名 `rand_xoshiro` → CP 包名 `rand_xoshiro`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rand_xoshiro` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rand_xoshiro
```

```cp
import {
    Xoshiro256,
    makeU64,
    rotl7,
    rotl45,
    splitmix64Next,
    seedFromU64,
    nextU64,
    nextF64,
} from "rand_xoshiro";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Xoshiro256` | `class` | 字段: `s: [4]u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `rotl7` | `(x: u64): u64` | 调用 `rotl7` 并返回 `u64`。 |
| `rotl45` | `(x: u64): u64` | 调用 `rotl45` 并返回 `u64`。 |
| `splitmix64Next` | `(state: &mut u64): u64` | 调用 `splitmix64Next` 并返回 `u64`。 |
| `seedFromU64` | `(seed: u64): Xoshiro256` | 调用 `seedFromU64` 并返回 `Xoshiro256`。 |
| `nextU64` | `(rng: &mut Xoshiro256): u64` | 调用 `nextU64` 并返回 `u64`。 |
| `nextF64` | `(rng: &mut Xoshiro256): f64` | 调用 `nextF64` 并返回 `f64`。 |
| `nextRange` | `(rng: &mut Xoshiro256, upper_exclusive: u64): u64` | 调用 `nextRange` 并返回 `u64`。 |
| `jump` | `(rng: &mut Xoshiro256)` | 就地执行 `jump` 操作。 |
| `shuffleU32` | `(rng: &mut Xoshiro256, data: []u32)` | 就地执行 `shuffleU32` 操作。 |

## 最小调用示例

```cp
import { makeU64 } from "rand_xoshiro";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
