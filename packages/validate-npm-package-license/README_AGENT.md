# validate-npm-package-license@1.0.0 (Agent Quickref)

> Zero-allocation npm package license field validator supporting SPDX, UNLICENSED, SEE LICENSE IN, and legacy values for CP. | License: MIT
> Import: `import { ... } from "validate-npm-package-license";` | Full types: `src/index.d.cp`
> Deps: `spdx-expression-parse`

## Functions
- `export fn vnlContainsWhitespace(s: string): bool;`
- `export fn vnlIsLegacyLicense(license: string): bool;`
- `export fn validateLicense(license: string): LicenseValidation;`
- `export fn isValidNewLicense(license: string): bool;`
