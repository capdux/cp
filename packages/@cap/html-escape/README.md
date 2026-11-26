# @cap/html-escape

Zero-heap HTML entity escaper and unescaper (named entities + decimal/hex numeric character references with UTF-8 output) for CP.
Compatible with npm `escape-html`, `he`, and Rust `html-escape`.

## API

- `needsEscape(input: string): bool`
- `escapeHtml(input: string, out: []u8): usize`
- `unescapeHtml(input: string, out: []u8): usize`

## License

MIT
