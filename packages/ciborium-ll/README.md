# @cap/ciborium-ll

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ciborium-ll` ([https://github.com/enarx/ciborium](https://github.com/enarx/ciborium)) — `Apache-2.0`

Zero-heap RFC 8949 low-level streaming CBOR encoding and decoding primitives in pure CP.

## Features

- `CborHeaderKind` (`Positive`, `Negative`, `Bytes`, `Text`, `Array`, `Map`, `Tag`, `Bool`, `Null`, `Undefined`, `Float`, `Break`, `Invalid`)
- `CborHeader` (`kind`, `u64_val`, `i64_val`, `f64_val`, `bool_val`, `len` with `-1` denoting indefinite length)
- `CborDecoder` — streaming cursor with `.pull(*_): CborHeader`, `.readBytes(*_, n): ([]const u8)?`, `.remaining(): usize`, supporting definite & indefinite lengths and IEEE 754 binary16 / binary32 / binary64 floats
- `CborEncoder` — zero-heap buffer writer with `.pushPositive`, `.pushNegative`, `.pushBytes`, `.pushText`, `.pushArray`, `.pushMap`, `.pushTag`, `.pushBool`, `.pushNull`, `.pushUndefined`, `.pushFloat64`, `.pushBreak`, `.written()`

## License

Apache-2.0
