# @cap/strftime

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strftime` ([https://github.com/samsonjs/strftime](https://github.com/samsonjs/strftime))

Zero-heap POSIX `strftime` and proleptic Gregorian calendar formatter in pure CP (replaces npm `strftime` / `dateformat` [20M/wk]).

Supports `%Y`, `%y`, `%m`, `%d`, `%e`, `%H`, `%I`, `%M`, `%S`, `%p`, `%P`, `%a`, `%A`, `%b`, `%B`, `%j`, `%w`, `%u`, `%F`, `%T`, `%R`, `%z`, `%Z`, `%n`, `%t`, `%%`.

## Install

```bash
cap add @cap/strftime
```

## Usage

```cp
import { formatUnix, formatIso8601Utc, fromUnixSeconds, strftime } from "@cap/strftime";

let buf: [128]u8 = undefined;
const iso = formatIso8601Utc(1700000000, buf[0..128]);
// "2023-11-14T22:13:20Z"

const cn = formatUnix("%Y-%m-%d %H:%M:%S %z", 1700000000, 28800, buf[0..128]);
// "2023-11-15 06:13:20 +0800"
```

## API

| Function | Description |
| --- | --- |
| `fromUnixSeconds(unix_sec, tz_offset_sec)` | Convert Unix epoch seconds to broken-down `DateTime` |
| `strftime(fmt, dt, out)` | Format `DateTime` using POSIX `strftime` specifiers |
| `formatUnix(fmt, unix_sec, tz_offset_sec, out)` | Format Unix timestamp directly |
| `formatIso8601Utc(unix_sec, out)` | Format UTC timestamp as `YYYY-MM-DDTHH:MM:SSZ` |
| `isLeapYear(year)` | Proleptic Gregorian leap year test |
