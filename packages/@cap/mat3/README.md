# @cap/mat3

Zero-allocation 3x3 row-major matrix and 2D affine transform library for CP with built-in Taylor-series `sin`/`cos` (zero external dependencies).

## Features

- **Zero heap allocation**: fixed `[9]f64` row-major `Mat3` value type.
- **2D affine constructors**: `identity`, `zero`, `fromTranslation`, `fromScale`, `fromRotation`.
- **Linear algebra**: `mul`, `transpose`, `determinant`, `isInvertible`, `inverse` (singular-safe), `transformPoint` (`w=1`), `transformDir` (`w=0`), and `approxEq`.

## Usage

```cp
import { fromTranslation, fromRotation, mul, transformPoint, Vec2D } from "@cap/mat3";

const t = fromTranslation(10.0 as f64, 5.0 as f64);
const r = fromRotation(1.57079632679489661923 as f64);
const m = mul(t, r);
const p = transformPoint(m, Vec2D{ x: 1.0 as f64, y: 0.0 as f64 }); // (10, 6)
```
