[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# calver

Zero-allocation Calendar Versioning (CalVer) parser, validator, comparator, bumper, and formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:calver` ([https://github.com/muratgozel/node-calver](https://github.com/muratgozel/node-calver))

| 元信息 | 值 |
| --- | --- |
| 包名 | `calver` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add calver
```

```cp
import {
    CalverFormat,
    Calver,
    ModifierParts,
    cmpIgnoreCase,
    isLeapYear,
    daysInMonth,
    parseU32Strict,
    parseModifierTail,
} from "calver";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CalverFormat` | `YyyyMmMicro, YyMmMicro, Yyyy0mMicro, YyyyMmDd, YyyyMmDdMicro` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Calver` | `class` | 字段: `valid: bool, year: u16, month: u8, day: u8, micro: u32, modifier: string, +1 项` · 方法: `invalid` |
| `ModifierParts` | `class` | 字段: `ok: bool, name: string, num: u32` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmpIgnoreCase` | `(a: string, b: string): i32` | 调用 `cmpIgnoreCase` 并返回 `i32`。 |
| `isLeapYear` | `(year: u16): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(year: u16, month: u8): u8` | 调用 `daysInMonth` 并返回 `u8`。 |
| `parseU32Strict` | `(s: string): u32?` | 通过 `parseU32Strict` 解析输入数据，返回 `u32?`。 |
| `parseModifierTail` | `(s: string): ModifierParts` | 通过 `parseModifierTail` 解析输入数据，返回 `ModifierParts`。 |
| `parseCalverCore` | `(raw: string, force_day_on_3: bool): Calver` | 通过 `parseCalverCore` 解析输入数据，返回 `Calver`。 |
| `parseCalver` | `(raw: string): Calver` | 通过 `parseCalver` 解析输入数据，返回 `Calver`。 |
| `parseCalverWith` | `(raw: string, fmt: CalverFormat): Calver` | 通过 `parseCalverWith` 解析输入数据，返回 `Calver`。 |
| `modifierRank` | `(name: string): u8` | 调用 `modifierRank` 并返回 `u8`。 |
| `compareCalver` | `(a: Calver, b: Calver): i32` | 调用 `compareCalver` 并返回 `i32`。 |
| `bumpCalver` | `(cur: Calver, year: u16, month: u8, day: u8, use_day: bool): Calver` | 调用 `bumpCalver` 并返回 `Calver`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `append2Digits` | `(out: []u8, pos: usize, val: u32): usize` | 调用 `append2Digits` 并返回 `usize`。 |
| `formatCalver` | `(v: Calver, fmt: CalverFormat, out: []u8): string` | 通过 `formatCalver` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseU32Strict } from "calver";

pub fn main() {
    let result = parseU32Strict("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
