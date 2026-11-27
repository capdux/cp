# postgres-date@1.0.0 (Agent Quickref)

> Zero-allocation PostgreSQL TIMESTAMP / TIMESTAMPTZ / DATE / infinity parser with microsecond and timezone offset precision (clean-room MIT implementation of npm postgres-date, #505 / 112M wk) | License: MIT
> Import: `import { ... } from "postgres-date";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum PgDateKind`

## Functions
- `export fn parsePgDate(input: string): PgDateTime;`
- `export fn toUnixMs(dt: PgDateTime): i64?;`
