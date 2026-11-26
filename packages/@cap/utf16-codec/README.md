# @cap/utf16-codec

Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec with full surrogate pair support, lone-surrogate rejection, BOM detection, and UTF-16 code-unit counting in pure CP.

## Features

- `detectBom(data)`: detects `BOM_UTF8`, `BOM_UTF16_LE`, `BOM_UTF16_BE`, or `BOM_NONE`
- `utf8ToUtf16(utf8, out)` & `utf16ToUtf8(utf16, out)`: full BMP + astral plane (`U+10000..U+10FFFF`) surrogate pairs
- `utf8ToUtf16LeBytes(utf8, out)` & `utf16LeBytesToUtf8(bytes, out)`: Win32-ready little-endian byte streams
- `countUtf16Units(utf8)`: JS/Java `String.length` equivalent unit count without writing

## Usage

```cp
import { utf8ToUtf16, utf16ToUtf8, countUtf16Units } from "@cap/utf16-codec";

let u16_buf: [32]u16 = undefined;
const units = utf8ToUtf16("Hi中文🚀", u16_buf[0..32]); // 6 units
```
