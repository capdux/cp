[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# spdx

Zero-allocation SPDX 2.3 license expression parser, validator, and compatibility evaluator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`spdx`](https://github.com/EmbarkStudios/spdx)（`MIT OR Apache-2.0`）（包名 `spdx`）。

| Metadata | Value |
| --- | --- |
| Package | `spdx` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add spdx
```

```cp
import {
    LicenseCategory,
    EvalMode,
    LicenseInfo,
    SpdxResult,
    Parser,
    isIdChar,
    isPermissiveCategory,
    lookupBaseLicense,
} from "spdx";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `LicenseCategory` | `Permissive, WeakCopyleft, StrongCopyleft, NetworkCopyleft, PublicDomain, Unknown` |
| `enum` | `EvalMode` | `ValidateOnly, PermissiveOnly, Satisfies` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LicenseInfo` | `class` | Fields: `id: string, category: LicenseCategory, isOsiApproved: bool` |
| `SpdxResult` | `class` | Fields: `valid: bool, allKnown: bool, errorOffset: usize, licenses: [MAX_LICENSES]string, licenseCount: usize` · Methods: `getLicense` |
| `Parser` | `class` | Fields: `src: string, pos: usize, depth: usize, failed: bool, errorOffset: usize, allKnown: bool, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isIdChar` | `(c: u8): bool` | Checks predicate `isIdChar` and returns `bool`. |
| `isPermissiveCategory` | `(cat: LicenseCategory): bool` | Checks predicate `isPermissiveCategory` and returns `bool`. |
| `lookupBaseLicense` | `(id: string): LicenseInfo?` | Queries/retrieves data via `lookupBaseLicense` returning `LicenseInfo?`. |
| `lookupLicense` | `(id: string): LicenseInfo?` | Queries/retrieves data via `lookupLicense` returning `LicenseInfo?`. |
| `initParser` | `(src: string, mode: EvalMode, allowed: &[]string): Parser` | Constructs/initializes an instance via `initParser`. |
| `skipSpaces` | `(p: &mut Parser)` | Executes `skipSpaces` in-place. |
| `failAt` | `(p: &mut Parser, offset: usize): bool` | Invokes `failAt` and returns `bool`. |
| `isKeyword` | `(tok: string): bool` | Checks predicate `isKeyword` and returns `bool`. |
| `peekKeyword` | `(p: &mut Parser, kw: string): bool` | Invokes `peekKeyword` and returns `bool`. |
| `consumeKeyword` | `(p: &mut Parser, kw: string): bool` | Invokes `consumeKeyword` and returns `bool`. |
| `recordLicense` | `(p: &mut Parser, lic: string)` | Executes `recordLicense` in-place. |
| `isValidRefId` | `(s: string): bool` | Checks predicate `isValidRefId` and returns `bool`. |
| `licenseTokenMatches` | `(licBase: string, hasPlus: bool, allowedLic: string): bool` | Invokes `licenseTokenMatches` and returns `bool`. |
| `matchAllowedEntry` | `(licBase: string, hasPlus: bool, hasExc: bool, exc: string, entry: string): bool` | Invokes `matchAllowedEntry` and returns `bool`. |
| `evalSimple` | `(p: &mut Parser, licBase: string, hasPlus: bool, hasExc: bool, exc: string, fullLic: string): bool` | Invokes `evalSimple` and returns `bool`. |
| `parseSimple` | `(p: &mut Parser): bool` | Parses input via `parseSimple` and returns `bool`. |
| `parsePrimary` | `(p: &mut Parser): bool` | Parses input via `parsePrimary` and returns `bool`. |
| `parseAnd` | `(p: &mut Parser): bool` | Parses input via `parseAnd` and returns `bool`. |
| `parseOr` | `(p: &mut Parser): bool` | Parses input via `parseOr` and returns `bool`. |
| `runParse` | `(p: &mut Parser): bool` | Invokes `runParse` and returns `bool`. |
| `validateExpression` | `(expr: string): SpdxResult` | Validates input with `validateExpression` returning `SpdxResult`. |
| `isPermissiveOnly` | `(expr: string): bool` | Checks predicate `isPermissiveOnly` and returns `bool`. |
| `satisfies` | `(expr: string, allowed: &[]string): bool` | Invokes `satisfies` and returns `bool`. |

## Minimal Example

```cp
import { isKeyword } from "spdx";

pub fn main() {
    let result = isKeyword("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
