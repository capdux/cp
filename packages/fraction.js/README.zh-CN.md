[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fraction.js

Zero-heap exact rational number (fraction) arithmetic in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fraction.js` ([https://github.com/rawify/Fraction.js](https://github.com/rawify/Fraction.js)) / `crates.io:fraction` ([https://github.com/dnsl48/fraction](https://github.com/dnsl48/fraction))（原库名 `fraction.js` → CP 包名 `fraction.js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fraction.js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fraction.js
```

```cp
import {
    Rational,
    absToU64,
    divTrunc64,
    gcd64,
    zero,
    one,
    fromInt,
    new,
} from "fraction.js";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rational` | `class` | 字段: `numer: i64, denom: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absToU64` | `(v: i64): u64` | 调用 `absToU64` 并返回 `u64`。 |
| `divTrunc64` | `(a: i64, b: i64): i64` | 调用 `divTrunc64` 并返回 `i64`。 |
| `gcd64` | `(a: i64, b: i64): i64` | 调用 `gcd64` 并返回 `i64`。 |
| `zero` | `(): Rational` | 调用 `zero` 并返回 `Rational`。 |
| `one` | `(): Rational` | 调用 `one` 并返回 `Rational`。 |
| `fromInt` | `(n: i64): Rational` | 调用 `fromInt` 并返回 `Rational`。 |
| `new` | `(n: i64, d: i64): Rational` | 创建或初始化实例，返回 `Rational`。 |
| `neg` | `(a: Rational): Rational` | 调用 `neg` 并返回 `Rational`。 |
| `abs` | `(a: Rational): Rational` | 调用 `abs` 并返回 `Rational`。 |
| `recip` | `(a: Rational): Rational` | 调用 `recip` 并返回 `Rational`。 |
| `add` | `(a: Rational, b: Rational): Rational` | 写入或更新状态数据。 |
| `sub` | `(a: Rational, b: Rational): Rational` | 调用 `sub` 并返回 `Rational`。 |
| `mul` | `(a: Rational, b: Rational): Rational` | 调用 `mul` 并返回 `Rational`。 |
| `div` | `(a: Rational, b: Rational): Rational` | 调用 `div` 并返回 `Rational`。 |
| `eq` | `(a: Rational, b: Rational): bool` | 调用 `eq` 并返回 `bool`。 |
| `cmp` | `(a: Rational, b: Rational): i32` | 调用 `cmp` 并返回 `i32`。 |
| `toF64` | `(a: Rational): f64` | 调用 `toF64` 并返回 `f64`。 |
| `floorInt` | `(a: Rational): i64` | 调用 `floorInt` 并返回 `i64`。 |
| `ceilInt` | `(a: Rational): i64` | 调用 `ceilInt` 并返回 `i64`。 |
| `isInteger` | `(a: Rational): bool` | 执行 `isInteger` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isInteger } from "fraction.js";

pub fn main() {
    let result = isInteger(/* a: Rational */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
