# stringprep@1.0.0 (Agent Quickref)

> Zero-heap RFC 3454 / RFC 4013 SASLprep & Nameprep string preparation for SCRAM-SHA-256, PostgreSQL, and LDAP in pure CP | License: MIT
> Import: `import { ... } from "stringprep";` | Full types: `src/index.d.cp`

## Functions
- `export fn decodeUtf8(s: string, i: usize): DecCp;`
- `export fn encodeUtf8(cp: u32, out: []u8, pos: usize): i64;`
- `export fn isMappedToNothing(cp: u32): bool;`
- `export fn isNonAsciiSpace(cp: u32): bool;`
- `export fn isSaslProhibited(cp: u32): bool;`
- `export fn saslprep(input: string, out: []u8): PrepResult;`
- `export fn nameprepAscii(input: string, out: []u8): PrepResult;`
