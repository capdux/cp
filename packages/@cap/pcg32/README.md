# @cap/pcg32

Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG in pure CP (Rust `rand_pcg` / npm `pcg-random` parity).

## Features

- `Pcg32.new(seed, stream)` & `Pcg32.fromSeed(seed)` with official PCG-XSH-RR seeding
- `nextU32()`, `nextU64()`, `nextF64()` (`[0.0, 1.0)`), unbiased `nextBounded(bound)`
- `advance(delta)`: O(log n) fast jump-ahead
- `fillBytes(out)`: zero-heap random byte buffer filling

## Usage

```cp
import { Pcg32 } from "@cap/pcg32";

let rng = Pcg32.fromSeed(42);
const r = rng.nextU32();
const dice = rng.nextBounded(6);
```
