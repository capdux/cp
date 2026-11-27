[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cron-parser

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cron-parser` ([https://github.com/harrisiirak/cron-parser](https://github.com/harrisiirak/cron-parser))

Clean-room MIT, zero-heap 5-field standard Cron expression parser, bitmask representation, and time matching engine for CP — modeled on npm `cron-parser` v4.

## API

### Structs
- `CronSchedule { valid: bool, min_mask: u64, hour_mask: u32, dom_mask: u32, month_mask: u16, dow_mask: u8 }`
  - Compact bitmask encoding: `min_mask` (bits 0..59), `hour_mask` (bits 0..23), `dom_mask` (bits 1..31), `month_mask` (bits 1..12), `dow_mask` (bits 0..6, with Sunday `7` normalized to `0`).

### Functions
- `parseCron(expr: string): CronSchedule`
  - Supports `*`, `*/step`, `n`, `a-b`, `a-b/step`, comma-separated lists `a,b,c`, Sunday `7 -> 0` normalization, and preset aliases (`@yearly`, `@annually`, `@monthly`, `@weekly`, `@daily`, `@midnight`, `@hourly`).
  - Strictly rejects out-of-range values, step `0`, malformed ranges, and expressions whose field count is not 5.
- `matches(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool`
  - O(1) bitmask match against a calendar timestamp.
- `nextMinuteOffset(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?`
  - Searches forward up to 24 hours (1440 minutes) for the next trigger minute offset (`1..=1440`), returning `null` if none occurs within 24 hours.
- `isValidCron(expr: string): bool`
  - Convenience validity check for a Cron expression.
