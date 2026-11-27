# `@cap/macaddr`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:macaddr` ([https://github.com/svartalf/rust-macaddr](https://github.com/svartalf/rust-macaddr))

Zero-heap pure-CP IEEE 802 MAC-48 / EUI-48 (`MacAddr6`) and EUI-64 (`MacAddr8`) hardware address parser, formatter, and RFC 4291 EUI-64 converter — clean-room CP equivalent of crates.io `macaddr`.

## Features

- `MacAddr6`: `parse` (colon, hyphen, Cisco dot, mixed case), `isNil`, `isBroadcast`, `isMulticast`, `isUnicast`, `isLocal`, `isUniversal`, `oui()`, `formatColonInto`, `formatHyphenInto`, `formatDotInto`, `equals`
- `MacAddr8`: `parse` (colon, hyphen, Cisco dot), `fromMac6Eui64` (inserts `FF:FE` & flips U/L bit), `isNil`, `isBroadcast`, `isMulticast`, `isLocal`, `formatColonInto`, `formatHyphenInto`, `formatDotInto`, `equals`
- Strict validation rejecting wrong lengths, invalid hex digits, and mixed separators
