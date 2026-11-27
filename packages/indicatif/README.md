[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/indicatif`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indicatif` ([https://github.com/console-rs/indicatif](https://github.com/console-rs/indicatif))

Zero-heap terminal progress bar, throughput, and ETA renderer in pure CP (clean-room counterpart to Rust `indicatif`). Directly composes `@cap/pretty-bytes` and `@cap/pretty-ms`.

## Features

- **Real Cross-Package Composition**: Reuses `@cap/pretty-bytes` (`prettyBytes`) and `@cap/pretty-ms` (`prettyMs`) with zero duplicated formatting logic.
- **100% Zero Heap Allocation**: Renders bars and full progress lines directly into caller-provided `out: []u8`.
- **Complete Progress Metrics**: `percent`, `etaMs`, `bytesPerSec`, `renderBar`, and `renderLine`.

## Quick Start

```cp
import { ProgressState, renderLine } from "@cap/indicatif";

const st = ProgressState { pos: 512, total: 1024, elapsed_ms: 2000, width: 10 };
let buf: [128]u8 = undefined;
const n = renderLine(st, "Downloading", &buf);
// "Downloading [====>     ] 50% 512 B/1.02 kB ETA 2s"
```

## License

MIT
