# `@cap/zip-core`

Zero-heap PKZIP Stored archive writer, local file header & EOCD parser, and CRC-32 payload verifier (`writeStoredZipSingle`, `parseLocalHeader`, `findAndParseEocd`, `verifyStoredEntry`, `ZipLocalHeader`, `ZipEocd`) — clean-room CP equivalent of crates.io `zip`, composing `@cap/crc32fast` and `@cap/bytemuck-crate`.
