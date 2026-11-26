# @cap/unicode-width

Zero-allocation Unicode 15.1 East Asian Width and terminal display column width engine for CP (Rust `unicode-width` & npm `string-width` parity).

## Features

- **Zero heap allocation**: direct UTF-8 decoding with Unicode 15.1 East Asian Width and Combining Marks tables.
- **Standard & CJK widths**: `charWidth`, `charWidthCjk`, `strWidth`, `strWidthCjk`.
- **Safe column truncation**: `truncateToWidth(s, max_cols)` returns a UTF-8 boundary-aligned byte length fitting within `max_cols`.

## Usage

```cp
import { strWidth, truncateToWidth } from "@cap/unicode-width";

const w = strWidth("你好CP!"); // 2 + 2 + 1 + 1 + 1 = 7
const cut = truncateToWidth("你好CP!", 5); // "你好C" (7 bytes, 5 cols)
```
