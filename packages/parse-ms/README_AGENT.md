# parse-ms (AI Agent Quick Ref)

Zero-heap duration breakdown (`days`..`nanoseconds`), reconstruction, and formatting.

## Import
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

## API Signatures
- Consts (`u64`): `MS_PER_SECOND=1000`, `MS_PER_MINUTE=60000`, `MS_PER_HOUR=3600000`, `MS_PER_DAY=86400000`
- `class ParsedMs { days: i64, hours: i64, minutes: i64, seconds: i64, milliseconds: i64, microseconds: i64, nanoseconds: i64 }`
  - `zero(): ParsedMs`, `isZero(): bool`, `isNegative(): bool`, `equals(other: ParsedMs): bool`
- `parseMs(ms: i64): ParsedMs`
- `parseMsF64Checked(ms: f64): ParsedMs?` (`null` on NaN/Inf)
- `parseMsF64(ms: f64): ParsedMs` (NaN/Inf -> `ParsedMs.zero()`)
- `fromNanoseconds(ns_total: i64): ParsedMs`, `fromMicroseconds(us_total: i64): ParsedMs`, `fromSeconds(sec: i64): ParsedMs`
- `toMilliseconds(p: ParsedMs): i64`, `toMillisecondsF64(p: ParsedMs): f64`, `toNanoseconds(p: ParsedMs): i64`
- `formatParsedMs(p: ParsedMs, out: []u8): usize`

## Minimal Example
```typescript
import { parseMs, formatParsedMs } from "parse-ms";

fn fmtMs(ms: i64, out: []u8): usize {
    let p = parseMs(ms);
    return formatParsedMs(p, out);
}
```

## Pitfalls & Borrowing Rules
- For negative inputs (e.g. `parseMs(-1500)`), ALL non-zero fields are negative (`seconds = -1, milliseconds = -500`), matching npm `parse-ms` truncation-toward-zero semantics.
- Prefer `parseMsF64Checked(ms)` over `parseMsF64(ms)` when caller needs to distinguish NaN/Inf (`null`) from `0.0`.
- `toMilliseconds(p)` drops `microseconds` and `nanoseconds`; use `toMillisecondsF64(p)` or `toNanoseconds(p)` to retain sub-ms precision.
- `ParsedMs` is passed by value (zero heap, no borrow lifetimes); only `formatParsedMs` requires a mutable `out: []u8` buffer (64 bytes is sufficient for typical durations).
- `MS_PER_*` constants are `u64`, whereas `ParsedMs` fields are `i64`.
