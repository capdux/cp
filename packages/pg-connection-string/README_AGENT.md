# pg-connection-string@1.0.0 (Agent Quickref)

> Zero-heap PostgreSQL connection string parser and URI builder supporting both postgresql:// URIs and libpq key=value DSNs. | License: MIT
> Import: `import { ... } from "pg-connection-string";` | Full types: `src/index.d.cp`

## Functions
- `export fn makeInvalid(): PgConfig;`
- `export fn isWs(c: u8): bool;`
- `export fn parsePortStrict(s: string): i32;`
- `export fn isValidSslMode(mode: string): bool;`
- `export fn parseUri(uri: string): PgConfig;`
- `export fn parseKeyValue(dsn: string): PgConfig;`
- `export fn parse(input: string): PgConfig;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn appendU16(out: []u8, pos: usize, v: u16): i64;`
- `export fn formatUri(cfg: PgConfig, include_password: bool, out: []u8): usize;`
