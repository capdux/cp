# spdx-expression-parse — AI Agent Quick Ref

Zero-allocation SPDX license expression recursive-descent AST parser and evaluator for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SpdxAstNode,
    SpdxAst,
    SpdxpState,
    spdxpEmptyNode,
    spdxpEmptyAst,
    spdxpIsIdChar,
    spdxpIsKeyword,
    spdxpIsAllIdChars,
} from "spdx-expression-parse";
```

## Core Types & Constants

```cp
class SpdxAstNode { kind: u8, license: string, exception: string, plus: bool, /* +2 fields */ }
class SpdxAst { nodes: [32]SpdxAstNode, root: i32, node_count: usize, valid: bool }
class SpdxpState { src: string, pos: usize, depth: usize, failed: bool, /* +1 fields */ }
```

## API Signatures

```cp
fn spdxpEmptyNode(): SpdxAstNode;
fn spdxpEmptyAst(): SpdxAst;
fn spdxpIsIdChar(ch: u8): bool;
fn spdxpIsKeyword(tok: string): bool;
fn spdxpIsAllIdChars(s: string): bool;
fn spdxpIsValidLicenseRef(id: string): bool;
fn spdxpIsValidLicenseId(id: string): bool;
fn spdxpIsValidExceptionId(exc: string): bool;
fn spdxpSkipSpaces(st: &mut SpdxpState);
fn spdxpAllocNode(st: &mut SpdxpState, node: SpdxAstNode): i32;
fn spdxpConsumeKeyword(st: &mut SpdxpState, kw: string): bool;
fn spdxpParseSimple(st: &mut SpdxpState): i32;
fn spdxpParsePrimary(st: &mut SpdxpState): i32;
fn spdxpParseAnd(st: &mut SpdxpState): i32;
fn spdxpParseOr(st: &mut SpdxpState): i32;
fn parseSpdxExpression(expr: string): SpdxAst;
fn isValidSpdxExpression(expr: string): bool;
fn spdxpMatchLicenseNode(node: SpdxAstNode, allowed: string): bool;
fn spdxpEvalAst(ast: SpdxAst, idx: i32, allowed: string, depth: usize): bool;
fn satisfiesSpdx(ast: SpdxAst, allowed_license: string): bool;
```

## Minimal Usage

```cp
import { parseSpdxExpression } from "spdx-expression-parse";

pub fn main() {
    let result = parseSpdxExpression("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"spdx-expression-parse"`; install via `cap add spdx-expression-parse`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
