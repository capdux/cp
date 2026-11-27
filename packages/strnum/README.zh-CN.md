[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strnum

Zero-heap configurable string-to-number parser for CP (clean-room MIT implementation of npm strnum).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strnum` ([https://github.com/NaturalIntelligence/strnum](https://github.com/NaturalIntelligence/strnum))（原库名 `strnum` → CP 包名 `strnum`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `strnum` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add strnum
```

```cp
import {
    StrNumOptions,
    StrNumResult,
    snumInvalid,
    snumIsSpace,
    snumHexDigit,
    snumScalePow10,
    parseStrNum,
} from "strnum";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StrNumOptions` | `class` | 字段: `hex: bool, leading_zeros: bool, e_notation: bool` |
| `StrNumResult` | `class` | 字段: `is_number: bool, value: f64, is_int: bool, int_value: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `snumInvalid` | `(): StrNumResult` | 调用 `snumInvalid` 并返回 `StrNumResult`。 |
| `snumIsSpace` | `(ch: u8): bool` | 调用 `snumIsSpace` 并返回 `bool`。 |
| `snumHexDigit` | `(ch: u8): i32` | 调用 `snumHexDigit` 并返回 `i32`。 |
| `snumScalePow10` | `(val: f64, exp10: i32): f64` | 调用 `snumScalePow10` 并返回 `f64`。 |
| `parseStrNum` | `(input: string, opts: StrNumOptions): StrNumResult` | 通过 `parseStrNum` 解析输入数据，返回 `StrNumResult`。 |

## 最小调用示例

```cp
import { parseStrNum } from "strnum";

pub fn main() {
    let result = parseStrNum("example", /* opts: StrNumOptions */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
