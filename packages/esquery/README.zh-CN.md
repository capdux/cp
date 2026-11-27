[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# esquery

Zero-allocation ESTree AST CSS-like selector parser and matcher (npm esquery parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:esquery` ([https://github.com/estools/esquery](https://github.com/estools/esquery))（原库名 `esquery` → CP 包名 `esquery`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `esquery` |
| 版本 | `1.0.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add esquery
```

```cp
import {
    SelectorKind,
    AttrOp,
    AstNodeInfo,
    SelectorNode,
    ParsedSelector,
    EsqParser,
    EsqIntParse,
    esqEmptyNode,
} from "esquery";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SelectorKind` | `Wildcard, NodeType, Attribute, Compound, Descendant, Child, AdjacentSibling, Matches` |
| `enum` | `AttrOp` | `Exists, Eq, NotEq, Lt, Gt` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AstNodeInfo` | `class` | 字段: `node_type: string, name: string, operator: string, parent_idx: i32, prev_sibling_idx: i32` |
| `SelectorNode` | `class` | 字段: `kind: SelectorKind, attr_op: AttrOp, text: string, attr_name: string, attr_val: string, left: i32, +1 项` |
| `ParsedSelector` | `class` | 字段: `ok: bool, root: i32, count: usize, error_pos: usize, nodes: [64]SelectorNode` |
| `EsqParser` | `class` | 字段: `src: string, pos: usize, has_err: bool, err_pos: usize, count: usize, nodes: [64]SelectorNode` |
| `EsqIntParse` | `class` | 字段: `ok: bool, val: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `esqEmptyNode` | `(): SelectorNode` | 调用 `esqEmptyNode` 并返回 `SelectorNode`。 |
| `esqIsWs` | `(ch: u8): bool` | 调用 `esqIsWs` 并返回 `bool`。 |
| `esqIsIdentStart` | `(ch: u8): bool` | 调用 `esqIsIdentStart` 并返回 `bool`。 |
| `esqIsIdentChar` | `(ch: u8): bool` | 调用 `esqIsIdentChar` 并返回 `bool`。 |
| `esqCanStartSimple` | `(ch: u8): bool` | 调用 `esqCanStartSimple` 并返回 `bool`。 |
| `esqStrEq` | `(a: string, b: string): bool` | 调用 `esqStrEq` 并返回 `bool`。 |
| `esqStrCmp` | `(a: string, b: string): i32` | 调用 `esqStrCmp` 并返回 `i32`。 |
| `esqParseI64` | `(s: string): EsqIntParse` | 调用 `esqParseI64` 并返回 `EsqIntParse`。 |
| `esqSetErr` | `(p: &mut EsqParser, pos: usize)` | 就地执行 `esqSetErr` 操作。 |
| `esqSkipWs` | `(p: &mut EsqParser): bool` | 调用 `esqSkipWs` 并返回 `bool`。 |
| `esqAddNode` | `(p: &mut EsqParser, node: SelectorNode): i32` | 调用 `esqAddNode` 并返回 `i32`。 |
| `esqParseAttr` | `(p: &mut EsqParser): i32` | 调用 `esqParseAttr` 并返回 `i32`。 |
| `esqParseSimple` | `(p: &mut EsqParser): i32` | 调用 `esqParseSimple` 并返回 `i32`。 |
| `esqParseCompound` | `(p: &mut EsqParser): i32` | 调用 `esqParseCompound` 并返回 `i32`。 |
| `esqParseComplex` | `(p: &mut EsqParser): i32` | 调用 `esqParseComplex` 并返回 `i32`。 |
| `esqParseList` | `(p: &mut EsqParser): i32` | 调用 `esqParseList` 并返回 `i32`。 |
| `parseSelector` | `(sel: string): ParsedSelector` | 通过 `parseSelector` 解析输入数据，返回 `ParsedSelector`。 |
| `esqGetAttrVal` | `(ast_node: AstNodeInfo, attr_name: string): string` | 调用 `esqGetAttrVal` 并返回 `string`。 |
| `esqMatchAttr` | `(ast_node: AstNodeInfo, sel_node: SelectorNode): bool` | 调用 `esqMatchAttr` 并返回 `bool`。 |
| `esqMatchRec` | `(sel_nodes: &[]SelectorNode, sel_idx: i32, nodes: &[]AstNodeInfo, idx: usize, depth: usize): bool` | 调用 `esqMatchRec` 并返回 `bool`。 |
| `matchesSelector` | `(sel: ParsedSelector, nodes: &[]AstNodeInfo, idx: usize): bool` | 调用 `matchesSelector` 并返回 `bool`。 |
| `queryAll` | `(sel_str: string, nodes: &[]AstNodeInfo, out_indices: []usize): usize` | 调用 `queryAll` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseSelector } from "esquery";

pub fn main() {
    let result = parseSelector("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
