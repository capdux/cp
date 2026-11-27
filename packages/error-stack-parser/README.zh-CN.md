[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# error-stack-parser

Zero-allocation multi-language stack trace parser & cleaner supporting V8/Node/Bun, SpiderMonkey/Safari, and Rust/Zig/CP formats (error-stack-parser / stacktrace-parser / clean-stack parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:error-stack-parser` ([https://github.com/stacktracejs/error-stack-parser](https://github.com/stacktracejs/error-stack-parser))（原库名 `error-stack-parser` → CP 包名 `error-stack-parser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `error-stack-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add error-stack-parser
```

```cp
import {
    StackFrame,
    ParsedStack,
    LocationInfo,
    isInternalLocation,
    parseLocation,
    parseFrameLine,
    parseStack,
    pushByte,
} from "error-stack-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StackFrame` | `class` | 字段: `func_name: string, file: string, line: u32, column: u32, is_native: bool, is_internal: bool` · 方法: `empty` |
| `ParsedStack` | `class` | 字段: `frames: [MAX_FRAMES]StackFrame, count: usize, message: string` · 方法: `empty, frameAt` |
| `LocationInfo` | `class` | 字段: `file: string, line: u32, column: u32, is_native: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isInternalLocation` | `(file: string, is_native: bool): bool` | 执行 `isInternalLocation` 条件判定，返回 `bool`。 |
| `parseLocation` | `(raw_loc: string): LocationInfo` | 通过 `parseLocation` 解析输入数据，返回 `LocationInfo`。 |
| `parseFrameLine` | `(raw_line: string): StackFrame?` | 通过 `parseFrameLine` 解析输入数据，返回 `StackFrame?`。 |
| `parseStack` | `(raw: string): ParsedStack` | 通过 `parseStack` 解析输入数据，返回 `ParsedStack`。 |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `formatFrameAt` | `(f: StackFrame, out: []u8, start_pos: usize): usize` | 通过 `formatFrameAt` 编码或格式化输出，返回 `usize`。 |
| `formatFrame` | `(f: StackFrame, out: []u8): string` | 通过 `formatFrame` 编码或格式化输出，返回 `string`。 |
| `cleanStack` | `(raw: string, out: []u8): string` | 调用 `cleanStack` 并返回 `string`。 |

## 最小调用示例

```cp
import { isInternalLocation } from "error-stack-parser";

pub fn main() {
    let result = isInternalLocation("example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
