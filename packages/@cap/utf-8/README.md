# @cap/utf-8

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf-8` ([https://github.com/SimonSapin/rust-utf8](https://github.com/SimonSapin/rust-utf8)) — `MIT OR Apache-2.0`

Zero-copy incremental streaming UTF-8 decoding, RFC 3629 validation, and Unicode Maximal Subpart lossy recovery in pure CP.

## Features

- `Utf8DecodeStatus` (`Ok`, `InvalidSequence`, `Incomplete`) & `Utf8ChunkResult` (`status`, `valid_prefix`, `consumed`, `error_len`)
- `decodeUtf8Chunk(input: []const u8): Utf8ChunkResult` — strict RFC 3629 validation (rejects overlong encodings, UTF-16 surrogates `U+D800..U+DFFF`, and codepoints `> U+10FFFF`), distinguishing invalid sequences from incomplete trailing prefixes
- `LossyUtf8Decoder` — zero-heap cross-chunk incremental decoder (`.push(*_, chunk, out)`, `.finish(*_, out)`) emitting `U+FFFD` strictly per the Unicode Maximal Subpart rule
- `isValidUtf8(input: []const u8): bool`

## License

MIT OR Apache-2.0
