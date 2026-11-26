<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`qs`](https://github.com/ljharb/qs)（`BSD-3-Clause`）（原库名 `qs`，CP 包名 `@cap/qs-lite`）。

# @cap/qs-lite

Zero-heap `application/x-www-form-urlencoded` query string parser, lookup, and percent-encoding serializer for CP.
Compatible with npm `qs`, `query-string`, and Rust `serde_urlencoded`.

## API

- `QueryPair { key: string, value: string }`
- `encodeComponent(input: string, space_as_plus: bool, out: []u8): usize`
- `decodeComponent(input: string, plus_as_space: bool, out: []u8): usize`
- `parseRaw(query: string, out: []QueryPair): usize`
- `countPairs(query: string): usize`
- `getDecoded(query: string, target_key: string, out: []u8, out_len: *usize): bool`
- `stringify(pairs: []const QueryPair, space_as_plus: bool, out: []u8): usize`

## License

MIT
