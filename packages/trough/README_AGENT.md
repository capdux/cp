# trough@2.2.0 (Agent Quickref)

> Zero-heap middleware pipeline engine and tag router in pure CP (covers unified & zwitch) | License: MIT
> Import: `import { ... } from "trough";` | Full types: `src/index.d.cp`

## Functions
- `export fn trghStrEq(a: string, b: string): bool;`
- `export fn trghIdentity(v: i64): StageOutcome;`
- `export fn stageOk(value: i64): StageOutcome;`
- `export fn stageErr(err_code: i32): StageOutcome;`
- `export fn stageHalt(value: i64): StageOutcome;`
- `export fn dispatchTag(tag: string, routes: &[]TagRoute): i32;`
