# `cap:ode` — Zero-Allocation ODE Initial-Value Problem Solvers (RK2, RK4, Dormand-Prince RK4(5), Velocity Verlet)

Pure CP, zero-heap-allocation numerical ODE solver library providing explicit 2nd-order Midpoint (RK2), classical 4th-order Runge-Kutta (RK4), adaptive-step Dormand-Prince RK4(5) (`ode45` / `RK45`), and energy-conserving symplectic Velocity Verlet integrators.

## Types

- `OdeRhsFn = fn(t: f64, y: []const f64, dydt: *[]f64)`
- `AccelFn = fn(t: f64, pos: []const f64, acc: *[]f64)`
- `Rk45Result`: `{ steps_accepted: u32, steps_rejected: u32, final_dt: f64, reached_end: bool }`

## Core Functions

| Symbol | Scratch Requirement | Description |
| :--- | :--- | :--- |
| `rk2MidpointStep(f, t, dt, y, scratch): bool` | `>= 3 * y.len` | Single explicit 2nd-order Midpoint step. |
| `rk4Step(f, t, dt, y, scratch): bool` | `>= 5 * y.len` | Single classical 4th-order Runge-Kutta step. |
| `rk4Integrate(f, t0, t1, steps, y, scratch): bool` | `>= 5 * y.len` | Uniform multi-step RK4 integration over `[t0, t1]`. |
| `rk45Step(f, t, dt, y, y_out, err_out, scratch): bool` | `>= 8 * y.len` | Single Dormand-Prince RK4(5) step with embedded error estimate. |
| `rk45Integrate(f, t0, t1, y, rtol, atol, max_steps, scratch): Rk45Result?` | `>= 10 * y.len` | Adaptive-step Dormand-Prince RK4(5) integrator. |
| `verletStep(acc_fn, t, dt, pos, vel, acc_curr, acc_next): bool` | caller slices | Symplectic Velocity Verlet step preserving Hamiltonian energy. |

## Example

```cp
import { rk45Integrate } from "cap:ode";

fn expRhs(t: f64, y: []const f64, dydt: *[]f64) {
    * = t;
    dydt.*[0] = y[0];
}

fn solveE(): f64 {
    let y_arr: [1]f64 = [1.0];
    let scr_arr: [10]f64 = [0.0; 10];
    let y: []f64 = y_arr[0..];
    let scr: []f64 = scr_arr[0..];
    * = rk45Integrate(expRhs, 0.0, 1.0, *y, 1.0e-10, 1.0e-12, 1000, *scr);
    return y_arr[0]; // ~2.718281828459
}
```
