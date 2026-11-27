# @web/circuit-breaker@1.0.0 (Agent Quickref)

> Official Envoy / Tower three-state circuit breaker (Closed / Open / HalfOpen) plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/circuit-breaker";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CircuitState`

## Functions
- `export fn cbNowSec(): u64;`
- `export fn cbState(): CircuitState;`
- `export fn cbFailureCount(): u32;`
- `export fn cbSetTestClock(now_sec: u64);`
- `export fn cbReset();`
- `export fn cbBeforeHandle(ctx: &mut Context): bool;`
- `export fn cbAfterHandle(ctx: &mut Context, resp: &mut Response);`
- `export fn cbCreatePlugin(cfg: CircuitBreakerConfig): Plugin;`
- `export fn registerCircuitBreaker(app: &mut App, cfg: CircuitBreakerConfig): &mut App;`
