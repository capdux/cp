# p-try@1.0.0 (Agent Quickref)

> Zero-heap structured sync/async execution outcome wrapper preventing escaping errors, with safe integer parsing, division, and short-circuit chaining (clean-room port of npm p-try) | License: MIT
> Import: `import { ... } from "p-try";` | Full types: `src/index.d.cp`

## Functions
- `export fn ptryIsSpace(ch: u8): bool;`
- `export fn ptryIsDigit(ch: u8): bool;`
- `export fn ptryI64Min(): i64;`
- `export fn ptryAbsU64(v: i64): u64;`
- `export fn tryOkInt(v: i64): TryOutcome;`
- `export fn tryOkStr(s: string): TryOutcome;`
- `export fn tryErr(code: i32, msg: string): TryOutcome;`
- `export fn unwrapOrInt(o: TryOutcome, fallback: i64): i64;`
- `export fn unwrapOrStr(o: TryOutcome, fallback: string): string;`
- `export fn tryParseI64(s: string): TryOutcome;`
- `export fn tryDivI64(a: i64, b: i64): TryOutcome;`
- `export fn tryChain(first: TryOutcome, next_ok: bool, next_val: i64, next_err_code: i32, next_err_msg: string): TryOutcome;`
