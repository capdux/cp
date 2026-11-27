# @cap/nibble-vec

Zero-heap 4-bit nibble vector in pure CP for radix trees and prefix compression (up to 128 nibbles / 64 bytes inline).

## API

- `NIBBLE_MAX_BYTES: usize = 64`
- `NibbleVec` struct:
  - `NibbleVec.new(): NibbleVec`
  - `NibbleVec.fromBytes(bytes: []const u8): NibbleVec`
  - `len(): usize`, `isEmpty(): bool`
  - `push(*_, nib: u8): bool`, `pop(*_): u8?`
  - `get(idx: usize): u8?`, `set(*_, idx: usize, nib: u8): bool`
  - `commonPrefixLen(other: NibbleVec): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`nibble_vec@0.1.0`)
- **Upstream Repository**: https://github.com/michaelsproul/rust_nibble_vec
- **License**: MIT (clean-room CP implementation)
