# @cap/indent-strip

Zero-heap common leading indentation detector, dedent stripper, and line indenter for CP.
Compatible with npm `strip-indent`, `dedent`, and Rust `indoc`.

## API

- `minIndent(input: string): usize`
- `stripIndent(input: string, out: []u8): usize`
- `indentLines(input: string, spaces: usize, out: []u8): usize`

## License

MIT
