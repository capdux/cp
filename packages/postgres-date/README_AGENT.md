# postgres-date — AI Agent Quick Ref

Zero-allocation PostgreSQL TIMESTAMP / TIMESTAMPTZ / DATE / infinity parser with microsecond and timezone offset precision (clean-room MIT implementation of npm postgres-date, #505 / 112M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PgDateKind,
    PgDateTime,
    pgdParseDigits,
    pgdIsLeap,
    pgdDaysInMonth,
    parsePgDate,
    toUnixMs,
} from "postgres-date";
```

## Core Types & Constants

```cp
enum PgDateKind { Invalid, Finite, PosInfinity, NegInfinity }
class PgDateTime { kind: PgDateKind, year: i32, month: u8, day: u8, /* +6 fields */ } // methods: isValid(...)
```

## API Signatures

```cp
fn pgdParseDigits(s: string): i32?;
fn pgdIsLeap(y: i32): bool;
fn pgdDaysInMonth(y: i32, m: u8): u8;
fn parsePgDate(input: string): PgDateTime;
fn toUnixMs(dt: PgDateTime): i64?;
```

## Minimal Usage

```cp
import { parsePgDate } from "postgres-date";

pub fn main() {
    let result = parsePgDate("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"postgres-date"`; install via `cap add postgres-date`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
