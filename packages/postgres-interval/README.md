[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `postgres-interval`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postgres-interval` ([https://github.com/bendrucker/postgres-interval](https://github.com/bendrucker/postgres-interval))

Zero-heap-allocation PostgreSQL `INTERVAL` parser and ISO-8601 / PostgreSQL bidirectional formatter in pure CP.

## Features

- **PostgreSQL Text Formats**: Parses standard (`1 year 2 mons 3 days 04:05:06.789`), negative (`-2 days -01:30:00`), verbose (`@ 1 year 2 days ago`), and time-only (`00:00:42`) intervals.
- **ISO-8601 Duration**: Parses `P1Y2M3DT4H5M6S` (including fractional seconds and weeks) and formats via `toIsoDuration`.
- **Zero Heap Allocation**: Pure CP, stack-only formatting directly into caller-provided `[]u8` buffers.

## API

```cp
import {
    PgInterval,
    parsePgInterval,
    toIsoDuration,
    toPostgresString,
    totalSecondsApprox,
} from "postgres-interval";
```

## License

MIT
