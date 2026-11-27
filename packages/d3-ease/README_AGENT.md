# d3-ease — AI Agent Quick Ref

Zero-allocation D3 easing functions (linear, quad, cubic, poly, sin, exp, circle, bounce, back, elastic) in pure CP (`v1.0.0`, `ISC`)

## Import

```cp
import {
    deasTpmt,
    deasClampAmp,
    easeLinear,
    easeQuadIn,
    easeQuadOut,
    easeQuadInOut,
    easeCubicIn,
    easeCubicOut,
} from "d3-ease";
```

## API Signatures

```cp
fn deasTpmt(x: f64): f64;
fn deasClampAmp(a: f64): f64;
fn easeLinear(t: f64): f64;
fn easeQuadIn(t: f64): f64;
fn easeQuadOut(t: f64): f64;
fn easeQuadInOut(t: f64): f64;
fn easeCubicIn(t: f64): f64;
fn easeCubicOut(t: f64): f64;
fn easeCubicInOut(t: f64): f64;
fn easePolyInExp(t: f64, e: f64): f64;
fn easePolyOutExp(t: f64, e: f64): f64;
fn easePolyInOutExp(t: f64, e: f64): f64;
fn easePolyIn(t: f64): f64;
fn easePolyOut(t: f64): f64;
fn easePolyInOut(t: f64): f64;
fn easeSinIn(t: f64): f64;
fn easeSinOut(t: f64): f64;
fn easeSinInOut(t: f64): f64;
fn easeExpIn(t: f64): f64;
fn easeExpOut(t: f64): f64;
fn easeExpInOut(t: f64): f64;
fn easeCircleIn(t: f64): f64;
fn easeCircleOut(t: f64): f64;
fn easeCircleInOut(t: f64): f64;
fn easeBounceOut(t: f64): f64;
fn easeBounceIn(t: f64): f64;
fn easeBounceInOut(t: f64): f64;
fn easeBackInOvershoot(t: f64, s: f64): f64;
fn easeBackOutOvershoot(t: f64, s: f64): f64;
fn easeBackInOutOvershoot(t: f64, s: f64): f64;
fn easeBackIn(t: f64): f64;
fn easeBackOut(t: f64): f64;
fn easeBackInOut(t: f64): f64;
fn easeElasticInParams(t: f64, a: f64, p: f64): f64;
fn easeElasticOutParams(t: f64, a: f64, p: f64): f64;
fn easeElasticInOutParams(t: f64, a: f64, p: f64): f64;
fn easeElasticIn(t: f64): f64;
fn easeElasticOut(t: f64): f64;
fn easeElasticInOut(t: f64): f64;
```

## Minimal Usage

```cp
import { deasTpmt } from "d3-ease";

pub fn main() {
    let result = deasTpmt(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-ease"`; install via `cap add d3-ease`.
