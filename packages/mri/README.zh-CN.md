[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mri

Zero-allocation POSIX/GNU CLI argument parser (--flag, --key=val, --key val, -abc short clusters, -- terminator, stdin '-') in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mri` ([https://github.com/lukeed/mri](https://github.com/lukeed/mri)) / `crates.io:lexopt` ([https://github.com/blyxxyz/lexopt](https://github.com/blyxxyz/lexopt))（原库名 `mri` → CP 包名 `mri`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mri` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mri
```

```cp
import {
    ParsedArg,
    strEq,
    parseArgs,
    hasFlag,
    getOption,
    countPositionals,
} from "mri";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedArg` | `class` | 字段: `kind: u8, name: string, value: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `strEq` | `(a: string, b: string): bool` | 调用 `strEq` 并返回 `bool`。 |
| `parseArgs` | `(argv: &[]string, out: []ParsedArg): usize` | 通过 `parseArgs` 解析输入数据，返回 `usize`。 |
| `hasFlag` | `(argv: &[]string, long_name: string, short_char: u8): bool` | 执行 `hasFlag` 条件判定，返回 `bool`。 |
| `getOption` | `(argv: &[]string, long_name: string, short_char: u8): string` | 查询或读取目标数据，返回 `string`。 |
| `countPositionals` | `(argv: &[]string): usize` | 调用 `countPositionals` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseArgs } from "mri";

pub fn main() {
    let result = parseArgs(/* argv: &[]string */, /* out: []ParsedArg */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
