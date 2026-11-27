# tdigest — AI Agent Quick Ref

Zero-allocation Merging T-Digest streaming quantile (P50/P95/P99/P99.9) and CDF estimator with distributed merge for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Centroid, TDigest, emptyCentroid } from "tdigest";
```

## Core Types & Constants

```cp
class Centroid { mean: f64, weight: f64 }
class TDigest { centroids: [128]Centroid, centroid_len: usize, compression: f64, total_weight: f64, /* +3 fields */ } // methods: init(...) +14
```

## API Signatures

```cp
fn emptyCentroid(): Centroid;
```

## Minimal Usage

```cp
import { emptyCentroid } from "tdigest";

pub fn main() {
    let result = emptyCentroid();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tdigest"`; install via `cap add tdigest`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add tdigest
cpc check src/main.cp
```
