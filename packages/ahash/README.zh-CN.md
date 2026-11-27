[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ahash

Zero-allocation randomized high-speed non-cryptographic hasher with 4-key RandomState, folded multiply, and rotation rounds (Rust ahash parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ahash` ([https://github.com/tkaitchuck/aHash](https://github.com/tkaitchuck/aHash))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ahash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ahash
```

```cp
import {
    AHasher,
    RandomState,
    makeU64,
    addWrap64,
    mulWrap64,
    pow2_64,
    rotl64,
    foldedMultiply,
} from "ahash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AHasher` | `class` | 字段: `buffer: u64, pad: u64, extra_keys: [2]u64, tail: [8]u8, tail_len: usize, total_len: u64` · 方法: `init, writeU8, writeU32, writeU64, writeBytes, finish` |
| `RandomState` | `class` | 字段: `k0: u64, k1: u64, k2: u64, k3: u64` · 方法: `withSeeds, defaultState, buildHasher, hashOneBytes, hashOneU64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `pow2_64` | `(n: u32): u64` | 调用 `pow2_64` 并返回 `u64`。 |
| `rotl64` | `(x: u64, r_in: u32): u64` | 调用 `rotl64` 并返回 `u64`。 |
| `foldedMultiply` | `(x: u64, y: u64): u64` | 调用 `foldedMultiply` 并返回 `u64`。 |
| `multipleConst` | `(): u64` | 调用 `multipleConst` 并返回 `u64`。 |
| `pi0` | `(): u64` | 调用 `pi0` 并返回 `u64`。 |
| `pi1` | `(): u64` | 调用 `pi1` 并返回 `u64`。 |
| `pi2` | `(): u64` | 调用 `pi2` 并返回 `u64`。 |
| `pi3` | `(): u64` | 调用 `pi3` 并返回 `u64`。 |
| `packTail` | `(tail: [8]u8, tail_len: usize): u64` | 调用 `packTail` 并返回 `u64`。 |
| `hashBytes` | `(data: &[]u8, k0: u64, k1: u64): u64` | 执行 `hashBytes` 条件判定，返回 `bool`。 |
| `hashU64` | `(v: u64, k0: u64, k1: u64): u64` | 执行 `hashU64` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { hashBytes } from "ahash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytes(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
