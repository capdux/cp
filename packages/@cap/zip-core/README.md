# `@cap/zip-core`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zip` ([https://github.com/zip-rs/zip2](https://github.com/zip-rs/zip2))（原库名 `zip` → CP 包名 `@cap/zip-core`）

Zero-heap PKZIP Stored archive writer, local file header & EOCD parser, and CRC-32 payload verifier (`writeStoredZipSingle`, `parseLocalHeader`, `findAndParseEocd`, `verifyStoredEntry`, `ZipLocalHeader`, `ZipEocd`) — clean-room CP equivalent of crates.io `zip`, composing `@cap/crc32fast` and `@cap/bytemuck-crate`.
