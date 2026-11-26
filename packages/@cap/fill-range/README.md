# @cap/fill-range

Zero-heap numeric, alphabetic, stepped, and zero-padded range expansion engine in pure CP (npm `fill-range` parity).

## Features

- `countRange(start, end, step)`: O(1) element count for ascending or descending ranges
- `expandNumbers(start, end, step, out)`: zero-heap integer range expansion with automatic step direction
- `expandChars(start, end, step, out)`: ASCII letter expansion (`'a'..'z'`, `'Z'..'A'`)
- `formatRange(start, end, step, sep, dst)`: string range formatter supporting integers, negative numbers, zero-padding (`"001".."005"`, `"-02".."02"`), and letters

## Usage

```cp
import { formatRange, countRange } from "@cap/fill-range";

let dst: [64]u8 = undefined;
const n = formatRange("01", "05", 1, ",", dst[0..64]);
// dst[0..n] => "01,02,03,04,05"
```
