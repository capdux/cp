# utf8parse@1.0.0 — AI Agent Quick Ref

- **Import**: `import { validateUtf8, isValidUtf8, isAscii, toLossyUtf8, utf16Len, checkSeqAt, isCont } from "utf8parse";`
- **Provenance**: crates.io `utf8parse` (v0.2) + `utf8_iter` (v1) parity | RFC 3629 strict | Zero-heap | License: MIT
- **Perf (ReleaseFast)**: `validateUtf8` (64 KB ASCII) = **17,815.77 MB/s (17.82 GB/s)** (3.51 µs/op, 16-lane SIMD)
- **Types**: `src/index.d.cp`

## Core Types & Functions
```cp
export class Utf8Error { valid_up_to: usize, error_len: u8 }
export class Utf8CheckResult {
    valid: bool,
    valid_up_to: usize,
    error_len: u8,          // 0 if unexpected EOF truncation; 1..3 for invalid byte(s)
    codepoint_count: usize,
}
export class StepResult { valid: bool, step_len: usize, error_len: u8 }

export fn isAscii(data: string): bool;
export fn validateUtf8(data: string): Utf8CheckResult;
export fn isValidUtf8(data: string): bool;
export fn toLossyUtf8(data: string, out: []u8): string;
export fn utf16Len(data: string): usize;
export fn checkSeqAt(data: string, pos: usize): StepResult;
export fn isCont(b: u8): bool;
```

## Minimal Example
```cp
import { validateUtf8, toLossyUtf8, utf16Len } from "utf8parse";

const r = validateUtf8("hello 世界 🚀");
if (r.valid) {
    const u16 = utf16Len("hello 世界 🚀"); // 11
    _ = u16;
}
let buf: [64]u8 = undefined;
const clean = toLossyUtf8("a\xFFb", buf[0..]); // "a\u{FFFD}b"
```

## Gotchas
1. `toLossyUtf8(data, out)` replaces each invalid maximal subpart with the 3-byte `U+FFFD` (`0xEF 0xBF 0xBD`); worst-case output size is `3 * data.len` (when every byte is invalid).
2. On invalid input, `Utf8CheckResult.error_len == 0` means a valid multi-byte lead was truncated at EOF (useful for streaming decoders waiting for more bytes); `1..3` means a hard invalid sequence.
3. `utf16Len(data)` counts UTF-16 code units (BMP chars = 1, supplementary/emoji `>= U+10000` = 2), matching JS `String.prototype.length`.
4. `validateUtf8` uses a 16-lane SIMD ASCII fast path automatically — no need to pre-call `isAscii` before `validateUtf8`.
