[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# num-integer

Zero-allocation integer number theory, overflow-safe GCD/LCM/extended-GCD, floor/ceil division, isqrt/icbrt, and binomial coefficients for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-integer` ([https://github.com/rust-num/num-integer](https://github.com/rust-num/num-integer))

| 元信息 | 值 |
| --- | --- |
| 包名 | `num-integer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add num-integer
```

```cp
import {
    ExtGcdResult,
    nintAbsU64,
    nintDivTruncRem,
    gcdU64,
    lcmU64,
    extendedGcd,
    divFloor,
    modFloor,
} from "num-integer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ExtGcdResult` | `class` | 字段: `gcd: i64, x: i64, y: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nintAbsU64` | `(v: i64): u64` | 调用 `nintAbsU64` 并返回 `u64`。 |
| `nintDivTruncRem` | `(a: i64, b: i64, q_out: &mut i64, r_out: &mut i64)` | 就地执行 `nintDivTruncRem` 操作。 |
| `gcdU64` | `(a: u64, b: u64): u64` | 调用 `gcdU64` 并返回 `u64`。 |
| `lcmU64` | `(a: u64, b: u64): u64` | 调用 `lcmU64` 并返回 `u64`。 |
| `extendedGcd` | `(a: i64, b: i64): ExtGcdResult` | 调用 `extendedGcd` 并返回 `ExtGcdResult`。 |
| `divFloor` | `(a: i64, b: i64): i64` | 调用 `divFloor` 并返回 `i64`。 |
| `modFloor` | `(a: i64, b: i64): i64` | 调用 `modFloor` 并返回 `i64`。 |
| `divCeil` | `(a: i64, b: i64): i64` | 调用 `divCeil` 并返回 `i64`。 |
| `isqrtU64` | `(n: u64): u64` | 执行 `isqrtU64` 条件判定，返回 `bool`。 |
| `icbrtU64` | `(n: u64): u64` | 调用 `icbrtU64` 并返回 `u64`。 |
| `binomialU64` | `(n: u64, k: u64): u64` | 调用 `binomialU64` 并返回 `u64`。 |
| `isEven` | `(n: i64): bool` | 执行 `isEven` 条件判定，返回 `bool`。 |
| `isOdd` | `(n: i64): bool` | 执行 `isOdd` 条件判定，返回 `bool`。 |
| `nextMultipleOfU64` | `(n: u64, step: u64): u64` | 调用 `nextMultipleOfU64` 并返回 `u64`。 |
| `prevMultipleOfU64` | `(n: u64, step: u64): u64` | 调用 `prevMultipleOfU64` 并返回 `u64`。 |

## 最小调用示例

```cp
import { isqrtU64 } from "num-integer";

pub fn main() {
    let result = isqrtU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
