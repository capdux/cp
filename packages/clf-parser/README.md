[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `clf-parser`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clf-parser` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `clf-parser` → CP 包名 `clf-parser`）

Zero-heap-allocation Apache/Nginx Common Log Format (CLF) and Combined Log Format parser and formatter in pure CP.

## Features

- **Zero-Copy Parsing (`parseClf`)**: Extracts byte offsets for `host`, `user`, `time`, `method`, `path`, `proto`, `referer`, `ua`, plus numeric `status` and `bytes` (`-1` for `-`).
- **CLF & Combined Compatible**: Automatically detects optional `"referer" "user-agent"` suffix (`is_combined`).
- **Canonical Formatter (`formatClf`)**: Emits standard CLF lines into caller buffers with `-` placeholders for empty user or negative bytes.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { ClfRecord, parseClf, formatClf } from "clf-parser";
```

## License

MIT
