[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# nopt

Zero-allocation CLI option parser with shorthands, --no-flag negation, and -- terminator (npm nopt parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`nopt`](https://github.com/npm/nopt)（`ISC`）（包名 `nopt`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `nopt` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add nopt
```

```cp
import {
    NoptFlag,
    NoptShorthand,
    NoptResult,
    nptStrEq,
    nptHasPrefix,
    nptIndexOfByte,
    nptStripLeadingDashes,
    nptFindShorthand,
} from "nopt";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `NoptFlag` | `class` | 字段: `key: string, value: string, is_bool: bool, bool_val: bool` |
| `NoptShorthand` | `class` | 字段: `short_key: string, long_key: string` |
| `NoptResult` | `class` | 字段: `flags: [64]NoptFlag, flag_count: usize, remain: [64]string, remain_count: usize` · 方法: `new, getString, getBool, has, remainCount, remainAt, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nptStrEq` | `(a: string, b: string): bool` | 调用 `nptStrEq` 并返回 `bool`。 |
| `nptHasPrefix` | `(s: string, prefix: string): bool` | 调用 `nptHasPrefix` 并返回 `bool`。 |
| `nptIndexOfByte` | `(s: string, ch: u8): usize?` | 调用 `nptIndexOfByte` 并返回 `usize?`。 |
| `nptStripLeadingDashes` | `(s: string): string` | 调用 `nptStripLeadingDashes` 并返回 `string`。 |
| `nptFindShorthand` | `(shorthands: &[]NoptShorthand, short_name: string): string?` | 调用 `nptFindShorthand` 并返回 `string?`。 |
| `nptIsOptionToken` | `(arg: string): bool` | 调用 `nptIsOptionToken` 并返回 `bool`。 |
| `parseNopt` | `(args: &[]string, shorthands: &[]NoptShorthand): NoptResult` | 通过 `parseNopt` 解析输入数据，返回 `NoptResult`。 |

## 最小调用示例

```cp
import { parseNopt } from "nopt";

pub fn main() {
    let result = parseNopt(/* args: &[]string */, /* shorthands: &[]NoptShorthand */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
