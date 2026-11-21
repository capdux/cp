# `cap:logfmt` — Structured `logfmt` Log Parser & Encoder

Zero-allocation `logfmt` (`key=value`) structured logging parser, key lookup, and canonical quoting/escaping encoder.

## Exports

- `LogfmtField` — `key`, `raw_value`, `is_quoted`, `has_value`, `decodeValue(out): ([]const u8)?`.
- `LogfmtIter` — `init(line)`, `next(*_): LogfmtField?`, `hasError(): bool` (handles quoted escapes, bare boolean keys, unclosed quote detection).
- `getLogfmtValue(line, key, out): ([]const u8)?`
- `appendLogfmtPair(out, pos: *usize, key, val): bool` — Automatically quotes/escapes values with spaces, `=`, `"`, `\`, `\n`, `\r`, `\t`.
