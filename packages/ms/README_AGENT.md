# ms@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:ms` (`vercel/ms`, MIT) | **Deps**: `itoa`
- **Import**: `import { parseMs, formatMs, MsParseResult, MS_SECOND, MS_MINUTE, MS_HOUR, MS_DAY, MS_WEEK, MS_YEAR } from "ms";`
- **Full Types**: `packages/ms/src/index.d.cp`

## Constants
```cp
MS_SECOND: i64 = 1000
MS_MINUTE: i64 = 60000
MS_HOUR:   i64 = 3600000
MS_DAY:    i64 = 86400000
MS_WEEK:   i64 = 604800000
MS_YEAR:   i64 = 31557600000 // 365.25 days
```

## Core Types & Functions
```cp
class MsParseResult { ok: bool, ms: i64 }

fn parseMs(input: string): MsParseResult
fn formatMs(ms: i64, long_fmt: bool, out: []u8): usize
```

## Minimal Pattern
```cp
import { parseMs, formatMs } from "ms";

const res = parseMs("1.5h"); // res.ok == true, res.ms == 5400000
let out: [32]u8 = undefined;
const n = formatMs(res.ms, true, out[0..]); // out[0..n] == "2 hours" (rounded)
```

## Pitfalls & Notes
1. **Check `res.ok`**: `parseMs` returns `MsParseResult { ok: false, ms: 0 }` on invalid units, empty strings, or inputs `> 100` bytes.
2. **Buffer Sizing**: Pass at least `[32]u8` for `out` in `formatMs`; it returns the exact byte count written (`out[0..n]`).
3. **Rounding**: `formatMs` rounds half-away-from-zero to the largest matching unit (`d`, `h`, `m`, `s`, `ms`), matching `vercel/ms`.
4. **Supported Units** (case-insensitive): `ms`/`msec`/`msecs`/`millisecond(s)`, `s`/`sec`/`secs`/`second(s)`, `m`/`min`/`mins`/`minute(s)`, `h`/`hr`/`hrs`/`hour(s)`, `d`/`day(s)`, `w`/`week(s)`, `y`/`yr`/`yrs`/`year(s)`, or bare number (treated as `ms`).
5. **Perf**: Zero-heap (`[24]u8 = undefined` scratch + `itoa::formatI64`), linear single-pass ASCII scan (no regex).
6. **Internal Helpers**: `msUnitMultiplier`, `msRoundDiv`, `msFormatUnit` are internal helpers — prefer `parseMs` and `formatMs`.
