# @cap/soundex

Zero-heap NARA American Soundex phonetic algorithm encoder and fuzzy name matcher for CP.
Compatible with npm `soundex-code` and Rust `rphonetic`.

## API

- `soundex(word: string, out: []u8): usize`
- `soundsLike(a: string, b: string): bool`
- `soundexDifference(a: string, b: string): u8`

## License

MIT
