# async-retry@1.0.0 (Agent Quickref)

> Zero-allocation industrial retry & backoff engine (Fixed/Linear/Exponential/Fibonacci + AWS Full/Equal/Decorrelated Jitter + RetryBudget) for CP | License: MIT
> Import: `import { ... } from "async-retry";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum BackoffStrategy`
- `export enum JitterMode`
- `export enum FailureKind`
- `export enum StopReason`

## Functions
- `export fn saturatingMulU64(a: u64, b: u64): u64;`
- `export fn clampDelay(delay: u64, max_delay: u64): u64;`
