# backoff@1.0.0 (Agent Quickref)

> Zero-allocation deterministic retry backoff engine with Exponential, Linear, and AWS Full/Equal/Decorrelated Jitter (Rust backoff / npm exponential-backoff parity) for CP | License: MIT
> Import: `import { ... } from "backoff";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum JitterKind`

## Functions
- `export fn expDelayMs(attempt: u32, initial_ms: u64, max_ms: u64): u64;`
- `export fn linearDelayMs(attempt: u32, step_ms: u64, max_ms: u64): u64;`
