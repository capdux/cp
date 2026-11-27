[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# markdown-table

Zero-heap GFM Markdown table renderer with automatic column width calculation and alignment padding

| 元信息 | 值 |
| --- | --- |
| 包名 | `markdown-table` |
| 版本 | `3.0.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add markdown-table
```

```cp
import {
    TableAlign,
    mdtbWriteByte,
    mdtbWriteStr,
    mdtbWriteRepeat,
    mdtbGetAlign,
    mdtbMinWidth,
    mdtbWriteCell,
    mdtbWriteDelim,
} from "markdown-table";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `TableAlign` | `None, Left, Center, Right` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mdtbWriteByte` | `(out: []u8, pos: usize, ch: u8): usize` | 调用 `mdtbWriteByte` 并返回 `usize`。 |
| `mdtbWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `mdtbWriteStr` 并返回 `usize`。 |
| `mdtbWriteRepeat` | `(out: []u8, pos: usize, ch: u8, count: usize): usize` | 调用 `mdtbWriteRepeat` 并返回 `usize`。 |
| `mdtbGetAlign` | `(aligns: &[]TableAlign, col: usize): TableAlign` | 调用 `mdtbGetAlign` 并返回 `TableAlign`。 |
| `mdtbMinWidth` | `(al: TableAlign): usize` | 调用 `mdtbMinWidth` 并返回 `usize`。 |
| `mdtbWriteCell` | `(out: []u8, pos: usize, text: string, width: usize, al: TableAlign): usize` | 调用 `mdtbWriteCell` 并返回 `usize`。 |
| `mdtbWriteDelim` | `(out: []u8, pos: usize, width: usize, al: TableAlign): usize` | 调用 `mdtbWriteDelim` 并返回 `usize`。 |
| `mdtbColWidth` | `(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, col: usize, al: TableAlign): usize` | 调用 `mdtbColWidth` 并返回 `usize`。 |
| `renderMarkdownTable` | `(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, aligns: &[]TableAlign, out: []u8): usize` | 调用 `renderMarkdownTable` 并返回 `usize`。 |

## 最小调用示例

```cp
import { mdtbWriteStr } from "markdown-table";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = mdtbWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
