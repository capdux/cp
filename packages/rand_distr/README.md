[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rand_distr

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_distr` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand))（原库名 `rand_distr` → CP 包名 `rand_distr`）

Clean-room MIT, zero-heap probability distribution samplers for CP — modeled on crates.io `rand_distr` v0.4 and built on [`rand_xoshiro`](../rand_xoshiro/README.md).

## Features

- **Zero heap allocation & pure CP**: self-contained `ln`, `sqrt`, and `cos` math helpers with zero external C/Zig math dependencies.
- **Deterministic PRNG Integration**: directly reuses `rand_xoshiro`'s `Xoshiro256` engine and `nextF64`.
- **Distributions**:
  - `UniformRange`: continuous uniform `[low, high)`
  - `NormalDistr`: Gaussian `N(mean, std_dev^2)` via Box-Muller transform
  - `ExpDistr`: exponential `Exp(lambda)` via inverse transform `-ln(1 - u) / lambda`
  - `BernoulliDistr`: boolean trial `Bernoulli(p)`
  - `WeightedIndex`: zero-heap discrete weighted sampling over up to 16 weights

## Dependencies

- `rand_xoshiro` `1.0.0`

## API

- `UniformRange { low: f64, high: f64 }` (`new`, `sample`)
- `NormalDistr { mean: f64, std_dev: f64 }` (`new`, `sample`)
- `ExpDistr { lambda: f64 }` (`new`, `sample`)
- `BernoulliDistr { p: f64 }` (`new`, `sample`)
- `WeightedIndex { cumulative: [16]f64, count: usize, total: f64 }` (`fromWeights`, `sample`)

## License

MIT
