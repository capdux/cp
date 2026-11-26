# @cap/schema-utils

Zero-heap configuration option schema validator in pure CP.

## API

- Error codes: `SCHU_OK`, `SCHU_ERR_REQUIRED`, `SCHU_ERR_MIN_LEN`, `SCHU_ERR_MAX_LEN`, `SCHU_ERR_INT_MIN`, `SCHU_ERR_INT_MAX`, `SCHU_ERR_ENUM`
- `FieldConstraint { name: string, required: bool, min_len: usize, max_len: usize, has_int_range: bool, min_int: i64, max_int: i64 }`
- `ValidationResult { valid: bool, error_code: u32, message: string }`
- `validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult`
- `validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult`
- `validateEnumField(value: string, allowed: []const string): ValidationResult`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`schema-utils@4.3.0`)
- **Upstream Repository**: https://github.com/webpack/schema-utils
- **License**: MIT (clean-room CP implementation)
