# syslog@1.0.0 (Agent Quickref)

> Zero-heap RFC 5424 Syslog message parser and formatter with PRI facility/severity codec, structured data skipping, and NILVALUE handling. | License: MIT
> Import: `import { ... } from "syslog";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodePri(facility: u8, severity: u8): u8;`
- `export fn decodeFacility(pri: u8): u8;`
- `export fn decodeSeverity(pri: u8): u8;`
- `export fn readSpToken(raw: &[]u8, pos: usize): TokenSpan;`
- `export fn parseRfc5424(raw: &[]u8): SyslogMessage;`
- `export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;`
- `export fn appendFieldOrNil(dst: []u8, pos: usize, field: &[]u8): i64;`
- `export fn appendU8Dec(dst: []u8, pos: usize, val: u8): i64;`
- `export fn formatRfc5424(facility: u8, severity: u8, timestamp: &[]u8, host: &[]u8, app: &[]u8, proc_id: &[]u8, msg_id: &[]u8, msg: &[]u8, dst: []u8): i64;`
