[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# calver

Zero-allocation Calendar Versioning (CalVer) parser, validator, comparator, bumper, and formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:calver` ([https://github.com/muratgozel/node-calver](https://github.com/muratgozel/node-calver))

| Metadata | Value |
| --- | --- |
| Package | `calver` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add calver
```

```cp
import {
    CalverFormat,
    Calver,
    ModifierParts,
    cmpIgnoreCase,
    isLeapYear,
    daysInMonth,
    parseU32Strict,
    parseModifierTail,
} from "calver";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CalverFormat` | `YyyyMmMicro, YyMmMicro, Yyyy0mMicro, YyyyMmDd, YyyyMmDdMicro` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Calver` | `class` | Fields: `valid: bool, year: u16, month: u8, day: u8, micro: u32, modifier: string, +1 more` · Methods: `invalid` |
| `ModifierParts` | `class` | Fields: `ok: bool, name: string, num: u32` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmpIgnoreCase` | `(a: string, b: string): i32` | Invokes `cmpIgnoreCase` and returns `i32`. |
| `isLeapYear` | `(year: u16): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(year: u16, month: u8): u8` | Invokes `daysInMonth` and returns `u8`. |
| `parseU32Strict` | `(s: string): u32?` | Parses input via `parseU32Strict` and returns `u32?`. |
| `parseModifierTail` | `(s: string): ModifierParts` | Parses input via `parseModifierTail` and returns `ModifierParts`. |
| `parseCalverCore` | `(raw: string, force_day_on_3: bool): Calver` | Parses input via `parseCalverCore` and returns `Calver`. |
| `parseCalver` | `(raw: string): Calver` | Parses input via `parseCalver` and returns `Calver`. |
| `parseCalverWith` | `(raw: string, fmt: CalverFormat): Calver` | Parses input via `parseCalverWith` and returns `Calver`. |
| `modifierRank` | `(name: string): u8` | Invokes `modifierRank` and returns `u8`. |
| `compareCalver` | `(a: Calver, b: Calver): i32` | Invokes `compareCalver` and returns `i32`. |
| `bumpCalver` | `(cur: Calver, year: u16, month: u8, day: u8, use_day: bool): Calver` | Invokes `bumpCalver` and returns `Calver`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `append2Digits` | `(out: []u8, pos: usize, val: u32): usize` | Invokes `append2Digits` and returns `usize`. |
| `formatCalver` | `(v: Calver, fmt: CalverFormat, out: []u8): string` | Serializes/formats data via `formatCalver` returning `string`. |

## Minimal Example

```cp
import { parseU32Strict } from "calver";

pub fn main() {
    let result = parseU32Strict("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
