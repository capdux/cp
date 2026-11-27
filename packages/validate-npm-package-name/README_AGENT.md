# validate-npm-package-name — AI Agent Quick Ref

Zero-heap npm/cap package name validator with scoped @org/pkg rules, reserved names, and legacy vs new validation (npm validate-npm-package-name equivalent) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class PackageNameValidation { valid_for_new: bool, valid_for_old: bool, error_code: u8, warning_code: u8 }
```

## API Signatures

```cp
fn vnpStrEq(a: string, b: string): bool;
fn vnpEqIgnoreCase(a: string, b: string): bool;
fn vnpIsUrlSafeChar(c: u8): bool;
fn vnpIsSpecialWarnChar(c: u8): bool;
fn isBlacklistedName(name: string): bool;
fn isCoreModuleName(name: string): bool;
fn vnpCheckSegment(seg: string): u8;
fn validatePackageName(name: string): PackageNameValidation;
fn isValidNewPackageName(name: string): bool;
```

## Minimal Usage

```cp
import { isBlacklistedName } from "validate-npm-package-name";

pub fn main() {
    let result = isBlacklistedName("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"validate-npm-package-name"`; install via `cap add validate-npm-package-name`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
