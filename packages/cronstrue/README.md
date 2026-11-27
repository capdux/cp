[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# cronstrue

Zero-allocation Cron expression to human-readable English & Chinese natural language translator for CP — clean-room MIT parity with npm `cronstrue`.

Translates standard 5-field POSIX Cron expressions (`minute hour dom month dow`) and predefined macros (`@yearly`, `@annually`, `@monthly`, `@weekly`, `@daily`, `@midnight`, `@hourly`) directly into caller-provided stack buffers with both English (`Locale.En`) and Chinese (`Locale.Zh`) output and 12h/24h formatting.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `cronstrue` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/bradymholt/cRonstrue` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: cronstrue)
import cronstrue from "cronstrue";
const text = cronstrue.toString("0 9 * * 1-5");
```

```cp
// CP (cap: cronstrue) — zero-allocation EN/ZH description into stack buffer
import { describeCron } from "cronstrue";
let buf: [128]u8 = undefined;
const text = describeCron("0 9 * * 1-5", buf[0..128]);
```

## Installation

```bash
cap add cronstrue
```

## Usage

### Basic Usage: English & Chinese Descriptions

```cp
import {
    Locale,
    CronDescOptions,
    isValidCron,
    describeCron,
    describeCronWith,
} from "cronstrue";

fn demoDescribe() {
    let buf_en: [128]u8 = undefined;
    let buf_zh: [128]u8 = undefined;

    // Default English 24-hour description
    const en = describeCron("0 9 * * 1-5", buf_en[0..128]);
    // => "At 09:00, Monday through Friday"

    // Chinese 24-hour description
    const zh = describeCronWith("0 9 * * 1-5", CronDescOptions.chinese(), buf_zh[0..128]);
    // => "在 09:00，周一至周五"

    const ok = isValidCron("*/15 * * * *"); // true
    _ = en;
    _ = zh;
    _ = ok;
}
```

### Edge Cases: 12-Hour Format, Macros, Named Months/Days & Invalid Inputs

```cp
import { Locale, CronDescOptions, describeCronWith, parseCron } from "cronstrue";

fn demoAdvanced() {
    let buf: [256]u8 = undefined;

    // 1. 12-hour clock option
    let opts12 = CronDescOptions.english();
    opts12.use_24h = false;
    const t12 = describeCronWith("30 14 1 * *", opts12, buf[0..256]);
    // => "At 02:30 PM, on day 1 of the month"

    // 2. Named months & days (case-insensitive) and predefined macros
    const named = describeCronWith("0 8 1,15 JAN,JUN MON-FRI", CronDescOptions.english(), buf[0..256]);
    const macro = describeCronWith("@weekly", CronDescOptions.chinese(), buf[0..256]);
    // => "在 00:00，仅在周日"

    // 3. Invalid expression returns empty string "" (never panics)
    const bad = describeCronWith("99 99 * * *", CronDescOptions.english(), buf[0..256]);
    // bad.len == 0

    // 4. Structured AST inspection via parseCron
    const parsed = parseCron("@daily");
    // parsed.valid == true
    _ = t12;
    _ = named;
    _ = macro;
    _ = bad;
    _ = parsed;
}
```

## Translation Examples

| Expression | English (`Locale.En`, 24h) | Chinese (`Locale.Zh`, 24h) |
| --- | --- | --- |
| `* * * * *` | `Every minute` | `每分钟` |
| `*/15 * * * *` | `Every 15 minutes` | `每 15 分钟` |
| `0 9 * * 1-5` | `At 09:00, Monday through Friday` | `在 09:00，周一至周五` |
| `@daily` | `At 00:00, every day` | `每天 00:00` |
| `@hourly` | `Every hour` | `每小时` |

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `Locale` | `enum { En = 0, Zh = 1 }` | Output language selector. |
| `CronDescOptions` | `class { locale: Locale, use_24h: bool }` | Formatting options with `CronDescOptions.english()` and `CronDescOptions.chinese()`. |
| `isValidCron` | `fn(expr: string): bool` | Strictly validates a 5-field Cron expression or predefined macro. |
| `describeCron` | `fn(expr: string, out: []u8): string` | Translates `expr` into English (24h) inside `out`; returns `""` if invalid. |
| `describeCronWith` | `fn(expr: string, opts: CronDescOptions, out: []u8): string` | Translates `expr` using custom `CronDescOptions`; returns `""` if invalid. |
| `parseCron` | `fn(raw: string): ParsedCron` | Parses a Cron string into structured `CronField` items (`valid == false` on error). |
| `expandMacro` | `fn(trimmed: string): string` | Expands `@yearly`/`@daily`/`@hourly`/etc. into their 5-field equivalents. |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `isDigit`, `eqIgnoreCase`, `trim`) and `cap:fmt` (`writeStr`, `writeU64`) for fast parsing and formatting.
- **Zero Heap Allocation**: All parsed fields use fixed stack pools (`[16]FieldItem` per field) and write directly into caller `out: []u8`.
- **Zero-Clear Stack Arrays (`undefined`)**: Internal token span arrays use `undefined` initialization, avoiding redundant memset overhead.
- **Buffer-Safe Truncation**: Output writes are strictly bounds-checked against `out.len`, preventing buffer overruns.

## License

MIT — Clean-room CP implementation inspired by npm `cronstrue`.
