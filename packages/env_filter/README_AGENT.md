# env_filter@1.0.0 (Agent Quickref)

> Zero-allocation RUST_LOG-style hierarchical log filter parser and longest-prefix module matcher (Rust env_filter parity) for CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "env_filter";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum LogLevel`

## Functions
- `export fn efltLevelRank(lvl: LogLevel): u8;`
- `export fn efltIsSpace(c: u8): bool;`
- `export fn efltTrim(s: string): string;`
- `export fn efltToLower(c: u8): u8;`
- `export fn efltEqIgnoreCase(a: string, b: string): bool;`
- `export fn efltStrEq(a: string, b: string): bool;`
- `export fn parseLogLevel(s: string): LogLevel?;`
- `export fn efltMatchesTarget(target: string, directive_target: string): bool;`
- `export fn efltFindByte(s: string, needle: u8): usize?;`
