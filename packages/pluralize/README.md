[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pluralize

Zero-allocation English pluralization, singularization, irregular/uncountable dictionary, count formatting & ordinal engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pluralize` ([https://github.com/plurals/pluralize](https://github.com/plurals/pluralize))

| Metadata | Value |
| --- | --- |
| Package | `pluralize` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pluralize
```

```cp
import {
    isVowelLower,
    detectCaseStyle,
    copySlice,
    writeWithCase,
    writeStemAndSuffix,
    inList,
    isUncountable,
    plural,
} from "pluralize";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isVowelLower` | `(c: u8): bool` | Checks predicate `isVowelLower` and returns `bool`. |
| `detectCaseStyle` | `(word: &[]u8): u8` | Invokes `detectCaseStyle` and returns `u8`. |
| `copySlice` | `(out: []u8, src: &[]u8): &[]u8` | Invokes `copySlice` and returns `&[]u8`. |
| `writeWithCase` | `(out: []u8, src_lower: &[]u8, style: u8): &[]u8` | Invokes `writeWithCase` and returns `&[]u8`. |
| `writeStemAndSuffix` | `(out: []u8, stem: &[]u8, suffix_lower: &[]u8, all_upper: bool): &[]u8` | Invokes `writeStemAndSuffix` and returns `&[]u8`. |
| `inList` | `(lower: &[]u8, list: &[]&[]u8): bool` | Invokes `inList` and returns `bool`. |
| `isUncountable` | `(word: &[]u8): bool` | Checks predicate `isUncountable` and returns `bool`. |
| `plural` | `(word: &[]u8, out: []u8): &[]u8` | Invokes `plural` and returns `&[]u8`. |
| `singular` | `(word: &[]u8, out: []u8): &[]u8` | Invokes `singular` and returns `&[]u8`. |
| `isPlural` | `(word: &[]u8): bool` | Checks predicate `isPlural` and returns `bool`. |
| `isSingular` | `(word: &[]u8): bool` | Checks predicate `isSingular` and returns `bool`. |
| `pluralizeCount` | `(word: &[]u8, count: i64, out: []u8): &[]u8` | Invokes `pluralizeCount` and returns `&[]u8`. |
| `ordinal` | `(n: i64, out: []u8): &[]u8` | Invokes `ordinal` and returns `&[]u8`. |

## Minimal Example

```cp
import { isVowelLower } from "pluralize";

pub fn main() {
    let result = isVowelLower(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
