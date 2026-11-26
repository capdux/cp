# @cap/detect-newline

Zero-heap-allocation dominant newline detector and cross-platform LF/CRLF normalizer in pure CP (inspired by Sindre Sorhus `detect-newline`).

## Features

- Constants: `EOL_NONE (0)`, `EOL_LF (1)`, `EOL_CRLF (2)`, `EOL_CR (3)`
- `analyzeNewlines(input: string): NewlineStats` (single-pass exact counts for `\n`, `\r\n`, `\r`)
- `detectNewline(input: string): string` (`"\r\n"`, `"\n"`, or `""`)
- `detectNewlineGraceful(input: string): string` (defaults to `"\n"` when no newline is present)
- `normalizeToLf(input, out)` & `normalizeToCrlf(input, out)` (idempotent, never emits `\r\r\n`)

## License

MIT
