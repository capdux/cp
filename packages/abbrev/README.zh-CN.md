[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# abbrev

Zero-heap unique unambiguous prefix abbreviation generator and resolver for CP (clean-room ISC implementation of npm abbrev).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:abbrev` ([https://github.com/npm/abbrev-js](https://github.com/npm/abbrev-js))（原库名 `abbrev` → CP 包名 `abbrev`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `abbrev` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add abbrev
```

```cp
import {
    AbbrevEntry,
    abvStrEq,
    abvStrLt,
    abvHasPrefix,
    abvLcp,
    abbrev,
    shortestAbbrev,
    resolveAbbrev,
} from "abbrev";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AbbrevEntry` | `class` | 字段: `abbrev: string, full: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `abvStrEq` | `(a: string, b: string): bool` | 调用 `abvStrEq` 并返回 `bool`。 |
| `abvStrLt` | `(a: string, b: string): bool` | 调用 `abvStrLt` 并返回 `bool`。 |
| `abvHasPrefix` | `(word: string, prefix: string): bool` | 调用 `abvHasPrefix` 并返回 `bool`。 |
| `abvLcp` | `(a: string, b: string): usize` | 调用 `abvLcp` 并返回 `usize`。 |
| `abbrev` | `(words: &[]string, out: []AbbrevEntry): usize` | 调用 `abbrev` 并返回 `usize`。 |
| `shortestAbbrev` | `(words: &[]string, target: string): string?` | 调用 `shortestAbbrev` 并返回 `string?`。 |
| `resolveAbbrev` | `(words: &[]string, prefix: string): string?` | 调用 `resolveAbbrev` 并返回 `string?`。 |

## 最小调用示例

```cp
import { abvStrEq } from "abbrev";

pub fn main() {
    let result = abvStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
