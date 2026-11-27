[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# argparse

Zero-allocation Python-style CLI argument parser supporting short/long flags, counts, bundling, and required checks for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:argparse` ([https://github.com/nodeca/argparse](https://github.com/nodeca/argparse))

| 元信息 | 值 |
| --- | --- |
| 包名 | `argparse` |
| 版本 | `1.0.0` |
| 许可证 | `Python-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add argparse
```

```cp
import {
    ArgAction,
    ArgDef,
    ArgSlot,
    ParseResult,
    ArgParser,
    argpStrEq,
    argpStripDashes,
    argpFindEq,
} from "argparse";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ArgAction` | `Store, StoreTrue, StoreFalse, Count` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ArgDef` | `class` | 字段: `short_flag: u8, long_name: string, dest: string, action: ArgAction, required: bool, default_val: string, +1 项` |
| `ArgSlot` | `class` | 字段: `dest: string, action: ArgAction, has_str: bool, str_val: string, bool_val: bool, count_val: u32, +1 项` |
| `ParseResult` | `class` | 字段: `ok: bool, err_code: i32, slot_count: usize, slots: [32]ArgSlot, pos_count: usize, positionals: [32]string` · 方法: `getString, getBool, getCount, positionalCount, positionalAt` |
| `ArgParser` | `class` | 字段: `def_count: usize, defs: [32]ArgDef` · 方法: `init, addArgument, parseArgs` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `argpStrEq` | `(a: string, b: string): bool` | 调用 `argpStrEq` 并返回 `bool`。 |
| `argpStripDashes` | `(s: string): string` | 调用 `argpStripDashes` 并返回 `string`。 |
| `argpFindEq` | `(s: string): usize?` | 调用 `argpFindEq` 并返回 `usize?`。 |
| `argpResolveDest` | `(def: ArgDef): string` | 调用 `argpResolveDest` 并返回 `string`。 |
| `argpFindLong` | `(defs: &[]ArgDef, name: string): usize?` | 调用 `argpFindLong` 并返回 `usize?`。 |
| `argpFindShort` | `(defs: &[]ArgDef, ch: u8): usize?` | 调用 `argpFindShort` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { argpStrEq } from "argparse";

pub fn main() {
    let result = argpStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
