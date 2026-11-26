# @cap/rand-distr

Clean-room MIT, zero-heap probability distribution samplers for CP — modeled on crates.io `rand_distr` v0.4 and built on [`@cap/xoshiro256`](../xoshiro256/README.md).

## Features

- **Zero heap allocation & pure CP**: self-contained `ln`, `sqrt`, and `cos` math helpers with zero external C/Zig math dependencies.
- **Deterministic PRNG Integration**: directly reuses `@cap/xoshiro256`'s `Xoshiro256` engine and `nextF64`.
- **Distributions**:
  - `UniformRange`: continuous uniform `[low, high)`
  - `NormalDistr`: Gaussian `N(mean, std_dev^2)` via Box-Muller transform
  - `ExpDistr`: exponential `Exp(lambda)` via inverse transform `-ln(1 - u) / lambda`
  - `BernoulliDistr`: boolean trial `Bernoulli(p)`
  - `WeightedIndex`: zero-heap discrete weighted sampling over up to 16 weights

## Dependencies

- `@cap/xoshiro256` `1.0.0`

## API

- `UniformRange { low: f64, high: f64 }` (`new`, `sample`)
- `NormalDistr { mean: f64, std_dev: f64 }` (`new`, `sample`)
- `ExpDistr { lambda: f64 }` (`new`, `sample`)
- `BernoulliDistr { p: f64 }` (`new`, `sample`)
- `WeightedIndex { cumulative: [16]f64, count: usize, total: f64 }` (`fromWeights`, `sample`)

## License

MIT
