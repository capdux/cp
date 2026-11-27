# `@cap/oorandom`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oorandom` ([https://sr.ht/~icefox/oorandom/](https://sr.ht/~icefox/oorandom/))（原库名 `oorandom` → CP 包名 `@cap/oorandom`）

Zero-heap deterministic PCG32 (XSH RR 64/32) and dual-stream 64-bit PRNG in pure CP, modeled after Rust's `oorandom` crate. Uses wrapping arithmetic (`*%`, `+%`) for overflow-free PCG32 state transitions, unbiased Lemire/threshold range sampling, and IEEE-754 `[0.0, 1.0)` float generation.

零堆分配纯 CP PCG32（XSH RR 64/32）与双流 64 位伪随机数生成器，完整对标 `crates.io:oorandom`：使用环绕算术 `*%`/`+%` 严格复现标准 PCG32 状态演进，支持无偏区间采样与 `[0.0, 1.0)` 浮点生成。

## Installation

```bash
cap add @cap/oorandom
```

## API

| Type / Method | Description |
| --- | --- |
| `Rand32.new(seed)` / `Rand32.newInc(seed, inc)` | Initialize PCG32 generator with default or custom stream |
| `Rand32.randU32(*_)` / `randI32(*_)` | Next uniform `u32` or bit-cast `i32` |
| `Rand32.randFloat(*_)` | Uniform `f32` in `[0.0, 1.0)` |
| `Rand32.randRange(*_, start, end)` | Unbiased uniform `u32` in `[start, end)` |
| `Rand32.state()` | Inspect internal 64-bit PCG state |
| `Rand64.new(seed)` | Initialize dual-PCG32 64-bit generator |
| `Rand64.randU64(*_)` / `randFloat(*_)` | Next uniform `u64` or `f64` in `[0.0, 1.0)` |

## License

MIT
