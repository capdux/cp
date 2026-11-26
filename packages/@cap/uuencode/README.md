# @cap/uuencode

Zero-heap POSIX/Unix `uuencode` and `uudecode` codec in pure CP.

## Features

- `encodeLine(data, out)` & `decodeLine(line, out)`: 1..45-byte line codec compatible with both legacy space `' '` and modern backtick `` ` `` zero encoding
- `encode(data, out)` & `decode(input, out)`: multi-line 45-byte chunked codec with `` `\n `` terminator and automatic `begin <mode> <name>` / `end` frame support

## Usage

```cp
import { encodeLine, decodeLine } from "@cap/uuencode";

let out: [16]u8 = undefined;
const n = encodeLine("Cat", out[0..16]); // "#0V%T"
```
