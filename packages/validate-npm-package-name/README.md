[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# validate-npm-package-name

Zero-heap npm/cap package name validator with scoped @org/pkg rules, reserved names, and legacy vs new validation (npm validate-npm-package-name equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:validate-npm-package-name` ([https://github.com/npm/validate-npm-package-name](https://github.com/npm/validate-npm-package-name))

| Metadata | Value |
| --- | --- |
| Package | `validate-npm-package-name` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add validate-npm-package-name
```

```cp
import {
    PackageNameValidation,
    vnpStrEq,
    vnpEqIgnoreCase,
    vnpIsUrlSafeChar,
    vnpIsSpecialWarnChar,
    isBlacklistedName,
    isCoreModuleName,
    vnpCheckSegment,
} from "validate-npm-package-name";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PackageNameValidation` | `class` | Fields: `valid_for_new: bool, valid_for_old: bool, error_code: u8, warning_code: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `vnpStrEq` | `(a: string, b: string): bool` | Invokes `vnpStrEq` and returns `bool`. |
| `vnpEqIgnoreCase` | `(a: string, b: string): bool` | Invokes `vnpEqIgnoreCase` and returns `bool`. |
| `vnpIsUrlSafeChar` | `(c: u8): bool` | Invokes `vnpIsUrlSafeChar` and returns `bool`. |
| `vnpIsSpecialWarnChar` | `(c: u8): bool` | Invokes `vnpIsSpecialWarnChar` and returns `bool`. |
| `isBlacklistedName` | `(name: string): bool` | Checks predicate `isBlacklistedName` and returns `bool`. |
| `isCoreModuleName` | `(name: string): bool` | Checks predicate `isCoreModuleName` and returns `bool`. |
| `vnpCheckSegment` | `(seg: string): u8` | Invokes `vnpCheckSegment` and returns `u8`. |
| `validatePackageName` | `(name: string): PackageNameValidation` | Validates input with `validatePackageName` returning `PackageNameValidation`. |
| `isValidNewPackageName` | `(name: string): bool` | Checks predicate `isValidNewPackageName` and returns `bool`. |

## Minimal Example

```cp
import { isBlacklistedName } from "validate-npm-package-name";

pub fn main() {
    let result = isBlacklistedName("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
