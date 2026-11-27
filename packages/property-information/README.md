[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# property-information

Zero-allocation HTML/SVG attribute and DOM property name mapping table

| Metadata | Value |
| --- | --- |
| Package | `property-information` |
| Version | `6.5.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add property-information
```

```cp
import {
    PropInfo,
    pinfMake,
    pinfNotFound,
    pinfMatchPair,
    findHtmlProp,
    isBooleanAttr,
    isSpaceSeparatedAttr,
    isCommaSeparatedAttr,
} from "property-information";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PropInfo` | `class` | Fields: `property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pinfMake` | `(property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool): PropInfo` | Invokes `pinfMake` and returns `PropInfo`. |
| `pinfNotFound` | `(name: string): PropInfo` | Invokes `pinfNotFound` and returns `PropInfo`. |
| `pinfMatchPair` | `(name: string, prop: string, attr: string): bool` | Invokes `pinfMatchPair` and returns `bool`. |
| `findHtmlProp` | `(name: string): PropInfo` | Queries/retrieves data via `findHtmlProp` returning `PropInfo`. |
| `isBooleanAttr` | `(name: string): bool` | Checks predicate `isBooleanAttr` and returns `bool`. |
| `isSpaceSeparatedAttr` | `(name: string): bool` | Checks predicate `isSpaceSeparatedAttr` and returns `bool`. |
| `isCommaSeparatedAttr` | `(name: string): bool` | Checks predicate `isCommaSeparatedAttr` and returns `bool`. |
| `attrToDomProp` | `(attr: string): string` | Invokes `attrToDomProp` and returns `string`. |
| `domPropToAttr` | `(prop: string): string` | Invokes `domPropToAttr` and returns `string`. |

## Minimal Example

```cp
import { isBooleanAttr } from "property-information";

pub fn main() {
    let result = isBooleanAttr("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
