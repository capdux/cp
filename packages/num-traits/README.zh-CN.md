[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# num-traits

Zero-heap overflow-safe numeric operations and bit manipulation primitives: checked/saturating arithmetic, clamp, signum, absDiff, checkedPow, powers of two, popcount, clz, ctz, and rotations (clean-room port of Rust num-traits)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-traits` ([https://github.com/rust-num/num-traits](https://github.com/rust-num/num-traits))

| 元信息 | 值 |
| --- | --- |
| 包名 | `num-traits` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add num-traits
```

```cp
import {
    ntrtAbsI64ToU64,
    checkedAddI64,
    checkedSubI64,
    checkedMulI64,
    checkedAddU64,
    checkedSubU64,
    checkedMulU64,
    saturatingAddU64,
} from "num-traits";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ntrtAbsI64ToU64` | `(x: i64): u64` | 调用 `ntrtAbsI64ToU64` 并返回 `u64`。 |
| `checkedAddI64` | `(a: i64, b: i64): i64?` | 校验输入合法性，返回 `i64?`。 |
| `checkedSubI64` | `(a: i64, b: i64): i64?` | 校验输入合法性，返回 `i64?`。 |
| `checkedMulI64` | `(a: i64, b: i64): i64?` | 校验输入合法性，返回 `i64?`。 |
| `checkedAddU64` | `(a: u64, b: u64): u64?` | 校验输入合法性，返回 `u64?`。 |
| `checkedSubU64` | `(a: u64, b: u64): u64?` | 校验输入合法性，返回 `u64?`。 |
| `checkedMulU64` | `(a: u64, b: u64): u64?` | 校验输入合法性，返回 `u64?`。 |
| `saturatingAddU64` | `(a: u64, b: u64): u64` | 调用 `saturatingAddU64` 并返回 `u64`。 |
| `saturatingSubU64` | `(a: u64, b: u64): u64` | 调用 `saturatingSubU64` 并返回 `u64`。 |
| `saturatingMulU64` | `(a: u64, b: u64): u64` | 调用 `saturatingMulU64` 并返回 `u64`。 |
| `saturatingAddI64` | `(a: i64, b: i64): i64` | 调用 `saturatingAddI64` 并返回 `i64`。 |
| `saturatingSubI64` | `(a: i64, b: i64): i64` | 调用 `saturatingSubI64` 并返回 `i64`。 |
| `clampI64` | `(v: i64, lo: i64, hi: i64): i64` | 调用 `clampI64` 并返回 `i64`。 |
| `clampF64` | `(v: f64, lo: f64, hi: f64): f64` | 调用 `clampF64` 并返回 `f64`。 |
| `signumI64` | `(v: i64): i64` | 调用 `signumI64` 并返回 `i64`。 |
| `absDiffU64` | `(a: u64, b: u64): u64` | 调用 `absDiffU64` 并返回 `u64`。 |
| `checkedPowU64` | `(base: u64, exp: u32): u64?` | 校验输入合法性，返回 `u64?`。 |
| `isPowerOfTwoU64` | `(n: u64): bool` | 执行 `isPowerOfTwoU64` 条件判定，返回 `bool`。 |
| `nextPowerOfTwoU64` | `(n: u64): u64?` | 调用 `nextPowerOfTwoU64` 并返回 `u64?`。 |
| `countOnesU64` | `(n: u64): u32` | 调用 `countOnesU64` 并返回 `u32`。 |
| `leadingZerosU64` | `(n: u64): u32` | 调用 `leadingZerosU64` 并返回 `u32`。 |
| `trailingZerosU64` | `(n: u64): u32` | 调用 `trailingZerosU64` 并返回 `u32`。 |
| `rotateLeftU64` | `(n: u64, k: u32): u64` | 调用 `rotateLeftU64` 并返回 `u64`。 |
| `rotateRightU64` | `(n: u64, k: u32): u64` | 调用 `rotateRightU64` 并返回 `u64`。 |

## 最小调用示例

```cp
import { isPowerOfTwoU64 } from "num-traits";

pub fn main() {
    let result = isPowerOfTwoU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
