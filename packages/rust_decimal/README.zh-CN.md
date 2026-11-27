[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rust_decimal

Zero-heap fixed-point financial decimal arithmetic (scale 0..12) with banker's rounding in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rust_decimal` ([https://github.com/paupino/rust-decimal](https://github.com/paupino/rust-decimal))（原库名 `rust_decimal` → CP 包名 `rust_decimal`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rust_decimal` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rust_decimal
```

```cp
import {
    ParseDecResult,
    Decimal,
    absToU64,
    clampScale,
    pow10F64,
} from "rust_decimal";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParseDecResult` | `class` | 字段: `value: Decimal, valid: bool` |
| `Decimal` | `class` | 字段: `mantissa: i64, scale: u8` · 方法: `new, fromInt, zero, one, parse, normalize, +12 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absToU64` | `(v: i64): u64` | 调用 `absToU64` 并返回 `u64`。 |
| `clampScale` | `(s: u8): u8` | 调用 `clampScale` 并返回 `u8`。 |
| `pow10F64` | `(s: u8): f64` | 调用 `pow10F64` 并返回 `f64`。 |

## 最小调用示例

```cp
import { absToU64 } from "rust_decimal";

pub fn main() {
    let result = absToU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
