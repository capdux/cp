[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/magnet-uri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:magnet-url` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `magnet-url` → CP 包名 `@cap/magnet-uri`）

Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53 `btih` v1 & `btmh` v2) parser, tracker extractor, and builder for CP.
Compatible with npm `magnet-uri` and Rust `magnet-url`.

## API

- Hash kinds: `HASH_BTIH_HEX`, `HASH_BTIH_BASE32`, `HASH_BTMH_V2`, `HASH_UNKNOWN`
- `MagnetLink { hash_kind, info_hash, display_name, exact_length, trackers, tracker_count, valid }`
- `parseMagnet(uri: string): MagnetLink`
- `formatMagnetV1(info_hash: string, display_name: string, trackers: []const string, out: []u8): usize`

## License

MIT
