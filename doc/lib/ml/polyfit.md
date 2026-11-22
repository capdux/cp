# `cap:polyfit` — Zero-Allocation Polynomial Least-Squares Fitting, Ridge Regression & Root Finding

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

Pure CP, zero-heap-allocation numerical polynomial library providing streaming Givens QR least-squares polynomial fitting (with coordinate shift/scale conditioning for `deg <= 15`), L2 Ridge regularization, Horner evaluation & calculus, goodness-of-fit metrics ($R^2$, RMSE, SSE), and real root solvers (Bisection & Newton-Raphson).

All polynomials use **lowest-degree-first** ordering: `coeffs[0] + coeffs[1]*x + ... + coeffs[deg]*x^deg`.

## Constants & Types

- `POLYFIT_MAX_DEG = 15`
- `LinearFit`: `{ slope, intercept, r_squared, rmse: f64 }`
- `FitMetrics`: `{ sse, mse, rmse, r_squared, max_abs_err: f64 }`

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `polyEval(coeffs, x): f64` / `polyDerivEval(coeffs, x): f64` | $O(\text{deg})$ Horner polynomial and derivative evaluation. |
| `polyDerivCoeffs(coeffs, out): usize?` / `polyIntegrateCoeffs(coeffs, c0, out): usize?` | Analytic polynomial differentiation and integration. |
| `linearFit(xs, ys): LinearFit?` | Centered single-pass OLS simple linear regression. |
| `polyfit(xs, ys, deg, out_coeffs): bool` | Coordinate-normalized streaming Givens QR least-squares fit (`0 <= deg <= 15`). |
| `polyfitRidge(xs, ys, deg, lambda, out_coeffs): bool` | L2-regularized Ridge polynomial regression (`lambda >= 0`). |
| `evalFitMetrics(xs, ys, coeffs): FitMetrics?` | Computes SSE, MSE, RMSE, $R^2$, and max absolute error. |
| `polyRootBisect(coeffs, a, b, tol, max_iter): f64?` | Bracketed bisection real root solver. |
| `polyRootNewton(coeffs, x0, tol, max_iter): f64?` | Newton-Raphson tangent real root solver. |

## Example

```cp
import { polyfit, polyEval } from "cap:ml";

fn fitQuadratic(xs: []const f64, ys: []const f64, x_query: f64): f64 {
    let c_arr: [3]f64 = [0.0; 3];
    let c: []f64 = c_arr[0..];
    if (polyfit(xs, ys, 2, *c)) {
        return polyEval(c_arr[0..], x_query);
    }
    return 0.0;
}
```
