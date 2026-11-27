# quadtree_rs — AI Agent Quick Ref

Zero-allocation pooled 2D QuadTree spatial index supporting point insertion, automatic subdivision, rectangular range queries, and radius queries for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Rect2,
    QuadItem,
    QuadNode,
    QuadTree,
    qtMakeLeaf,
    qtChildIndex,
} from "quadtree_rs";
```

## Core Types & Constants

```cp
class Rect2 { min_x: f64, min_y: f64, max_x: f64, max_y: f64 } // methods: new(...) +2
class QuadItem { x: f64, y: f64, id: u32, next_item: i32 }
class QuadNode { bounds: Rect2, first_item: i32, count: u32, children: [4]i32, /* +1 fields */ }
class QuadTree { node_count: usize, item_count: usize, max_leaf_items: u32, max_depth: u32 } // methods: empty(...) +4
```

## API Signatures

```cp
fn qtMakeLeaf(bounds: Rect2): QuadNode;
fn qtChildIndex(b: Rect2, x: f64, y: f64): usize;
```

## Minimal Usage

```cp
import { qtMakeLeaf } from "quadtree_rs";

pub fn main() {
    let result = qtMakeLeaf(/* bounds: Rect2 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"quadtree_rs"`; install via `cap add quadtree_rs`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
