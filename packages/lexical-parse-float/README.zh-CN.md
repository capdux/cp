[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lexical-parse-float

Zero-allocation pure CP strict floating-point and radix integer string parser (rust-lexical lexical-parse-float / lexical-parse-integer / lexical-util parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lexical-parse-float` ([https://github.com/Alexhuszagh/rust-lexical](https://github.com/Alexhuszagh/rust-lexical))

| 元信息 | 值 |
| --- | --- |
| 包名 | `lexical-parse-float` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lexical-parse-float
```

```cp
import {
    ParseFloatResult,
    ParseIntResult,
    lexpToLower,
    lexpEqIgnoreCase,
    lexpDecodeDigit,
    lexpScalePow10,
    parseFloat,
    parseIntRadix,
} from "lexical-parse-float";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParseFloatResult` | `class` | 字段: `ok: bool, value: f64, consumed: usize` |
| `ParseIntResult` | `class` | 字段: `ok: bool, value: i64, consumed: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lexpToLower` | `(c: u8): u8` | 调用 `lexpToLower` 并返回 `u8`。 |
| `lexpEqIgnoreCase` | `(a: string, b: string): bool` | 调用 `lexpEqIgnoreCase` 并返回 `bool`。 |
| `lexpDecodeDigit` | `(c: u8): i32` | 调用 `lexpDecodeDigit` 并返回 `i32`。 |
| `lexpScalePow10` | `(val_in: f64, exp10: i32): f64` | 调用 `lexpScalePow10` 并返回 `f64`。 |
| `parseFloat` | `(s: string): ParseFloatResult` | 通过 `parseFloat` 解析输入数据，返回 `ParseFloatResult`。 |
| `parseIntRadix` | `(s: string, radix: u32): ParseIntResult` | 通过 `parseIntRadix` 解析输入数据，返回 `ParseIntResult`。 |

## 最小调用示例

```cp
import { parseFloat } from "lexical-parse-float";

pub fn main() {
    let result = parseFloat("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
