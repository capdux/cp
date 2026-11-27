[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/raw-body`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:raw-body` ([https://github.com/stream-utils/raw-body](https://github.com/stream-utils/raw-body))

Zero-allocation HTTP request body byte-limit enforcer, `Content-Length` verifier, streaming chunk accumulator, and strict RFC 3629 UTF-8 validator for CP. Clean-room MIT implementation inspired by npm `raw-body` v3.

## Features

- **Zero heap allocation**: Accumulates streaming HTTP body chunks directly into a caller-provided `out: []u8` buffer.
- **Fast-fail size protection**: Rejects oversized declared `Content-Length` (`checkDeclaredLength() -> 413`) and overflowing chunks (`pushChunk() -> 413`) before copying.
- **Content-Length & UTF-8 verification**: Validates exact received length (`400`) and strict RFC 3629 UTF-8 (`400`, rejecting overlong encodings, UTF-16 surrogates, and truncated sequences) on `finish()`.
- **Human-readable byte limits**: `parseByteLimit` parses `"100kb"`, `"1mb"`, `"512b"`, `"1.5kb"`, `"1024"`, etc.

## API

### Error Codes

- `RAW_OK: u16 = 0`
- `RAW_ERR_TOO_LARGE: u16 = 413`
- `RAW_ERR_LENGTH_MISMATCH: u16 = 400`
- `RAW_ERR_INVALID_UTF8: u16 = 400`
- `RAW_ERR_ABORTED: u16 = 499`

### Structs & Methods

- `RawBodyOptions { limit: usize, expected_len: usize, has_expected_len: bool, validate_utf8: bool }`
  - `RawBodyOptions.defaultOpts()` (100 KiB limit)
  - `RawBodyOptions.withLimit(limit: usize)`
- `RawBodyResult { status: u16, bytes_written: usize }`
  - `isOk(): bool`
- `RawBodyReader`
  - `RawBodyReader.new(opts: RawBodyOptions)`
  - `checkDeclaredLength(): u16`
  - `pushChunk(*_, chunk: []const u8, out: []u8): u16`
  - `abort(*_): u16`
  - `finish(*_, out: []const u8): RawBodyResult`
  - `receivedBytes(): usize`

### Functions

- `parseByteLimit(spec: string): usize?`
- `isValidUtf8(data: []const u8): bool`

## Example

```cp
import {
    RAW_OK,
    RawBodyOptions,
    RawBodyReader,
    parseByteLimit
} from "@cap/raw-body";

let lim = parseByteLimit("100kb");
let opts = RawBodyOptions {
    limit: lim ?? 102400,
    expected_len: 11,
    has_expected_len: true,
    validate_utf8: true
};
let reader = RawBodyReader.new(opts);
let buf: [256]u8 = undefined;

reader.pushChunk("hello ", buf[0..256]);
reader.pushChunk("world", buf[0..256]);
let res = reader.finish(buf[0..256]);
// res.status == RAW_OK, res.bytes_written == 11, buf[0..11] == "hello world"
```

## License

MIT
