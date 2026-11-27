# d3-scale — AI Agent Quick Ref

Zero-allocation D3 quantitative scales (LinearScale, PowScale, SqrtScale, LogScale, QuantizeScale) in pure CP (`v1.0.0`, `ISC`)

## Import

```cp
import {
    LinearScale,
    PowScale,
    SqrtScale,
    LogScale,
    QuantizeScale,
    d3scClamp01,
    d3scLerp,
    d3scSignedPow,
} from "d3-scale";
```

## Core Types & Constants

```cp
class LinearScale { d0: f64, d1: f64, r0: f64, r1: f64, /* +1 fields */ } // methods: clamp(...) +4
class PowScale { d0: f64, d1: f64, r0: f64, r1: f64, /* +2 fields */ } // methods: clamp(...) +2
class SqrtScale { d0: f64, d1: f64, r0: f64, r1: f64, /* +1 fields */ } // methods: clamp(...) +2
class LogScale { d0: f64, d1: f64, r0: f64, r1: f64, /* +2 fields */ } // methods: clamp(...) +2
class QuantizeScale { d0: f64, d1: f64, buckets: usize } // methods: applyIndex(...) +3
```

## API Signatures

```cp
fn d3scClamp01(t: f64): f64;
fn d3scLerp(a: f64, b: f64, t: f64): f64;
fn d3scSignedPow(x: f64, exp: f64): f64;
fn d3scTickIncrement(start: f64, stop: f64, count: usize): f64;
fn linearScale(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale;
fn powScale(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale;
fn sqrtScale(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale;
fn logScale(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale;
fn quantizeScale(d0: f64, d1: f64, buckets: usize): QuantizeScale;
```

## Minimal Usage

```cp
import { d3scClamp01 } from "d3-scale";

pub fn main() {
    let result = d3scClamp01(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-scale"`; install via `cap add d3-scale`.
