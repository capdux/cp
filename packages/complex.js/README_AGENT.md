# complex.js@1.0.0 (Agent Quickref)

> Zero-allocation complex number arithmetic, polar form, exponential, integer powers, and principal square root for CP | License: MIT
> Import: `import { ... } from "complex.js";` | Full types: `src/index.d.cp`

## Functions
- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn reduceAngle(rad: f64): f64;`
- `export fn sinF64(rad: f64): f64;`
- `export fn cosF64(rad: f64): f64;`
- `export fn expF64(x: f64): f64;`
- `export fn atanSmall(x: f64): f64;`
- `export fn atanF64(x: f64): f64;`
- `export fn atan2F64(y: f64, x: f64): f64;`
- `export fn new(re: f64, im: f64): Complex;`
- `export fn zero(): Complex;`
- `export fn one(): Complex;`
- `export fn iUnit(): Complex;`
- `export fn fromPolar(r: f64, theta: f64): Complex;`
- `export fn add(a: Complex, b: Complex): Complex;`
- `export fn sub(a: Complex, b: Complex): Complex;`
- `export fn mul(a: Complex, b: Complex): Complex;`
- `export fn div(a: Complex, b: Complex): Complex;`
- `export fn conj(z: Complex): Complex;`
- `export fn neg(z: Complex): Complex;`
- `export fn scale(z: Complex, s: f64): Complex;`
- `export fn normSq(z: Complex): f64;`
- `export fn norm(z: Complex): f64;`
- `export fn arg(z: Complex): f64;`
- *(+4 more in `src/index.d.cp`)*
