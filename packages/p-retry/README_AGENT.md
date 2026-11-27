# p-retry@1.0.0 (Agent Quickref)

> Zero-heap exponential backoff & retry policy state machine with min/max caps, jitter, and elapsed deadline (clean-room port of npm retry / p-retry) | License: MIT
> Import: `import { ... } from "p-retry";` | Full types: `src/index.d.cp`

## Functions
- `export fn rbkClampDelay(delay: u64, max_delay: u64): u64;`
- `export fn rbkMix64(x: u64): u64;`
- `export fn computeBackoffMs(policy: RetryPolicy, attempt: u32): u64;`
- `export fn applyJitterMs(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64;`
