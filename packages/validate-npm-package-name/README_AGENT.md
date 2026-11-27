# validate-npm-package-name@1.0.0 (Agent Quickref)

> Zero-heap npm/cap package name validator with scoped @org/pkg rules, reserved names, and legacy vs new validation (npm validate-npm-package-name equivalent) | License: MIT
> Import: `import { ... } from "validate-npm-package-name";` | Full types: `src/index.d.cp`

## Functions
- `export fn vnpStrEq(a: string, b: string): bool;`
- `export fn vnpEqIgnoreCase(a: string, b: string): bool;`
- `export fn vnpIsUrlSafeChar(c: u8): bool;`
- `export fn vnpIsSpecialWarnChar(c: u8): bool;`
- `export fn isBlacklistedName(name: string): bool;`
- `export fn isCoreModuleName(name: string): bool;`
- `export fn vnpCheckSegment(seg: string): u8;`
- `export fn validatePackageName(name: string): PackageNameValidation;`
- `export fn isValidNewPackageName(name: string): bool;`
