[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# esquery

> 版本：`1.0.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ESTree AST CSS-like selector parser and matcher (npm esquery parity) for CP

## 安装与导入

```bash
cap add esquery
```

```cp
import { esqEmptyNode } from "esquery";
```

## 核心导出 API

- `export enum SelectorKind`
- `export enum AttrOp`
- `export fn esqEmptyNode(): SelectorNode;`
- `export fn esqIsWs(ch: u8): bool;`
- `export fn esqIsIdentStart(ch: u8): bool;`
- `export fn esqIsIdentChar(ch: u8): bool;`
- `export fn esqCanStartSimple(ch: u8): bool;`
- `export fn esqStrEq(a: string, b: string): bool;`
- `export fn esqStrCmp(a: string, b: string): i32;`
- `export fn esqParseI64(s: string): EsqIntParse;`
- `export fn esqSetErr(p: &mut EsqParser, pos: usize);`
- `export fn esqSkipWs(p: &mut EsqParser): bool;`
- `export fn esqAddNode(p: &mut EsqParser, node: SelectorNode): i32;`
- `export fn esqParseAttr(p: &mut EsqParser): i32;`
- `export fn esqParseSimple(p: &mut EsqParser): i32;`
- `export fn esqParseCompound(p: &mut EsqParser): i32;`
- `export fn esqParseComplex(p: &mut EsqParser): i32;`
- `export fn esqParseList(p: &mut EsqParser): i32;`
- `export fn parseSelector(sel: string): ParsedSelector;`
- `export fn esqGetAttrVal(ast_node: AstNodeInfo, attr_name: string): string;`
- `export fn esqMatchAttr(ast_node: AstNodeInfo, sel_node: SelectorNode): bool;`
- `export fn esqMatchRec(sel_nodes: &[]SelectorNode, sel_idx: i32, nodes: &[]AstNodeInfo, idx: usize, depth: usize): bool;`
- `export fn matchesSelector(sel: ParsedSelector, nodes: &[]AstNodeInfo, idx: usize): bool;`
- `export fn queryAll(sel_str: string, nodes: &[]AstNodeInfo, out_indices: []usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`esquery@1.0.0`)，许可证：BSD-3-Clause
