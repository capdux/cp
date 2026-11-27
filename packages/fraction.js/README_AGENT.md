# fraction.js@1.0.0 (Agent Quickref)

> CP package fraction.js | License: MIT
> Import: `import { ... } from "fraction.js";` | Full types: `src/index.d.cp`

## Functions
- `export fn absToU64(v: i64): u64;`
- `export fn divTrunc64(a: i64, b: i64): i64;`
- `export fn gcd64(a: i64, b: i64): i64;`
- `export fn zero(): Rational;`
- `export fn one(): Rational;`
- `export fn fromInt(n: i64): Rational;`
- `export fn new(n: i64, d: i64): Rational;`
- `export fn neg(a: Rational): Rational;`
- `export fn abs(a: Rational): Rational;`
- `export fn recip(a: Rational): Rational;`
- `export fn add(a: Rational, b: Rational): Rational;`
- `export fn sub(a: Rational, b: Rational): Rational;`
- `export fn mul(a: Rational, b: Rational): Rational;`
- `export fn div(a: Rational, b: Rational): Rational;`
- `export fn eq(a: Rational, b: Rational): bool;`
- `export fn cmp(a: Rational, b: Rational): i32;`
- `export fn toF64(a: Rational): f64;`
- `export fn floorInt(a: Rational): i64;`
- `export fn ceilInt(a: Rational): i64;`
- `export fn isInteger(a: Rational): bool;`
