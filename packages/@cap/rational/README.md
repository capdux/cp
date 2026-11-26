# `@cap/rational`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fraction.js` ([https://github.com/rawify/Fraction.js](https://github.com/rawify/Fraction.js)) / `crates.io:fraction` ([https://github.com/dnsl48/fraction](https://github.com/dnsl48/fraction))（原库名 `fraction.js` → CP 包名 `@cap/rational`）

Zero-heap exact rational number (fraction) arithmetic in pure CP. Always maintains canonical reduced form (`denom > 0`, `gcd(|numer|, denom) == 1`).

## Features

- **100% Zero Heap Allocation**: Pure value-type `Rational { numer: i64, denom: i64 }`.
- **Canonical Reduction**: Automatic GCD reduction and sign normalization to the numerator.
- **Complete Arithmetic**: `add`, `sub`, `mul`, `div`, `neg`, `abs`, `recip`, `eq`, `cmp`, `toF64`, `floorInt`, `ceilInt`, `isInteger`.
- **Safe Boundary Handling**: Zero denominator and division by zero safely return `0/1` without panicking.

## Quick Start

```cp
import { new, add, eq, toF64 } from "@cap/rational";

const a = new(1, 3);
const b = new(1, 6);
const sum = add(a, b); // 1/2
const ok = eq(sum, new(1, 2)); // true
const f = toF64(sum); // 0.5
```

## License

MIT
