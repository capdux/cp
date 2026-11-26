# `@cap/powerfmt`

Zero-heap high-performance integer formatting in pure CP (clean-room counterpart to Rust `powerfmt`).

## Features

- **100% Zero Heap Allocation**: Formats directly into caller-provided `out: []u8` with safe truncation.
- **Full Formatting Controls**: `min_width`, `pad_byte`, `ALIGN_RIGHT` / `ALIGN_LEFT` / `ALIGN_CENTER`, radix `2` / `8` / `10` / `16` (lower & upper), `force_sign`, and digit grouping (`group_sep`, `group_size`).
- **Sign-Aware Padding & `i64::MIN` Safety**: Zero-padding places sign before zeros (`"-0042"`); space-padding places sign adjacent to digits (`"  -42"`). Handles `-9223372036854775808` without overflow.
- **Convenience Helpers**: `formatDecPadded`, `formatHex`, `formatWithCommas`.

## Quick Start

```cp
import { formatDecPadded, formatHex, formatWithCommas } from "@cap/powerfmt";

let buf: [32]u8 = undefined;
const n1 = formatDecPadded(-42, 5, &buf);     // "-0042"
const n2 = formatHex(3735928559, true, &buf); // "DEADBEEF"
const n3 = formatWithCommas(1234567, &buf);   // "1,234,567"
```

## License

MIT
