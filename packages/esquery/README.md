[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# esquery

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:esquery` ([https://github.com/estools/esquery](https://github.com/estools/esquery))（原库名 `esquery` → CP 包名 `esquery`）

Zero-allocation ESTree AST CSS-like selector parser and matcher for CP — clean-room BSD-3-Clause implementation inspired by `esquery` (used by ESLint).

纯 CP 零堆分配 AST CSS-like 选择器解析与匹配引擎（对标 ESLint 核心 `esquery`），支持节点类型、通配符 `*`、属性过滤 `[attr=val]`、复合选择、后代选择 `A B`、直接子代 `A > B`、相邻兄弟 `A + B` 以及多选组合 `A, B`。

## Install / 安装

```bash
cap add esquery
```

## API

### Enums & Structs / 核心类型

| Symbol | Description |
| --- | --- |
| `SelectorKind` | `Wildcard`, `NodeType`, `Attribute`, `Compound`, `Descendant`, `Child`, `AdjacentSibling`, `Matches` |
| `AttrOp` | `Exists`, `Eq`, `NotEq`, `Lt`, `Gt` |
| `AstNodeInfo` | Flat AST node descriptor (`node_type`, `name`, `operator`, `parent_idx`, `prev_sibling_idx`) |
| `SelectorNode` | Individual parsed selector AST node stored in fixed-capacity pool |
| `ParsedSelector` | Zero-allocation parsed selector tree (`ok`, `root`, `count`, `error_pos`, `nodes`) |

### Functions / 核心函数

| Symbol | Signature | Description |
| --- | --- | --- |
| `parseSelector` | `(sel: string): ParsedSelector` | Parse CSS-like AST selector string into a fixed-size AST |
| `matchesSelector` | `(sel: ParsedSelector, nodes: []const AstNodeInfo, idx: usize): bool` | Test whether AST node at `idx` matches `sel` |
| `queryAll` | `(sel_str: string, nodes: []const AstNodeInfo, out_indices: []usize): usize` | Query all matching node indices into `out_indices` and return match count |

## Usage / 使用示例

```cp
import { AstNodeInfo, parseSelector, matchesSelector, queryAll } from "esquery";

const nodes = [
    AstNodeInfo{ node_type: "FunctionDeclaration", name: "greet", operator: "", parent_idx: -1, prev_sibling_idx: -1 },
    AstNodeInfo{ node_type: "CallExpression", name: "", operator: "", parent_idx: 0, prev_sibling_idx: -1 },
    AstNodeInfo{ node_type: "Identifier", name: "log", operator: "", parent_idx: 1, prev_sibling_idx: -1 },
];

let out: [8]usize = undefined;
const count = queryAll("CallExpression > Identifier[name=\"log\"]", nodes[0..3], out[0..8]);
// count == 1, out[0] == 2
```
