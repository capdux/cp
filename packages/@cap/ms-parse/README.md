# @cap/ms-parse

Zero-allocation human-readable duration string parser and formatter (`ms`, `s`, `m`, `h`, `d`, `w`, `y`) for CP.

## Features

- **Zero heap allocation**: parses and formats directly over `string` / `[]u8` buffers.
- **Full unit support**: `parseMs` supports signed values, decimals (`"1.5h"`), bare numbers (default ms), and case-insensitive short/long units (`ms`/`millisecond(s)`, `s`/`second(s)`, `m`/`minute(s)`, `h`/`hour(s)`, `d`/`day(s)`, `w`/`week(s)`, `y`/`year(s)`).
- **Short & long formatters**: `formatShort` (`"1m"`, `"-5s"`) and `formatLong` (`"1 minute"`, `"2 minutes"`).

## Usage

```cp
import { parseMs, formatShort, formatLong } from "@cap/ms-parse";

const r = parseMs("1.5h"); // r.ms == 5400000
let buf: [32]u8 = undefined;
const n = formatLong(120000 as i64, buf[0..32]); // "2 minutes"
```
