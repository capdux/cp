[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# codespan-reporting

Zero-heap Rustc-style multi-line source code diagnostic renderer with line numbers, gutter, and caret underlines

| 元信息 | 值 |
| --- | --- |
| 包名 | `codespan-reporting` |
| 版本 | `0.11.1` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add codespan-reporting
```

```cp
import {
    DiagSeverity,
    LineCol,
    Diagnostic,
    csrDigitWidth,
    severityLabel,
    locateOffset,
    renderDiagnostic,
} from "codespan-reporting";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `DiagSeverity` | `Bug, Error, Warning, Note, Help` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LineCol` | `class` | 字段: `line: usize, col: usize, line_start: usize, line_end: usize` |
| `Diagnostic` | `class` | 字段: `severity: DiagSeverity, code: string, message: string, file_name: string, span_start: usize, span_end: usize, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `csrDigitWidth` | `(val: usize): usize` | 调用 `csrDigitWidth` 并返回 `usize`。 |
| `severityLabel` | `(sev: DiagSeverity): string` | 调用 `severityLabel` 并返回 `string`。 |
| `locateOffset` | `(source: string, offset: usize): LineCol` | 调用 `locateOffset` 并返回 `LineCol`。 |
| `renderDiagnostic` | `(source: string, diag: Diagnostic, out: []u8): usize` | 调用 `renderDiagnostic` 并返回 `usize`。 |

## 最小调用示例

```cp
import { locateOffset } from "codespan-reporting";

pub fn main() {
    let result = locateOffset("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
