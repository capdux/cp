# @cap/range-parser

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:range-parser` ([https://github.com/jshttp/range-parser](https://github.com/jshttp/range-parser))

Clean-room MIT, zero-heap RFC 7233 HTTP `Range` header parser and interval combiner for CP — modeled on npm's `range-parser` v1.2.

## API
- `parseRange(size, header, combine): RangeResult` (`status`: `RANGE_OK` / `RANGE_UNSATISFIABLE` / `RANGE_MALFORMED`, `unit`, `ranges`, `count`)
- `rangeByteLength(r: ByteRange): u64`
