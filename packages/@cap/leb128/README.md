# @cap/leb128

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
