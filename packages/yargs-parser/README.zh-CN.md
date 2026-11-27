[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# yargs-parser

Zero-allocation CLI argument parser (npm yargs-parser parity) with long/short flags, --no- negation, camelCase/kebab-case key lookup, numeric/boolean coercion, and -- rest passthrough for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yargs-parser` ([https://github.com/yargs/yargs-parser](https://github.com/yargs/yargs-parser))

| 元信息 | 值 |
| --- | --- |
| 包名 | `yargs-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add yargs-parser
```

```cp
import {
    ParsedItem,
    ParsedEntry,
    ParsedArgs,
    ypIsNegativeNumber,
    ypFormatInt,
    ypIsFlagToken,
    ypEmptyItem,
    ypEmptyEntry,
} from "yargs-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedItem` | `class` | 字段: `data: [128]u8, len: usize` |
| `ParsedEntry` | `class` | 字段: `key: [64]u8, key_len: usize, camel_key: [64]u8, camel_len: usize, kebab_key: [64]u8, kebab_len: usize, +5 项` |
| `ParsedArgs` | `class` | 字段: `entries: [32]ParsedEntry, entry_count: usize, positionals: [32]ParsedItem, pos_count: usize, rest: [16]ParsedItem, rest_count: usize` · 方法: `findIndex, has, getBool, getInt, getString, entryCount, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ypIsNegativeNumber` | `(s: string): bool` | 调用 `ypIsNegativeNumber` 并返回 `bool`。 |
| `ypFormatInt` | `(valIn: i64, out: []u8): i64` | 调用 `ypFormatInt` 并返回 `i64`。 |
| `ypIsFlagToken` | `(s: string): bool` | 调用 `ypIsFlagToken` 并返回 `bool`。 |
| `ypEmptyItem` | `(): ParsedItem` | 调用 `ypEmptyItem` 并返回 `ParsedItem`。 |
| `ypEmptyEntry` | `(): ParsedEntry` | 调用 `ypEmptyEntry` 并返回 `ParsedEntry`。 |
| `ypEntryMatchesKey` | `(entry: ParsedEntry, query: string): bool` | 调用 `ypEntryMatchesKey` 并返回 `bool`。 |
| `ypPushPositional` | `(args: &mut ParsedArgs, s: string)` | 就地执行 `ypPushPositional` 操作。 |
| `ypPushRest` | `(args: &mut ParsedArgs, s: string)` | 就地执行 `ypPushRest` 操作。 |
| `ypSetEntry` | `(args: &mut ParsedArgs, rawKey: string, kind: u8, boolVal: bool, intVal: i64, strVal: string)` | 就地执行 `ypSetEntry` 操作。 |
| `ypSetCoerced` | `(args: &mut ParsedArgs, rawKey: string, rawVal: string)` | 就地执行 `ypSetCoerced` 操作。 |
| `parseArgs` | `(argv: &[]string): ParsedArgs` | 通过 `parseArgs` 解析输入数据，返回 `ParsedArgs`。 |

## 最小调用示例

```cp
import { parseArgs } from "yargs-parser";

pub fn main() {
    let result = parseArgs(/* argv: &[]string */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
