[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `validate-npm-package-license`

> **Source Attribution**: `npm:validate-npm-package-license` ([kemitchell/validate-npm-package-license.js](https://github.com/kemitchell/validate-npm-package-license.js)) — License: MIT (upstream: Apache-2.0)

Zero-allocation npm `package.json` / `cap.json` license field validator supporting SPDX expressions, `UNLICENSED`, `SEE LICENSE IN <file>`, legacy identifiers, and OSI-approved / FSF-libre AST evaluation in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`validate-npm-package-license`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: validate-npm-package-license)
import validate from "validate-npm-package-license";
const res = validate("(MIT OR Apache-2.0)");
// { validForNewPackages: true, validForOldPackages: true, spdx: true }
```

```cp
// CP (zero-heap validation + OSI/FSF inspection)
import { validateLicense, inspectLicense } from "validate-npm-package-license";

let res = validateLicense("(MIT OR Apache-2.0)");
// res.valid_for_new_packages == true, res.spdx == true

let info = inspectLicense("(MIT OR Apache-2.0)");
// info.osi_approved == true, info.fsf_libre == true, info.has_compound_expr == true
```

## Installation

Add `validate-npm-package-license` to your `cap.json`:

```json
{
  "dependencies": {
    "validate-npm-package-license": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `LicenseValidation` | `class { valid_for_new_packages: bool, valid_for_old_packages: bool, spdx: bool, unlicensed: bool, in_file: string }` | Standard npm license validation result |
| `LicenseDetails` | `class { valid_for_new_packages: bool, valid_for_old_packages: bool, spdx: bool, unlicensed: bool, in_file: string, osi_approved: bool, fsf_libre: bool, has_compound_expr: bool, has_exception: bool }` | Rich inspection result with OSI/FSF & compound/exception flags |
| `validateLicense` | `(license: string): LicenseValidation` | Validate SPDX, `UNLICENSED`, `SEE LICENSE IN <file>`, or legacy license |
| `inspectLicense` | `(license: string): LicenseDetails` | Full validation + OSI-approved, FSF-libre, compound & exception metadata |
| `isValidNewLicense` | `(license: string): bool` | True if valid for newly published packages |
| `isValidOldLicense` | `(license: string): bool` | True if valid for legacy/existing packages |
| `isOsiApprovedLicense` | `(license_id: string): bool` | Check whether a single SPDX license ID is OSI-approved |
| `isFsfLibreLicense` | `(license_id: string): bool` | Check whether a single SPDX license ID is FSF-libre |
| `isOsiApprovedExpression` | `(expr: string): bool` | Evaluate full SPDX `AND`/`OR` expression for OSI approval |
| `isFsfLibreExpression` | `(expr: string): bool` | Evaluate full SPDX `AND`/`OR` expression for FSF freedom |
| `vnlContainsWhitespace` / `vnlIsLegacyLicense` / `vnlEvalOsiAst` / `vnlEvalFsfAst` | Low-level helpers | Internal legacy check and AST evaluation helpers |

## Practical Examples

### 1. Validating Package License Fields

```cp
import { validateLicense, isValidNewLicense, isValidOldLicense } from "validate-npm-package-license";

let r1 = validateLicense("MIT");
// r1.valid_for_new_packages == true, r1.spdx == true

let r2 = validateLicense("UNLICENSED");
// r2.valid_for_new_packages == true, r2.unlicensed == true

let r3 = validateLicense("SEE LICENSE IN LICENSE.txt");
// r3.valid_for_new_packages == true, r3.in_file == "LICENSE.txt"

let legacy_new = isValidNewLicense("BSD"); // false (deprecated for new packages)
let legacy_old = isValidOldLicense("BSD"); // true
```

### 2. Inspecting OSI-Approved & FSF-Libre Expressions

```cp
import { inspectLicense, isOsiApprovedExpression, isFsfLibreExpression } from "validate-npm-package-license";

let osi = isOsiApprovedExpression("MIT AND Apache-2.0"); // true
let fsf = isFsfLibreExpression("MIT OR GPL-3.0-only");   // true

let details = inspectLicense("Apache-2.0 WITH LLVM-exception");
// details.spdx == true, details.has_exception == true
```

## Notes & Edge Cases

- **`SEE LICENSE IN <file>`**: Requires a non-empty filename without whitespace; `in_file` slices directly into the input string (zero-copy).
- **Boolean AST semantics**: `A AND B` requires both sides to be OSI/FSF compliant; `A OR B` succeeds if either side is compliant.
- **Zero heap allocations**: Powered by `spdx-expression-parse` and `spdx` with fixed-size stack ASTs.
