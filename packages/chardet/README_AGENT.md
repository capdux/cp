# chardet — AI Agent Quick Ref

Zero-allocation character encoding detector (BOM, UTF-8, UTF-16/32, ASCII, Shift-JIS, GBK, Windows-1252, ISO-8859-1) (`v2.1.0`, `MIT`)

## Import

```cp
import {
    DetectedCharset,
    CharsetMatch,
    charsetName,
    chdScoreShiftJis,
    chdScoreGbk,
    detect,
    detectName,
} from "chardet";
```

## Core Types & Constants

```cp
enum DetectedCharset { Utf8, Utf8Bom, Utf16Le, Utf16Be, Utf32Le, Utf32Be, ... }
class CharsetMatch { charset: DetectedCharset, name: string, confidence: u8 }
```

## API Signatures

```cp
fn charsetName(cs: DetectedCharset): string;
fn chdScoreShiftJis(data: &[]u8): u8;
fn chdScoreGbk(data: &[]u8): u8;
fn detect(data: &[]u8): CharsetMatch;
fn detectName(data: &[]u8): string;
```

## Minimal Usage

```cp
import { charsetName } from "chardet";

pub fn main() {
    let result = charsetName(/* cs: DetectedCharset */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"chardet"`; install via `cap add chardet`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
