[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# esquery

Zero-allocation ESTree AST CSS-like selector parser and matcher (npm esquery parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:esquery` ([https://github.com/estools/esquery](https://github.com/estools/esquery))（原库名 `esquery` → CP 包名 `esquery`）

| Metadata | Value |
| --- | --- |
| Package | `esquery` |
| Version | `1.0.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SelectorKind` | `Wildcard, NodeType, Attribute, Compound, Descendant, Child, AdjacentSibling, Matches` |
| `enum` | `AttrOp` | `Exists, Eq, NotEq, Lt, Gt` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AstNodeInfo` | `class` | Fields: `node_type: string, name: string, operator: string, parent_idx: i32, prev_sibling_idx: i32` |
| `SelectorNode` | `class` | Fields: `kind: SelectorKind, attr_op: AttrOp, text: string, attr_name: string, attr_val: string, left: i32, +1 more` |
| `ParsedSelector` | `class` | Fields: `ok: bool, root: i32, count: usize, error_pos: usize, nodes: [64]SelectorNode` |
| `EsqParser` | `class` | Fields: `src: string, pos: usize, has_err: bool, err_pos: usize, count: usize, nodes: [64]SelectorNode` |
| `EsqIntParse` | `class` | Fields: `ok: bool, val: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `esqEmptyNode` | `(): SelectorNode` | Invokes `esqEmptyNode` and returns `SelectorNode`. |
| `esqIsWs` | `(ch: u8): bool` | Invokes `esqIsWs` and returns `bool`. |
| `esqIsIdentStart` | `(ch: u8): bool` | Invokes `esqIsIdentStart` and returns `bool`. |
| `esqIsIdentChar` | `(ch: u8): bool` | Invokes `esqIsIdentChar` and returns `bool`. |
| `esqCanStartSimple` | `(ch: u8): bool` | Invokes `esqCanStartSimple` and returns `bool`. |
| `esqStrEq` | `(a: string, b: string): bool` | Invokes `esqStrEq` and returns `bool`. |
| `esqStrCmp` | `(a: string, b: string): i32` | Invokes `esqStrCmp` and returns `i32`. |
| `esqParseI64` | `(s: string): EsqIntParse` | Invokes `esqParseI64` and returns `EsqIntParse`. |
| `esqSetErr` | `(p: &mut EsqParser, pos: usize)` | Executes `esqSetErr` in-place. |
| `esqSkipWs` | `(p: &mut EsqParser): bool` | Invokes `esqSkipWs` and returns `bool`. |
| `esqAddNode` | `(p: &mut EsqParser, node: SelectorNode): i32` | Invokes `esqAddNode` and returns `i32`. |
| `esqParseAttr` | `(p: &mut EsqParser): i32` | Invokes `esqParseAttr` and returns `i32`. |
| `esqParseSimple` | `(p: &mut EsqParser): i32` | Invokes `esqParseSimple` and returns `i32`. |
| `esqParseCompound` | `(p: &mut EsqParser): i32` | Invokes `esqParseCompound` and returns `i32`. |
| `esqParseComplex` | `(p: &mut EsqParser): i32` | Invokes `esqParseComplex` and returns `i32`. |
| `esqParseList` | `(p: &mut EsqParser): i32` | Invokes `esqParseList` and returns `i32`. |
| `parseSelector` | `(sel: string): ParsedSelector` | Parses input via `parseSelector` and returns `ParsedSelector`. |
| `esqGetAttrVal` | `(ast_node: AstNodeInfo, attr_name: string): string` | Invokes `esqGetAttrVal` and returns `string`. |
| `esqMatchAttr` | `(ast_node: AstNodeInfo, sel_node: SelectorNode): bool` | Invokes `esqMatchAttr` and returns `bool`. |
| `esqMatchRec` | `(sel_nodes: &[]SelectorNode, sel_idx: i32, nodes: &[]AstNodeInfo, idx: usize, depth: usize): bool` | Invokes `esqMatchRec` and returns `bool`. |
| `matchesSelector` | `(sel: ParsedSelector, nodes: &[]AstNodeInfo, idx: usize): bool` | Invokes `matchesSelector` and returns `bool`. |
| `queryAll` | `(sel_str: string, nodes: &[]AstNodeInfo, out_indices: []usize): usize` | Invokes `queryAll` and returns `usize`. |

## Minimal Example

```cp
import { parseSelector } from "esquery";

pub fn main() {
    let result = parseSelector("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
