[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bn.js

Zero-allocation 256-bit unsigned big integer arithmetic (4x u64 limbs) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bn.js` ([https://github.com/indutny/bn.js](https://github.com/indutny/bn.js)) / `crates.io:primitive-types` ([https://github.com/paritytech/parity-common](https://github.com/paritytech/parity-common)) / `crates.io:ruint` ([https://github.com/recmo/uint](https://github.com/recmo/uint))（原库名 `bn.js` → CP 包名 `bn.js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bn.js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bn.js
```

```cp
import {
    Add64Res,
    Sub64Res,
    Mul64Res,
    DivRem256,
    U256,
    addWithCarry64,
    subWithBorrow64,
    mulWide64,
} from "bn.js";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Add64Res` | `class` | 字段: `sum: u64, carry: u64` |
| `Sub64Res` | `class` | 字段: `diff: u64, borrow: u64` |
| `Mul64Res` | `class` | 字段: `lo: u64, hi: u64` |
| `DivRem256` | `class` | 字段: `quot: U256, rem: U256` |
| `U256` | `class` | 字段: `limbs: [4]u64` · 方法: `zero, one, fromU64, fromU128, maxVal, fromHex, +15 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `addWithCarry64` | `(a: u64, b: u64, carry_in: u64): Add64Res` | 写入或更新状态数据。 |
| `subWithBorrow64` | `(a: u64, b: u64, borrow_in: u64): Sub64Res` | 调用 `subWithBorrow64` 并返回 `Sub64Res`。 |
| `mulWide64` | `(a: u64, b: u64): Mul64Res` | 调用 `mulWide64` 并返回 `Mul64Res`。 |
| `clz64` | `(x: u64): u32` | 调用 `clz64` 并返回 `u32`。 |
| `zero` | `(): U256` | 调用 `zero` 并返回 `U256`。 |
| `one` | `(): U256` | 调用 `one` 并返回 `U256`。 |
| `fromU64` | `(v: u64): U256` | 调用 `fromU64` 并返回 `U256`。 |
| `fromU128` | `(lo: u64, hi: u64): U256` | 调用 `fromU128` 并返回 `U256`。 |
| `maxVal` | `(): U256` | 调用 `maxVal` 并返回 `U256`。 |
| `fromHex` | `(s: string): U256` | 调用 `fromHex` 并返回 `U256`。 |

## 最小调用示例

```cp
import { fromHex } from "bn.js";

pub fn main() {
    let result = fromHex("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
