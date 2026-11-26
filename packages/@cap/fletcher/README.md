# @cap/fletcher

Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 position-sensitive checksums in pure CP.

## API
- `Fletcher16` (`new`, `update`, `updateBytes`, `digest`)
- `fletcher16(data: string): u16`, `fletcher16Bytes(data: []u8): u16`
- `fletcher32(data: string): u32`, `fletcher32Bytes(data: []u8): u32`
- `fletcher64(data: string): u64`
