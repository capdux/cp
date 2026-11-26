# `@cap/tzfile`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tzfile` ([https://github.com/x-hgg-x/tz-rs](https://github.com/x-hgg-x/tz-rs))

Zero-heap RFC 8536 TZif binary header parser, POSIX TZ string parser (`EST5EDT`, `AEST-10AEDT`, `<+11>-11`), and UTC transition offset evaluator — clean-room CP equivalent of crates.io `tzfile`.

## Features
- `isTzifMagic(buf)` & `parseTzifHeader(buf)` — validates TZif v1/v2/v3/v4 headers
- `parsePosixTz(spec)` — parses standard/DST abbreviations, quoted `<+11>` labels, and POSIX-inverted UTC offsets
- `lookupTransitionOffset(transitions, unix_secs, default_offset)` & `utcToLocalSeconds(unix_secs, offset_secs)`
