# spdx@1.0.0 (Agent Quickref)

> Zero-allocation SPDX 2.3 license expression parser, validator, and compatibility evaluator for CP | License: MIT
> Import: `import { ... } from "spdx";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum LicenseCategory`

## Functions
- `export fn lookupLicense(id: string): LicenseInfo?;`
- `export fn validateExpression(expr: string): SpdxResult;`
- `export fn isPermissiveOnly(expr: string): bool;`
- `export fn satisfies(expr: string, allowed: &[]string): bool;`
