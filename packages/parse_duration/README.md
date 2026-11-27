[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/parse-duration

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parse_duration` ([https://github.com/zeta12ti/parse_duration](https://github.com/zeta12ti/parse_duration))（原库名 `parse_duration` → CP 包名 `@cap/parse-duration`）

Clean-room MIT, zero-heap human-readable duration parser and formatter for CP — modeled on crates.io `parse_duration` / `duration-str`.

## Features

- **Zero heap allocation**: parses directly from `string` slices and formats into caller-provided `[]u8` buffers.
- **Comprehensive unit support (case-insensitive)**:
  - `ns`, `nsec`, `nanosecond(s)`
  - `us`, `usec`, `µs`, `μs`, `microsecond(s)`
  - `ms`, `msec`, `millisecond(s)`
  - `s`, `sec`, `second(s)`
  - `m`, `min`, `minute(s)`
  - `h`, `hr`, `hour(s)`
  - `d`, `day(s)`
  - `w`, `wk`, `week(s)`
  - `y`, `yr`, `year(s)` (365 days)
- **Compound expressions & fractional values**: handles `"2h 30m 15s"`, `"1d12h"`, `"5m, 10s"`, `"1h + 30m"`, `"1.5h"`, `"0.25s"`, `"2.5ms"` using exact integer arithmetic (no floating-point rounding drift).
- **Strict validation**: rejects empty strings, unitless numbers, unknown units, negative numbers, trailing separators, and `u64` overflows.

## API

### `ParsedDuration`
- Fields: `secs: u64`, `nanos: u32`
- Methods:
  - `totalNanos(): u64?`
  - `totalMillis(): u64`
  - `totalSecs(): u64`
  - `isZero(): bool`

### Functions
- `parseDuration(input: string): ParsedDuration?`
- `formatDuration(d: ParsedDuration, out: []u8): usize`

## Usage

```typescript
import { parseDuration, formatDuration } from "@cap/parse-duration";

let d = parseDuration("2h 30m 15s"); // secs = 9015, nanos = 0
let buf: [64]u8 = ...;
let n = formatDuration(d ?? ParsedDuration { secs: 0, nanos: 0 }, buf[0..64]);
```

## License

MIT
