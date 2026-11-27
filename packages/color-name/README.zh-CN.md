[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# color-name

Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB/HSL/HSV/HEX color space conversion engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:color-name` ([https://github.com/colorjs/color-name](https://github.com/colorjs/color-name))（原库名 `color-name` → CP 包名 `color-name`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `color-name` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add color-name
```

```cp
import {
    RgbColor,
    ColorEntry,
    cnToLower,
    cnCmpIgnoreCase,
    colorAt,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
} from "color-name";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RgbColor` | `class` | 字段: `r: u8, g: u8, b: u8, found: bool` |
| `ColorEntry` | `class` | 字段: `name: string, r: u8, g: u8, b: u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cnToLower` | `(b: u8): u8` | 调用 `cnToLower` 并返回 `u8`。 |
| `cnCmpIgnoreCase` | `(a: string, b: string): i8` | 调用 `cnCmpIgnoreCase` 并返回 `i8`。 |
| `colorAt` | `(index: usize): ColorEntry` | 调用 `colorAt` 并返回 `ColorEntry`。 |
| `lookupColor` | `(name: string): RgbColor` | 查询或读取目标数据，返回 `RgbColor`。 |
| `lookupColorHex` | `(name: string): i32` | 查询或读取目标数据，返回 `i32`。 |
| `reverseLookupColor` | `(r: u8, g: u8, b: u8): string` | 调用 `reverseLookupColor` 并返回 `string`。 |
| `isNamedColor` | `(name: string): bool` | 执行 `isNamedColor` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isNamedColor } from "color-name";

pub fn main() {
    let result = isNamedColor("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
