[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ansi-regex

Zero-heap ANSI escape sequence matcher & locator reusing anstyle-parse, matching npm ansi-regex (400M+/wk) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ansi-regex` ([https://github.com/chalk/ansi-regex](https://github.com/chalk/ansi-regex))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ansi-regex` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ansi-regex
```

```cp
import {
    AnsiSpan,
    testAnsi,
    findFirstAnsi,
    findAllAnsi,
    countAnsiSequences,
    stripAllAnsi,
} from "ansi-regex";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AnsiSpan` | `class` | 字段: `start: usize, len: usize, found: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `testAnsi` | `(input: string): bool` | 调用 `testAnsi` 并返回 `bool`。 |
| `findFirstAnsi` | `(input: string): AnsiSpan` | 查询或读取目标数据，返回 `AnsiSpan`。 |
| `findAllAnsi` | `(input: string, out: []AnsiSpan): usize` | 查询或读取目标数据，返回 `usize`。 |
| `countAnsiSequences` | `(input: string): usize` | 调用 `countAnsiSequences` 并返回 `usize`。 |
| `stripAllAnsi` | `(input: string, out: []u8): usize` | 调用 `stripAllAnsi` 并返回 `usize`。 |

## 最小调用示例

```cp
import { testAnsi } from "ansi-regex";

pub fn main() {
    let result = testAnsi("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
