# ajv-formats@1.0.0 (Agent Quickref)

> Zero-allocation JSON Schema Draft 2019-09/2020-12 string format validators: date, time, date-time, email, hostname, ipv4, ipv6, uuid, uri, json-pointer (clean-room MIT implementation of npm ajv-formats, #205 / 212M wk) | License: MIT
> Import: `import { ... } from "ajv-formats";` | Full types: `src/index.d.cp`

## Functions
- `export fn ajvfIsDigit(b: u8): bool;`
- `export fn ajvfIsAlpha(b: u8): bool;`
- `export fn ajvfIsAlphaNum(b: u8): bool;`
- `export fn ajvfIsHex(b: u8): bool;`
- `export fn ajvfParse2Digits(s: string, offset: usize): i32;`
- `export fn ajvfParse4Digits(s: string, offset: usize): i32;`
- `export fn ajvfIsLeapYear(year: u32): bool;`
- `export fn ajvfDaysInMonth(year: u32, month: u32): u32;`
- `export fn isDate(s: string): bool;`
- `export fn isTime(s: string): bool;`
- `export fn isDateTime(s: string): bool;`
- `export fn isHostname(s: string): bool;`
- `export fn ajvfIsEmailLocalChar(b: u8): bool;`
- `export fn isEmail(s: string): bool;`
- `export fn isIpv4(s: string): bool;`
- `export fn isIpv6(s: string): bool;`
- `export fn isUuid(s: string): bool;`
- `export fn ajvfIsSchemeChar(b: u8): bool;`
- `export fn isUri(s: string): bool;`
- `export fn isJsonPointer(s: string): bool;`
- `export fn ajvfStrEq(a: string, b: string): bool;`
- `export fn validateFormat(format_name: string, value: string): bool;`
