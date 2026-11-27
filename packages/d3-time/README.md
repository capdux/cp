# `@cap/d3-time`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-time` ([https://github.com/d3/d3-time](https://github.com/d3/d3-time))

Zero-heap-allocation UTC calendar time interval arithmetic on Unix milliseconds (`i64`) in pure CP.

## Features

- **Full Interval Primitives**: `timeSecondFloor/Ceil/Offset/Count`, `timeMinuteFloor/Offset`, `timeHourFloor/Offset`, `timeDayFloor/Ceil/Offset/Count`, `timeWeekFloor/Offset`, `timeMonthFloor/Offset`, `timeYearFloor/Offset`, plus `utcFromYmd`.
- **Howard Hinnant Civil Calendar**: Exact proleptic Gregorian leap-year arithmetic with unsigned-safe division across negative (pre-1970) and far-future timestamps.
- **Zero Heap Allocation**: Pure stack-only `i64`/`u64` integer math.

## API

```cp
import {
    utcFromYmd,
    timeSecondFloor, timeSecondCeil, timeSecondOffset, timeSecondCount,
    timeMinuteFloor, timeMinuteOffset,
    timeHourFloor, timeHourOffset,
    timeDayFloor, timeDayCeil, timeDayOffset, timeDayCount,
    timeWeekFloor, timeWeekOffset,
    timeMonthFloor, timeMonthOffset,
    timeYearFloor, timeYearOffset,
} from "@cap/d3-time";
```

## License

ISC
