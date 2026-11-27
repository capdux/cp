[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# doctrine

Zero-allocation JSDoc comment parser extracting description and @param/@returns/@type/@deprecated/@throws/@template tags (npm doctrine parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:doctrine` ([https://github.com/eslint/doctrine](https://github.com/eslint/doctrine))（原库名 `doctrine` → CP 包名 `doctrine`）

| Metadata | Value |
| --- | --- |
| Package | `doctrine` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add doctrine
```

```cp
import {
    JsDocTag,
    JsDocComment,
    docIsSpace,
    docStripLinePrefix,
    docTagHasName,
    docParseTagLine,
    parseJsDoc,
} from "doctrine";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JsDocTag` | `class` | Fields: `title: string, type_expr: string, name: string, description: string, optional: bool` |
| `JsDocComment` | `class` | Fields: `description: string, tags: [16]JsDocTag, tag_count: usize` · Methods: `tagCount, getTag, findTag, isDeprecated` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `docIsSpace` | `(ch: u8): bool` | Invokes `docIsSpace` and returns `bool`. |
| `docStripLinePrefix` | `(line: string): string` | Invokes `docStripLinePrefix` and returns `string`. |
| `docTagHasName` | `(title: string): bool` | Invokes `docTagHasName` and returns `bool`. |
| `docParseTagLine` | `(line: string): JsDocTag` | Invokes `docParseTagLine` and returns `JsDocTag`. |
| `parseJsDoc` | `(comment: string): JsDocComment` | Parses input via `parseJsDoc` and returns `JsDocComment`. |

## Minimal Example

```cp
import { parseJsDoc } from "doctrine";

pub fn main() {
    let result = parseJsDoc("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
