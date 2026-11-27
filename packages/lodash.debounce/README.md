[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `lodash.debounce`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lodash.debounce` ([https://github.com/lodash/lodash](https://github.com/lodash/lodash))（原库名 `lodash.debounce` → CP 包名 `lodash.debounce`）

Zero-heap-allocation, deterministic clock-driven `Debouncer` and `Throttler` state machines in pure CP.

## Features

- **`Debouncer`**: Full `leading`, `trailing`, `maxWait`, `poll`, `flush`, `cancel`, `isPending`, `execCount`, and `nextDueMs`.
- **`Throttler`**: Fixed-interval rate limiting with `leading`, `trailing`, `poll`, `cancel`, `isPending`, and `execCount`.
- **Deterministic & Zero Dependencies**: Driven entirely by caller-supplied `now_ms: u64`; no OS clock calls, no heap allocations, no FFI.

## API

```cp
import { Debouncer, Throttler } from "lodash.debounce";
```

## License

MIT
