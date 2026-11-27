[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# line-column

Zero-allocation byte offset <-> 1-based (line, col) locator, zero-copy line extractor, and LF/CRLF line counter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:line-column` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:line-index` ([https://github.com/rust-lang/rust-analyzer](https://github.com/rust-lang/rust-analyzer))（原库名 `line-column` → CP 包名 `line-column`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `line-column` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add line-column
```

```cp
import {
    LineCol,
    offsetToLineCol,
    lineColToOffset,
    getLine,
    countLines,
} from "line-column";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LineCol` | `class` | 字段: `line: usize, col: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `offsetToLineCol` | `(text: string, offset: usize): LineCol` | 调用 `offsetToLineCol` 并返回 `LineCol`。 |
| `lineColToOffset` | `(text: string, line: usize, col: usize): i64` | 调用 `lineColToOffset` 并返回 `i64`。 |
| `getLine` | `(text: string, line_1based: usize): string` | 查询或读取目标数据，返回 `string`。 |
| `countLines` | `(text: string): usize` | 调用 `countLines` 并返回 `usize`。 |

## 最小调用示例

```cp
import { getLine } from "line-column";

pub fn main() {
    let result = getLine("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
