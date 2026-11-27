[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parse-ms

Zero-heap millisecond, microsecond, nanosecond, and second duration breakdown (`days`/`hours`/`minutes`/`seconds`/`milliseconds`/`microseconds`/`nanoseconds`), reconstruction, and formatting in pure CP.

- **Package**: `parse-ms` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `npm/parse-ms` (`MIT`, `sindresorhus/parse-ms`), extended with `f64` sub-millisecond parsing, nanosecond/microsecond constructors, round-trip reconstruction, and zero-heap formatting.

## Installation & Import

```bash
cap add parse-ms
```

```typescript
import {
    MS_PER_SECOND, MS_PER_MINUTE, MS_PER_HOUR, MS_PER_DAY,
    ParsedMs,
    parseMs, parseMsF64, parseMsF64Checked,
    fromNanoseconds, fromMicroseconds, fromSeconds,
    toMilliseconds, toMillisecondsF64, toNanoseconds,
    formatParsedMs,
} from "parse-ms";
```

## Core API

### Constants & `ParsedMs` Class

| Symbol | Value / Signature | Description |
| --- | --- | --- |
| `MS_PER_SECOND` / `MS_PER_MINUTE` | `1000` / `60000` (`u64`) | Milliseconds per second / minute. |
| `MS_PER_HOUR` / `MS_PER_DAY` | `3600000` / `86400000` (`u64`) | Milliseconds per hour / day. |
| `ParsedMs` | Fields: `days`, `hours`, `minutes`, `seconds`, `milliseconds`, `microseconds`, `nanoseconds` (all `i64`); Methods: `zero(): ParsedMs`, `isZero(): bool`, `isNegative(): bool`, `equals(other: ParsedMs): bool` | Value struct holding all 7 signed duration components. |

### Parsing, Construction, Reconstruction & Formatting

| Function | Signature | Description |
| --- | --- | --- |
| `parseMs` | `(ms: i64): ParsedMs` | Decomposes integer milliseconds (positive or negative) into `ParsedMs` (`microseconds = 0, nanoseconds = 0`). |
| `parseMsF64` | `(ms: f64): ParsedMs` | Decomposes floating-point milliseconds including fractional `microseconds` and `nanoseconds` (NaN/Inf -> zero). |
| `parseMsF64Checked` | `(ms: f64): ParsedMs?` | Same as `parseMsF64`, returning `null` for NaN or infinite inputs. |
| `fromNanoseconds` | `(ns_total: i64): ParsedMs` | Decomposes total nanoseconds into all 7 fields. |
| `fromMicroseconds` | `(us_total: i64): ParsedMs` | Decomposes total microseconds into `days..microseconds`. |
| `fromSeconds` | `(sec: i64): ParsedMs` | Decomposes total seconds into `days..seconds`. |
| `toMilliseconds` | `(p: ParsedMs): i64` | Reconstructs integer milliseconds (truncating sub-ms fields). |
| `toMillisecondsF64` | `(p: ParsedMs): f64` | Reconstructs floating-point milliseconds including `microseconds` and `nanoseconds`. |
| `toNanoseconds` | `(p: ParsedMs): i64` | Reconstructs total nanoseconds from all 7 fields. |
| `formatParsedMs` | `(p: ParsedMs, out: []u8): usize` | Formats `ParsedMs` as human-readable text (e.g. `"1d 13h 56m 40s 999ms"`) into `out`. |

## Examples

### 1. Parse Milliseconds & Format

```typescript
import { parseMs, formatParsedMs } from "parse-ms";

fn main() {
    let p = parseMs(1337000001);
    // p.days == 15, p.hours == 11, p.minutes == 23, p.seconds == 20, p.milliseconds == 1
    let buf: [64]u8 = undefined;
    let n = formatParsedMs(p, buf[0..]);
}
```

### 2. Fractional `f64` Parsing with NaN/Inf Guard

```typescript
import { parseMsF64Checked, toMillisecondsF64 } from "parse-ms";

fn main() {
    if (let p = parseMsF64Checked(12.345678)) {
        // p.milliseconds == 12, p.microseconds == 345, p.nanoseconds == 678
        let ms_back = toMillisecondsF64(p);
    }
}
```

### 3. Negative Durations & Nanosecond Round-Trip

```typescript
import { fromNanoseconds, toNanoseconds } from "parse-ms";

fn main() {
    let p = fromNanoseconds(-1500000500);
    let neg = p.isNegative();      // true
    let total = toNanoseconds(p);  // -1500000500
}
```

## Differences from Upstream & Highlights

- **Signed Integer Precision**: `parseMs(i64)` avoids JS 53-bit `Number` precision loss for large durations, while `parseMsF64Checked` mirrors JS fractional ms support safely via `ParsedMs?`.
- **Two-Way Conversion**: Adds `fromNanoseconds`, `fromMicroseconds`, `fromSeconds`, `toMilliseconds`, `toMillisecondsF64`, `toNanoseconds`, and `formatParsedMs`.
- **Zero Heap Allocation**: `ParsedMs` is a pure 56-byte stack value struct with zero heap allocations.
