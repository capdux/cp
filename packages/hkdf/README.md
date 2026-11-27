# `@cap/hkdf-crate`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hkdf` ([https://github.com/RustCrypto/KDFs](https://github.com/RustCrypto/KDFs))（原库名 `hkdf` → CP 包名 `@cap/hkdf-crate`）

Zero-heap RFC 5869 HKDF-SHA256 key derivation (`HkdfSha256.extract`, `fromPrk`, `expand`, `hkdfSha256`) — clean-room CP equivalent of crates.io `hkdf`.

Reuses `@cap/hmac-crate` as an independent package.
