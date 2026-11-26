# @cap/libm

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:libm` ([https://github.com/rust-lang/libm](https://github.com/rust-lang/libm))

Pure CP, zero-allocation IEEE 754 `f64` math library with zero external dependencies (parity with Rust `libm` / C `<math.h>`).

## Features

- **Constants**: `PI`, `TAU`, `FRAC_PI_2`, `FRAC_PI_4`, `E`, `LN_2`, `LN_10`
- **Basic & Rounding**: `fabs`, `floor`, `ceil`, `trunc`, `round`, `fmod`, `copysign`, `fmin`, `fmax`
- **Roots, Exponentials & Logarithms**: `sqrt`, `cbrt`, `hypot`, `exp`, `ln`, `log2`, `log10`, `pow`
- **Trigonometric & Inverse Trigonometric**: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`
- **Hyperbolic**: `sinh`, `cosh`, `tanh`

## Usage

```cp
import { PI, sin, cos, sqrt, hypot, exp, ln, pow } from "@cap/libm";

let h = hypot(3.0, 4.0);      // 5.0
let s = sin(PI / 6.0);        // 0.5
let p = pow(2.0, 10.0);       // 1024.0
let r = ln(exp(2.5));         // 2.5
```

## License

MIT
