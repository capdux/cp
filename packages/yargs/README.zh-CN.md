[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# yargs

Zero-allocation CLI application framework (npm yargs v17 parity) composing yargs-parser, cliui, and y18n for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yargs` ([https://github.com/yargs/yargs](https://github.com/yargs/yargs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `yargs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add yargs
```

```cp
import {
    OptionSpec,
    CommandSpec,
    YargsResult,
    YargsApp,
    ygStrEq,
    ygAppendBytes,
    ygAppendByte,
    ygCopyBuf,
} from "yargs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OptionSpec` | `class` | 字段: `name: string, alias_ch: u8, desc: string, kind: u8, required: bool, default_str: string, +2 项` · 方法: `boolOpt, intOpt, strOpt` |
| `CommandSpec` | `class` | 字段: `name: string, desc: string` |
| `YargsResult` | `class` | 字段: `args: ParsedArgs, command: [64]u8, command_len: usize, show_help: bool, show_version: bool, ok: bool, +2 项` · 方法: `hasCommand, commandName, errorMessage, has, getBool, getInt, +5 项` |
| `YargsApp` | `class` | 字段: `script_name: string, usage_text: string, version_str: string, commands: [16]CommandSpec, cmd_count: usize, options: [24]OptionSpec, +2 项` · 方法: `new, usage, command, option, setTranslation, parse, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ygStrEq` | `(a: string, b: string): bool` | 调用 `ygStrEq` 并返回 `bool`。 |
| `ygAppendBytes` | `(dst: []u8, pos: usize, s: string): usize` | 调用 `ygAppendBytes` 并返回 `usize`。 |
| `ygAppendByte` | `(dst: []u8, pos: usize, b: u8): usize` | 调用 `ygAppendByte` 并返回 `usize`。 |
| `ygCopyBuf` | `(dst: []u8, src: string): usize` | 调用 `ygCopyBuf` 并返回 `usize`。 |
| `ygInsertEntry` | `(parsed: &mut ParsedArgs, name: string, kind: u8, bool_val: bool, int_val: i64, str_val: string)` | 就地执行 `ygInsertEntry` 操作。 |

## 最小调用示例

```cp
import { ygStrEq } from "yargs";

pub fn main() {
    let result = ygStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
