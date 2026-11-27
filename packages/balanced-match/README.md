<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`balanced-match`](https://github.com/juliangruber/balanced-match)（`MIT`）（包名 `@cap/balanced-match`）。

# `@cap/balanced-match`

Zero-heap-allocation nested balanced delimiter matcher in pure CP (clean-room MIT implementation inspired by npm `balanced-match` v3).

## Features

- **`balanced(a, b, input): BalancedResult`** — Finds the first balanced delimiter pair `(a, b)` with nested counting, identical open/close support, multi-byte delimiters, and unclosed-opener backtracking.
- **`balancedRange(a, b, input): [usize, usize]?`** — Returns the `[start, end]` byte offsets of the first balanced match.
- **`balancedAll(a, b, input, out): usize`** — Iteratively collects sequential top-level balanced matches into a caller-provided slice with zero heap allocation.

## Usage

```cp
import { BalancedResult, balanced, balancedRange, balancedAll } from "@cap/balanced-match";
```

## License

MIT
