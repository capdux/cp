# @cap/trunc-utf8

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-truncate` ([https://github.com/sindresorhus/cli-truncate](https://github.com/sindresorhus/cli-truncate)) / `crates.io:unicode-truncate` ([https://github.com/Aetf/unicode-truncate](https://github.com/Aetf/unicode-truncate))（原库名 `cli-truncate` → CP 包名 `@cap/trunc-utf8`）

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
