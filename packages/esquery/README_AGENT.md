# esquery — AI Agent Quick Ref

Zero-allocation ESTree AST CSS-like selector parser and matcher (npm esquery parity) for CP (`v1.0.0`, `BSD-3-Clause`)

## Import

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

## Core Types & Constants

```cp
enum SelectorKind { Wildcard, NodeType, Attribute, Compound, Descendant, Child, ... }
enum AttrOp { Exists, Eq, NotEq, Lt, Gt }
class AstNodeInfo { node_type: string, name: string, operator: string, parent_idx: i32, /* +1 fields */ }
class SelectorNode { kind: SelectorKind, attr_op: AttrOp, text: string, attr_name: string, /* +3 fields */ }
class ParsedSelector { ok: bool, root: i32, count: usize, error_pos: usize, /* +1 fields */ }
class EsqParser { src: string, pos: usize, has_err: bool, err_pos: usize, /* +2 fields */ }
class EsqIntParse { ok: bool, val: i64 }
```

## API Signatures

```cp
fn esqEmptyNode(): SelectorNode;
fn esqIsWs(ch: u8): bool;
fn esqIsIdentStart(ch: u8): bool;
fn esqIsIdentChar(ch: u8): bool;
fn esqCanStartSimple(ch: u8): bool;
fn esqStrEq(a: string, b: string): bool;
fn esqStrCmp(a: string, b: string): i32;
fn esqParseI64(s: string): EsqIntParse;
fn esqSetErr(p: &mut EsqParser, pos: usize);
fn esqSkipWs(p: &mut EsqParser): bool;
fn esqAddNode(p: &mut EsqParser, node: SelectorNode): i32;
fn esqParseAttr(p: &mut EsqParser): i32;
fn esqParseSimple(p: &mut EsqParser): i32;
fn esqParseCompound(p: &mut EsqParser): i32;
fn esqParseComplex(p: &mut EsqParser): i32;
fn esqParseList(p: &mut EsqParser): i32;
fn parseSelector(sel: string): ParsedSelector;
fn esqGetAttrVal(ast_node: AstNodeInfo, attr_name: string): string;
fn esqMatchAttr(ast_node: AstNodeInfo, sel_node: SelectorNode): bool;
fn esqMatchRec(sel_nodes: &[]SelectorNode, sel_idx: i32, nodes: &[]AstNodeInfo, idx: usize, depth: usize): bool;
fn matchesSelector(sel: ParsedSelector, nodes: &[]AstNodeInfo, idx: usize): bool;
fn queryAll(sel_str: string, nodes: &[]AstNodeInfo, out_indices: []usize): usize;
```

## Minimal Usage

```cp
import { parseSelector } from "esquery";

pub fn main() {
    let result = parseSelector("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"esquery"`; install via `cap add esquery`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
