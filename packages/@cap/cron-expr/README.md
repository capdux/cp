# @cap/cron-expr

Zero-heap 5-field Unix Cron expression bitmask compiler, standard macro expander, and O(1) schedule matcher in pure CP.

## Features

- `parseCron(expr)`: compiles `min hour dom month dow` into compact bitmasks (`u64`/`u32`/`u16`/`u8`), supporting `*`, `*/step`, `a-b`, `a-b/step`, comma lists, Sunday `7 -> 0` normalization, and `@hourly`/`@daily`/`@midnight`/`@weekly`/`@monthly`/`@yearly`/`@annually`
- `matches(sched, minute, hour, day, month, dow)`: O(1) bitmask timestamp check
- `isValidCron(expr)` & `countMatchingMinutesPerDay(sched)`

## Usage

```cp
import { parseCron, matches } from "@cap/cron-expr";

const s = parseCron("*/15 9-17 * * 1-5");
const hit = matches(s, 30, 10, 12, 5, 3); // true
```
