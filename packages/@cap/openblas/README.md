# @cap/openblas

OpenBLAS (v0.3.29) high-performance BLAS matrix computation library for CP (`sgemm`, `dgemm`, `sdot`, `ddot`, `openblasGetConfig`), distributed with a prebuilt dynamic library (`libopenblas.so`) for zero local build overhead.

- **Upstream**: [OpenMathLib/OpenBLAS](https://github.com/OpenMathLib/OpenBLAS) (`v0.3.29`)
- **License**: BSD-3-Clause

## Usage

```cp
import { dgemm, sgemm, sdot, ddot, openblasGetConfig } from "@cap/openblas";

pub fn main(): void! {
    const a: [4]f64 = .{ 1.0, 2.0, 3.0, 4.0 };
    const b: [4]f64 = .{ 5.0, 6.0, 7.0, 8.0 };
    let c: [4]f64 = .{ 0.0, 0.0, 0.0, 0.0 };
    dgemm(2, 2, 2, 1.0, &a, &b, 0.0, &c)?;
}
```
