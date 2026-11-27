# postgres-interval@1.0.0 (Agent Quickref)

> Zero-allocation PostgreSQL INTERVAL parser and ISO-8601 / PostgreSQL bidirectional formatter in pure CP | License: MIT
> Import: `import { ... } from "postgres-interval";` | Full types: `src/index.d.cp`

## Functions
- `export fn pgivInvalid(): PgInterval;`
- `export fn pgivZero(): PgInterval;`
- `export fn pgivIsSpace(c: u8): bool;`
- `export fn pgivIsDigit(c: u8): bool;`
- `export fn pgivIsAlpha(c: u8): bool;`
- `export fn pgivToLower(c: u8): u8;`
- `export fn pgivEqIgnoreCase(a: string, b: string): bool;`
- `export fn pgivSkipSpace(s: string, start: usize): usize;`
- `export fn pgivParseUint(s: string, start: usize): PgivIntRes;`
- `export fn pgivParseFracMs(s: string, start: usize): PgivFracRes;`
- `export fn pgivAbsI32ToU64(v: i32): u64;`
- `export fn pgivParseIso(s: string, start: usize, end: usize): PgInterval;`
- `export fn parsePgInterval(input: string): PgInterval;`
- `export fn toIsoDuration(iv: PgInterval, out: []u8): usize;`
- `export fn toPostgresString(iv: PgInterval, out: []u8): usize;`
- `export fn totalSecondsApprox(iv: PgInterval): i64;`
