# @cap/ansi-strip

Zero-heap ECMA-48 / ANSI terminal escape sequence stripper (CSI, OSC, 2-byte ESC) and visible character counter in pure CP.

## Features

- `hasAnsi(input)`: O(N) fast-path check for `ESC` (`0x1B`) or C1 CSI (`0x9B`)
- `stripAnsi(input, out)`: strips CSI (`ESC [ ...`), OSC (`ESC ] ... BEL/ST`), DCS/APC, and 2-byte ESC sequences into caller buffer
- `strippedLen(input)`: zero-write exact stripped byte length
- `visibleAsciiCount(input)`: counts visible ASCII / UTF-8 characters after stripping ANSI escapes

## Usage

```cp
import { stripAnsi, hasAnsi } from "@cap/ansi-strip";

let buf: [64]u8 = undefined;
const n = stripAnsi("\x1b[38;2;255;0;0mRed\x1b[0m", buf[0..64]);
// buf[0..n] => "Red"
```
