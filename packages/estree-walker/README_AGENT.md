# estree-walker — AI Agent Quick Ref

Zero-allocation index-pooled ESTree AST arena and non-recursive DFS walker with skip, remove, and replace for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EstreeNodeKind,
    WalkPhase,
    WalkEvent,
    EstreeNode,
    EstwFrame,
    EstreeArena,
    EstreeWalker,
    estwValidId,
} from "estree-walker";
```

## Core Types & Constants

```cp
enum EstreeNodeKind { Program, Identifier, Literal, BinaryExpression, CallExpression, FunctionDeclaration, ... }
enum WalkPhase { Enter, Leave }
class WalkEvent { phase: WalkPhase, node_id: i32, parent_id: i32, depth: usize }
class EstreeNode { kind: EstreeNodeKind, name: string, first_child: i32, next_sibling: i32, /* +2 fields */ }
class EstwFrame { node_id: i32, next_child: i32, entered: bool, skipped: bool }
class EstreeArena { count: usize, nodes: [64]EstreeNode } // methods: init(...) +5
class EstreeWalker { sp: usize, frames: [64]EstwFrame } // methods: init(...) +2
```

## API Signatures

```cp
fn estwValidId(id: i32, count: usize): bool;
```

## Minimal Usage

```cp
import { estwValidId } from "estree-walker";

pub fn main() {
    let result = estwValidId(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"estree-walker"`; install via `cap add estree-walker`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
