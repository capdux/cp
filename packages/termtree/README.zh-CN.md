[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# termtree

Zero-allocation CLI tree structure renderer with Unicode/ASCII glyph palettes and fixed-capacity arena (Rust termtree parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:termtree` ([https://github.com/rust-cli/termtree](https://github.com/rust-cli/termtree))

| 元信息 | 值 |
| --- | --- |
| 包名 | `termtree` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add termtree
```

```cp
import {
    GlyphPalette,
    TermTreeNode,
    TermTreeArena,
    ttrAppendBytes,
    ttrAppendByte,
} from "termtree";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GlyphPalette` | `class` | 字段: `middle_item: string, last_item: string, middle_skip: string, last_skip: string` · 方法: `unicode, ascii` |
| `TermTreeNode` | `class` | 字段: `label: string, first_child: i32, last_child: i32, next_sibling: i32` |
| `TermTreeArena` | `class` | 字段: `nodes: [64]TermTreeNode, count: usize` · 方法: `init, newRoot, addChild, nodeCount, maxDepth, renderWithGlyphs, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ttrAppendBytes` | `(out: []u8, pos: usize, src: string): usize?` | 调用 `ttrAppendBytes` 并返回 `usize?`。 |
| `ttrAppendByte` | `(out: []u8, pos: usize, b: u8): usize?` | 调用 `ttrAppendByte` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { ttrAppendBytes } from "termtree";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ttrAppendBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
