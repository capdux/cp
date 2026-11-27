[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# decimal.js

Zero-allocation 128-bit exact decimal & financial arithmetic engine (decimal.js / bignumber.js / rust_decimal parity: exact add/sub/mul/div, 6 rounding modes including Banker's HalfEven, scientific notation parsing) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:decimal.js` ([https://github.com/MikeMcl/decimal.js](https://github.com/MikeMcl/decimal.js))（原库名 `decimal.js` → CP 包名 `decimal.js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `decimal.js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add decimal.js
```

```cp
import {
    RoundingMode,
    Decimal,
    pow10,
    absI128,
    divRoundI128,
} from "decimal.js";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `RoundingMode` | `HalfEven, HalfUp, HalfDown, Floor, Ceil, Trunc` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Decimal` | `class` | 字段: `mantissa: i128, scale: u8` · 方法: `zero, one, fromInt, fromParts, parse, isZero, +21 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pow10` | `(exp: u8): i128` | 调用 `pow10` 并返回 `i128`。 |
| `absI128` | `(v: i128): i128` | 调用 `absI128` 并返回 `i128`。 |
| `divRoundI128` | `(num: i128, den: i128, mode: RoundingMode): i128` | 调用 `divRoundI128` 并返回 `i128`。 |

## 最小调用示例

```cp
import { pow10 } from "decimal.js";

pub fn main() {
    let result = pow10(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
