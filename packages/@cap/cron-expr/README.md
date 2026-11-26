# @cap/cron-expr

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cron` ([https://github.com/zslayton/cron](https://github.com/zslayton/cron))（原库名 `cron` → CP 包名 `@cap/cron-expr`）

Zero-allocation 6-field second-granularity Cron schedule parser & matcher for CP — clean-room MIT parity with crates.io `cron` v0.12.

Composes [`@cap/cron-parser`](../cron-parser) (`CronSchedule`, `parseCron`, `matches`) for the 5 standard calendar fields (minute/hour/day-of-month/month/day-of-week) and presets with zero code duplication, adding the 1st second-granularity bitmask field (`0..59`).

## Installation

```json
{
  "dependencies": {
    "@cap/cron-expr": "1.0.0"
  }
}
```

## Quick Start

```cp
import {
    parseCron6,
    matches6,
    nextSecInMinute,
    isValidCron6,
} from "@cap/cron-expr";

const sched = parseCron6("*/15 30 9 * * 1-5");
const hit = matches6(sched, 6, 15, 3, 9, 30, 45); // true
const next = nextSecInMinute(sched, 16);          // 30
```

## API Reference

### `CronExpr6`
- `valid: bool` — whether the expression parsed cleanly
- `sec_mask: u64` — 60-bit mask for seconds `0..59`
- `base: CronSchedule` — underlying 5-field schedule from `@cap/cron-parser`

### Functions
- `parseCron6(expr: string): CronExpr6` — parses 6-field expressions (`sec min hour dom month dow`), 5-field expressions (automatically defaulting seconds to `0`), and preset aliases (`@daily`, `@hourly`, `@weekly`, `@monthly`, `@yearly`, `@annually`, `@midnight`)
- `matches6(sched: CronExpr6, month: u8, dom: u8, dow: u8, hour: u8, minute: u8, sec: u8): bool`
- `nextSecInMinute(sched: CronExpr6, from_sec: u8): u8?` — finds the next matching second `>= from_sec` within the current minute
- `isValidCron6(expr: string): bool`

## License

MIT
