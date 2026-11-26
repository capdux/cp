# @cap/html-escape

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:html-escape` ([https://github.com/magiclen/html-escape](https://github.com/magiclen/html-escape)) / `npm:html-entities` ([https://github.com/mdevils/html-entities](https://github.com/mdevils/html-entities))

Zero-heap HTML entity escaper and unescaper (named entities + decimal/hex numeric character references with UTF-8 output) for CP.
Compatible with npm `escape-html`, `he`, and Rust `html-escape`.

## API

- `needsEscape(input: string): bool`
- `escapeHtml(input: string, out: []u8): usize`
- `unescapeHtml(input: string, out: []u8): usize`

## License

MIT
