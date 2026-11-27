# matrixmultiply — AI Agent Quick Ref

Zero-heap general dense matrix multiplication (sgemm / dgemm / matmulF64) with arbitrary row and column strides (`v0.3.10`, `MIT`)

## Import

```cp
import { mmulMin, sgemm, dgemm, matmulF64 } from "matrixmultiply";
```

## API Signatures

```cp
fn mmulMin(a: usize, b: usize): usize;
fn sgemm(m: usize, k: usize, n: usize, alpha: f32, a: &[]f32, rsa: usize, csa: usize, b: &[]f32, rsb: usize, csb: usize, beta: f32, c: []f32, rsc: usize, csc: usize);
fn dgemm(m: usize, k: usize, n: usize, alpha: f64, a: &[]f64, rsa: usize, csa: usize, b: &[]f64, rsb: usize, csb: usize, beta: f64, c: []f64, rsc: usize, csc: usize);
fn matmulF64(m: usize, k: usize, n: usize, a: &[]f64, b: &[]f64, c: []f64);
```

## Minimal Usage

```cp
import { mmulMin } from "matrixmultiply";

pub fn main() {
    let result = mmulMin(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"matrixmultiply"`; install via `cap add matrixmultiply`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add matrixmultiply
cpc check src/main.cp
```
