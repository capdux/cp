[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wildmatch

Zero-heap linear-time wildcard (*, ?, custom wildcards & case-insensitive) pattern matcher (clean-room port of Rust wildmatch)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wildmatch` ([https://github.com/becheran/wildmatch](https://github.com/becheran/wildmatch))

| 元信息 | 值 |
| --- | --- |
| 包名 | `wildmatch` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add wildmatch
```

```cp
import {
    WildMatch,
    wmByteEq,
    wmMatchCore,
    wildmatch,
    wildmatchIgnoreCase,
    wildmatchCustom,
    hasWildcards,
} from "wildmatch";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WildMatch` | `class` | 字段: `pattern: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool` · 方法: `new, newWithWildcards, newIgnoreCase, newWithEscape, matches` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wmByteEq` | `(a: u8, b: u8, ignore_case: bool): bool` | 调用 `wmByteEq` 并返回 `bool`。 |
| `wmMatchCore` | `(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool` | 调用 `wmMatchCore` 并返回 `bool`。 |
| `wildmatch` | `(pattern: string, input: string): bool` | 调用 `wildmatch` 并返回 `bool`。 |
| `wildmatchIgnoreCase` | `(pattern: string, input: string): bool` | 调用 `wildmatchIgnoreCase` 并返回 `bool`。 |
| `wildmatchCustom` | `(pattern: string, input: string, any_many: u8, any_one: u8): bool` | 调用 `wildmatchCustom` 并返回 `bool`。 |
| `hasWildcards` | `(pattern: string): bool` | 执行 `hasWildcards` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { wildmatch } from "wildmatch";

pub fn main() {
    let result = wildmatch("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
