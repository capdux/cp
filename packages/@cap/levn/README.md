# @cap/levn

Zero-heap Light Value Notation (levn) scalar, list, and object parser in pure CP.

## API

- `LevnKind` enum (`Null`, `Bool`, `Int`, `Float`, `String`, `Invalid`)
- `LevnValue { kind: LevnKind, bool_val: bool, int_val: i64, float_val: f64, str_val: string }`
- `LevnPair { key: string, value: LevnValue }`
- `parseLevnScalar(input: string): LevnValue`
- `parseLevnList(input: string, out: []LevnValue): usize`
- `parseLevnObject(input: string, out: []LevnPair): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`levn@0.4.1`)
- **Upstream Repository**: https://github.com/gkz/levn
- **License**: MIT (clean-room CP implementation)
