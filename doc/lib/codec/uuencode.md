# `cap:uuencode` — POSIX `uuencode`/`uudecode` & `xxencode` Zero-Allocation Codec

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

Pure CP, zero-heap-allocation binary-to-text codec supporting POSIX `uuencode`/`uudecode` (both historical space `0x20` and modern backtick `` ` `` zero-encoding), raw body lines, and `xxencode` (`+-0-9A-Za-z`).

## Constants & Types

- `UU_MAX_LINE_BYTES = 45`, `UU_DEFAULT_MODE = 0o644`
- `XX_ALPHABET = "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"`
- `UuHeader`: `{ mode: u16, filename: []const u8, body_offset: usize }`
- `UuDecoded`: `{ mode: u16, filename: []const u8, len: usize }`

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `parseUuHeader(data: []const u8): UuHeader?` | Parses `begin <octal_mode> <filename>\n` (LF or CRLF). |
| `uuEncode(src, mode, filename, dst: *[]u8): usize?` | Encodes full POSIX `uuencode` text with `begin`, 45B lines, `` ` ``, and `end`. |
| `uuDecode(src, dst: *[]u8): UuDecoded?` | Decodes full `uuencode` text, accepting both space `0x20` and backtick `0x60`. |
| `uuEncodeRaw(src, dst: *[]u8): usize?` / `uuDecodeRaw(src, dst: *[]u8): usize?` | Body-only encoding/decoding without `begin`/`end` framing. |
| `xxEncode(src, mode, filename, dst: *[]u8): usize?` / `xxDecode(src, dst: *[]u8): UuDecoded?` | Full `xxencode` codec using `XX_ALPHABET`. |

## Example

```cp
import { uuEncode, uuDecode, UU_DEFAULT_MODE } from "cap:codec";

fn roundTrip(data: []const u8, enc: *[]u8, out: *[]u8) {
    if (n := uuEncode(data, UU_DEFAULT_MODE, "cat.txt", enc)) {
        * = uuDecode(enc.*[0..n], out);
    }
}
```
