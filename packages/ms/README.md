[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ms

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:ms`](https://github.com/vercel/ms) (MIT) | CP Package Version: `1.0.0` | Dependency: `itoa`

Zero-heap millisecond conversion utility for CP. Parses human-readable duration strings (e.g. `"2 days"`, `"10h"`, `"2.5 hrs"`, `"-3 days"`) into `i64` milliseconds and formats `i64` milliseconds into short (`"1m"`, `"2d"`) or long (`"1 minute"`, `"2 days"`) strings directly into caller-provided buffers.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"ms"`) — no renaming required.

| TypeScript / Node.js (`npm:ms`) | CP (`ms`) |
| :--- | :--- |
| `import ms from "ms";` | `import { parseMs, formatMs } from "ms";` |
| `ms("2 days")` → `172800000` | `parseMs("2 days").ms` → `172800000` |
| `ms(60000)` → `"1m"` | `formatMs(60000, false, out)` → `"1m"` |
| `ms(60000, { long: true })` → `"1 minute"` | `formatMs(60000, true, out)` → `"1 minute"` |

## API Reference

### Constants

- `MS_SECOND: i64 = 1000`
- `MS_MINUTE: i64 = 60000`
- `MS_HOUR: i64 = 3600000`
- `MS_DAY: i64 = 86400000`
- `MS_WEEK: i64 = 604800000`
- `MS_YEAR: i64 = 31557600000` (365.25 days)

### Types & Functions

| Signature | Description |
| :--- | :--- |
| `MsParseResult { ok: bool, ms: i64 }` | Result of `parseMs`; check `ok` before reading `ms`. |
| `parseMs(input: string): MsParseResult` | Parse a duration string (up to 100 chars) into milliseconds. |
| `formatMs(ms: i64, long_fmt: bool, out: []u8): usize` | Format `ms` into `out` (`long_fmt = false` for short, `true` for long); returns bytes written. |

## Usage Examples

### Basic Parsing & Formatting

```cp
import { parseMs, formatMs } from "ms";

fn demoBasic() {
    const r1 = parseMs("2 days");
    // r1.ok == true, r1.ms == 172800000

    const r2 = parseMs("2.5 hrs");
    // r2.ok == true, r2.ms == 9000000

    let buf: [32]u8 = undefined;
    const n_short = formatMs(60000, false, buf[0..]);
    // buf[0..n_short] == "1m"

    const n_long = formatMs(120000, true, buf[0..]);
    // buf[0..n_long] == "2 minutes"
}
```

### Edge Cases & Negative / Invalid Inputs

```cp
import { parseMs, formatMs, MS_DAY } from "ms";

fn demoEdgeCases() {
    // Negative durations & bare numeric strings
    const neg = parseMs("-3 days");
    // neg.ok == true, neg.ms == -3 * MS_DAY

    const bare = parseMs("100");
    // bare.ok == true, bare.ms == 100

    // Invalid unit or >100-byte input fails cleanly without panic
    const bad = parseMs("10 fortnights");
    // bad.ok == false, bad.ms == 0

    let buf: [32]u8 = undefined;
    const n = formatMs(-86400000, true, buf[0..]);
    // buf[0..n] == "-1 day"
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: Formatting uses a stack scratch buffer (`let tmp: [24]u8 = undefined`) and `itoa::formatI64`, writing directly into caller-owned `out: []u8` slices.
- **Zero-Init Avoidance**: Temporary formatting buffers use `undefined` so no redundant zero-fill occurs on hot paths.
- **ReDoS-Free Linear Scanner**: Hand-written single-pass ASCII parser (no regex) with a strict 100-character input guard matching `vercel/ms`.
- **Borrow-Safe Output**: `out: []u8` is bounds-checked via `cap:fmt::writeStr`, guaranteeing zero buffer overruns.

## License

MIT
