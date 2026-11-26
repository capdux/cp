# @cap/semver-range

Zero-allocation npm / Cargo SemVer version range parser and matcher for CP.

## Features

- **Zero heap allocation**: pure CP streaming evaluation over `string` slices.
- **Full npm / Cargo range syntax**:
  - Caret (`^1.2.3`, `^0.2.3`, `^0.0.3`), Tilde (`~1.2.3`, `~1.2`).
  - Relational operators (`=`, `>`, `>=`, `<`, `<=`) and space/comma AND sets (`>=1.0.0 <2.0.0`, `>=1.0.0, <2.0.0`).
  - Logical OR branches (`1.0.0 || >=2.0.0 <3.0.0`).
  - Wildcards (`*`, `1.*`, `1.2.x`, `1.2.X`).
- **Resolution & validation**: `parseVersion`, `compareVersion`, `satisfies`, `isValidRange`, and `maxSatisfyingIndex`.

## Usage

```cp
import { satisfies, maxSatisfyingIndex } from "@cap/semver-range";

const ok = satisfies("1.5.2", "^1.2.3"); // true
const bad = satisfies("2.0.0", "^1.2.3"); // false
```
