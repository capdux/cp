# `validate-npm-package-license` — AI Agent Quick Reference

- **Package**: `validate-npm-package-license` (`1.0.0`, MIT)
- **Origin**: `npm:validate-npm-package-license`
- **Import**: `import { validateLicense, inspectLicense, isValidNewLicense, isValidOldLicense, isOsiApprovedLicense, isFsfLibreLicense, isOsiApprovedExpression, isFsfLibreExpression, LicenseValidation, LicenseDetails } from "validate-npm-package-license";`

## Types

```cp
class LicenseValidation {
    valid_for_new_packages: bool,
    valid_for_old_packages: bool,
    spdx: bool,
    unlicensed: bool,
    in_file: string,
}
class LicenseDetails {
    valid_for_new_packages: bool,
    valid_for_old_packages: bool,
    spdx: bool,
    unlicensed: bool,
    in_file: string,
    osi_approved: bool,
    fsf_libre: bool,
    has_compound_expr: bool,
    has_exception: bool,
}
```

## Core Signatures

```cp
fn validateLicense(license: string): LicenseValidation
fn inspectLicense(license: string): LicenseDetails
fn isValidNewLicense(license: string): bool
fn isValidOldLicense(license: string): bool
fn isOsiApprovedLicense(license_id: string): bool
fn isFsfLibreLicense(license_id: string): bool
fn isOsiApprovedExpression(expr: string): bool
fn isFsfLibreExpression(expr: string): bool
```

## Minimal Pattern

```cp
let info = inspectLicense("(MIT OR Apache-2.0)");
if (info.valid_for_new_packages && info.osi_approved) { /* compliant */ }
```

## Pitfalls

1. **Case-sensitive SPDX**: `"mit"` is invalid for new packages; use canonical SPDX IDs like `"MIT"` or `"Apache-2.0"`.
2. **`SEE LICENSE IN <file>`**: Filename must contain no whitespace; `in_file` is a zero-copy slice of `license`.
3. **Legacy values**: `"BSD"`, `"GPL"`, `"Apache"` return `valid_for_old_packages = true` but `valid_for_new_packages = false`.
4. **Boolean semantics**: `isOsiApprovedExpression` / `isFsfLibreExpression` evaluate `AND` as `&&` and `OR` as `||`.
