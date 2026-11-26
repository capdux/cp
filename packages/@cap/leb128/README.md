# @cap/leb128

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:leb128` ([https://github.com/gimli-rs/leb128](https://github.com/gimli-rs/leb128)) / `npm:leb` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap ULEB128 (unsigned) and SLEB128 (signed) variable-length integer encoder and decoder for CP.
Compatible with WebAssembly, DWARF, and Rust `leb128`.

## API

- `UlebResult { value: u64, consumed: usize, valid: bool }`
- `SlebResult { value: i64, consumed: usize, valid: bool }`
- `encodeUnsigned(val: u64, out: []u8): usize`
- `decodeUnsigned(data: []const u8, start: usize): UlebResult`
- `encodeSigned(val: i64, out: []u8): usize`
- `decodeSigned(data: []const u8, start: usize): SlebResult`
- `unsignedEncodedLen(val: u64): usize`

## License

MIT
