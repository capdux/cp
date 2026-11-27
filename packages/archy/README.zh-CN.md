[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# archy

Zero-allocation Unicode & ASCII dependency/directory tree renderer (archy / treeify / Rust ptree parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:archy` ([https://github.com/substack/node-archy](https://github.com/substack/node-archy))（原库名 `archy` → CP 包名 `archy`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `archy` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add archy
```

```cp
import {
    TreeStyle,
    TreeNode,
    TreeBuilder,
    appendStr,
    appendByte,
    branchStr,
    guideStr,
    renderNodeRec,
} from "archy";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `TreeStyle` | `Unicode, Rounded, Ascii` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TreeNode` | `class` | 字段: `label: string, parent: i32, first_child: i32, last_child: i32, next_sibling: i32` |
| `TreeBuilder` | `class` | 字段: `nodes: [MAX_NODES]TreeNode, count: usize` · 方法: `init, addChild, nodeCount` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `branchStr` | `(style: TreeStyle, is_last: bool): string` | 调用 `branchStr` 并返回 `string`。 |
| `guideStr` | `(style: TreeStyle, has_more: bool): string` | 调用 `guideStr` 并返回 `string`。 |
| `renderNodeRec` | `(tree: &mut TreeBuilder, node_idx: usize, depth: usize, ancestors_have_next: &mut [MAX_DEPTH]bool, style: TreeStyle, out: []u8, pos_in: usize): usize` | 调用 `renderNodeRec` 并返回 `usize`。 |
| `renderTreeWith` | `(tree: &mut TreeBuilder, style: TreeStyle, out: []u8): string` | 调用 `renderTreeWith` 并返回 `string`。 |
| `renderTree` | `(tree: &mut TreeBuilder, out: []u8): string` | 调用 `renderTree` 并返回 `string`。 |

## 最小调用示例

```cp
import { appendStr } from "archy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
