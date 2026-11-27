# spdx — AI Agent Quick Ref

Zero-allocation SPDX 2.3 license expression parser, validator, and compatibility evaluator for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum LicenseCategory { Permissive, WeakCopyleft, StrongCopyleft, NetworkCopyleft, PublicDomain, Unknown }
enum EvalMode { ValidateOnly, PermissiveOnly, Satisfies }
class LicenseInfo { id: string, category: LicenseCategory, isOsiApproved: bool }
class SpdxResult { valid: bool, allKnown: bool, errorOffset: usize, licenses: [MAX_LICENSES]string, /* +1 fields */ } // methods: getLicense(...)
class Parser { src: string, pos: usize, depth: usize, failed: bool, /* +6 fields */ }
```

## API Signatures

```cp
fn isIdChar(c: u8): bool;
fn isPermissiveCategory(cat: LicenseCategory): bool;
fn lookupBaseLicense(id: string): LicenseInfo?;
fn lookupLicense(id: string): LicenseInfo?;
fn initParser(src: string, mode: EvalMode, allowed: &[]string): Parser;
fn skipSpaces(p: &mut Parser);
fn failAt(p: &mut Parser, offset: usize): bool;
fn isKeyword(tok: string): bool;
fn peekKeyword(p: &mut Parser, kw: string): bool;
fn consumeKeyword(p: &mut Parser, kw: string): bool;
fn recordLicense(p: &mut Parser, lic: string);
fn isValidRefId(s: string): bool;
fn licenseTokenMatches(licBase: string, hasPlus: bool, allowedLic: string): bool;
fn matchAllowedEntry(licBase: string, hasPlus: bool, hasExc: bool, exc: string, entry: string): bool;
fn evalSimple(p: &mut Parser, licBase: string, hasPlus: bool, hasExc: bool, exc: string, fullLic: string): bool;
fn parseSimple(p: &mut Parser): bool;
fn parsePrimary(p: &mut Parser): bool;
fn parseAnd(p: &mut Parser): bool;
fn parseOr(p: &mut Parser): bool;
fn runParse(p: &mut Parser): bool;
fn validateExpression(expr: string): SpdxResult;
fn isPermissiveOnly(expr: string): bool;
fn satisfies(expr: string, allowed: &[]string): bool;
```

## Minimal Usage

```cp
import { isKeyword } from "spdx";

pub fn main() {
    let result = isKeyword("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"spdx"`; install via `cap add spdx`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
