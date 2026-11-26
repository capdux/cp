# @cap/complex-num

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:complex.js` ([https://github.com/rawify/Complex.js](https://github.com/rawify/Complex.js))（原库名 `complex.js` → CP 包名 `@cap/complex-num`）

Zero-heap-allocation complex number (`Complex`) algebra, polar conversion, exponential, integer powers, and principal square root library in pure CP with zero external dependencies.

## Features

- **Constructors**: `new(re, im)`, `zero()`, `one()`, `iUnit()`, `fromPolar(r, theta)`
- **Arithmetic**: `add`, `sub`, `mul`, `div` (zero-division safe), `conj`, `neg`, `scale`
- **Polar & Transcendental**: `normSq`, `norm`, `arg`, `exp`, `powInt(z, n: i32)`, `sqrt`, `approxEq`
- Built-in Newton-Raphson `sqrtF64` and Taylor-series `sinF64` / `cosF64` / `expF64` / `atan2F64`.

## License

MIT
