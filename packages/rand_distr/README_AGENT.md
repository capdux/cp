# rand_distr — AI Agent Quick Ref

Zero-heap probability distribution samplers (Uniform, Normal Box-Muller, Exponential, Bernoulli, WeightedIndex) built on xoshiro256 (clean-room port of Rust rand_distr) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    UniformRange,
    NormalDistr,
    ExpDistr,
    BernoulliDistr,
    WeightedIndex,
    rdstAbs,
    rdstSqrt,
    rdstLn,
} from "rand_distr";
```

## Core Types & Constants

```cp
class UniformRange { low: f64, high: f64 } // methods: new(...) +1
class NormalDistr { mean: f64, std_dev: f64 } // methods: new(...) +1
class ExpDistr { lambda: f64 } // methods: new(...) +1
class BernoulliDistr { p: f64 } // methods: new(...) +1
class WeightedIndex { cumulative: [16]f64, count: usize, total: f64 } // methods: fromWeights(...) +1
```

## API Signatures

```cp
fn rdstAbs(x: f64): f64;
fn rdstSqrt(x: f64): f64;
fn rdstLn(x: f64): f64;
fn rdstCos(angle: f64): f64;
fn rdstOpen01(rng: &mut Xoshiro256): f64;
```

## Minimal Usage

```cp
import { rdstAbs } from "rand_distr";

pub fn main() {
    let result = rdstAbs(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rand_distr"`; install via `cap add rand_distr`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
