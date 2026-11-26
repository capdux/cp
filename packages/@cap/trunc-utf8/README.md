# @cap/trunc-utf8

Zero-heap UTF-8 codepoint-boundary safe string truncator with ellipsis support for CP.
Compatible with npm `cli-truncate` and Rust `unicode-truncate`.

## API

- `isContinuationByte(b: u8): bool`
- `floorCharBoundary(s: string, max_bytes: usize): usize`
- `truncateBytes(s: string, max_bytes: usize): string`
- `countCodepoints(s: string): usize`
- `truncateChars(s: string, max_chars: usize): string`
- `truncateWithEllipsis(s: string, max_bytes: usize, ellipsis: string, out: []u8): usize`

## License

MIT
