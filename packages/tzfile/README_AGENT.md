# tzfile@1.0.0 (Agent Quickref)

> Zero-heap RFC 8536 TZif binary header parser, POSIX TZ string parser, and UTC offset transition evaluator (crates.io tzfile equivalent) | License: MIT
> Import: `import { ... } from "tzfile";` | Full types: `src/index.d.cp`

## Functions
- `export fn tzReadU32Be(buf: []u8, off: usize): u32;`
- `export fn isTzifMagic(buf: []u8): bool;`
- `export fn parseTzifHeader(buf: []u8): TzifHeader?;`
- `export fn tzIsAlpha(c: u8): bool;`
- `export fn tzIsDigit(c: u8): bool;`
- `export fn tzParseAbbr(s: string, pos_in: usize): TzAbbrRes;`
- `export fn tzParsePosixOffset(s: string, pos_in: usize): TzOffRes;`
- `export fn parsePosixTz(spec: string): PosixTz?;`
- `export fn lookupTransitionOffset(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32;`
- `export fn utcToLocalSeconds(unix_secs: i64, offset_secs: i32): i64;`
