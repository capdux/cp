[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rehype

Zero-allocation HAST (Hypertext Abstract Syntax Tree) builder & HTML5 serializer (npm rehype/hast parity) composing escape-html for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rehype` ([https://github.com/rehypejs/rehype](https://github.com/rehypejs/rehype))（原库名 `rehype` → CP 包名 `rehype`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rehype` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rehype
```

```cp
import {
    HastAttr,
    HastNode,
    HastTree,
    rhWriteByte,
    isVoidElement,
    rhMakeNode,
    rhSerializeHtml,
    rhExtractText,
} from "rehype";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HastAttr` | `class` | 字段: `name: [48]u8, name_len: usize, value: [192]u8, value_len: usize, next_attr: i32` |
| `HastNode` | `class` | 字段: `kind: u8, text: [256]u8, text_len: usize, first_child: i32, last_child: i32, next_sibling: i32, +2 项` |
| `HastTree` | `class` | 字段: `nodes: [64]HastNode, node_len: usize, attrs: [64]HastAttr, attr_len: usize` · 方法: `new, addElement, addText, addComment, addDoctype, setAttr, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rhWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `rhWriteByte` 并返回 `usize`。 |
| `isVoidElement` | `(tag: string): bool` | 执行 `isVoidElement` 条件判定，返回 `bool`。 |
| `rhMakeNode` | `(kind: u8, text: string): HastNode` | 调用 `rhMakeNode` 并返回 `HastNode`。 |
| `rhSerializeHtml` | `(nodes: &[]HastNode, attrs: &[]HastAttr, node_id: usize, out: []u8, pos: usize, depth: usize): usize` | 调用 `rhSerializeHtml` 并返回 `usize`。 |
| `rhExtractText` | `(nodes: &[]HastNode, node_id: usize, out: []u8, pos: usize, depth: usize): usize` | 调用 `rhExtractText` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isVoidElement } from "rehype";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
