# rfc3339@1.0.0 (Agent Quickref)

> Zero-heap strict RFC 3339 timestamp parser, Unix epoch converter, and UTC formatter (reuses iso8601) | License: MIT
> Import: `import { ... } from "rfc3339";` | Full types: `src/index.d.cp`
> Deps: `iso8601`

## Functions
- `export fn rfcIsDigit(c: u8): bool;`
- `export fn daysFromCivil(year: i32, month: u8, day: u8): i64;`
- `export fn civilFromDays(z_in: i64): IsoDate;`
- `export fn isValidRfc3339(s: string): bool;`
- `export fn parseRfc3339(s: string): Rfc3339Timestamp?;`
- `export fn fromUnixSeconds(unix_secs: i64): IsoDateTime;`
- `export fn formatRfc3339Utc(unix_secs: i64, out: []u8): usize;`
- `export fn isRfcLeapYear(year: i32): bool;`
