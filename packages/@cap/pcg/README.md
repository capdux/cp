# @cap/pcg

Zero-heap PCG32 (`PCG-XSH-RR`) pseudorandom number generator with independent streams and O(log N) jump-ahead (`advance`) in pure CP (inspired by Rust `rand_pcg`, 295M+ downloads).

## Install

```bash
cap add @cap/pcg
```

## Usage

```cp
import { Pcg32 } from "@cap/pcg";

let rng = Pcg32.init(42, 1);
const a = rng.nextU32();
const b = rng.nextBoundedU32(100); // unbiased [0, 100)
const f = rng.nextF64();           // [0.0, 1.0)
rng.advance(1000000);              // O(log N) jump ahead 1M steps!
```
