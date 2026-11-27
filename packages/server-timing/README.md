[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `server-timing`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:server-timing` ([https://github.com/yosuke-furukawa/server-timing](https://github.com/yosuke-furukawa/server-timing))

Zero-heap-allocation W3C `Server-Timing` HTTP response header parser and formatter in pure CP.

## Features

- **`parseHeader`**: Zero-copy parsing of bare metrics (`miss`), durations (`dur=53.2`), and quoted/unquoted descriptions (`desc="PostgreSQL; read, primary"`).
- **`findMetric` & `totalDurationMs`**: Streaming lookup and total latency aggregation without intermediate buffers.
- **`formatMetric` & `formatHeader`**: Formats metrics with automatic quoting/escaping for descriptions containing spaces or special characters.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    TimingMetric,
    parseHeader,
    findMetric,
    formatMetric,
    formatHeader,
    totalDurationMs,
} from "server-timing";
```

## License

MIT
