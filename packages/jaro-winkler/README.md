[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/jaro-winkler`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jaro-winkler` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:jaro-winkler` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap-allocation Jaro and Jaro-Winkler string similarity metrics and best-match finder in pure CP.

## Features

- **`jaro`**: Classic Jaro similarity in `[0.0, 1.0]` using stack-allocated `[256]bool` match masks.
- **`jaroWinkler` & `jaroWinklerWithScale`**: Winkler prefix boosting (`l <= 4`, threshold `0.7`, default `p = 0.1`, custom `p` clamped to `[0.0, 0.25]`).
- **`bestMatchIndex`**: Threshold-filtered top-1 fuzzy entity matching.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { jaro, jaroWinkler, jaroWinklerWithScale, bestMatchIndex } from "@cap/jaro-winkler";
```

## License

MIT
