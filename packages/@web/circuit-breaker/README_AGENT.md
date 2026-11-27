# @web/circuit-breaker — AI Agent Quick Ref

Official Envoy / Tower three-state circuit breaker (Closed / Open / HalfOpen) plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CircuitState,
    CircuitBreakerConfig,
    cbNowSec,
    cbState,
    cbFailureCount,
    cbSetTestClock,
    cbReset,
    cbBeforeHandle,
} from "@web/circuit-breaker";
```

## Core Types & Constants

```cp
enum CircuitState { Closed, Open, HalfOpen }
class CircuitBreakerConfig { failure_threshold: u32, reset_timeout_secs: u64, retry_after_secs: string }
```

## API Signatures

```cp
fn cbNowSec(): u64;
fn cbState(): CircuitState;
fn cbFailureCount(): u32;
fn cbSetTestClock(now_sec: u64);
fn cbReset();
fn cbBeforeHandle(ctx: &mut Context): bool;
fn cbAfterHandle(ctx: &mut Context, resp: &mut Response);
fn cbCreatePlugin(cfg: CircuitBreakerConfig): Plugin;
fn registerCircuitBreaker(app: &mut App, cfg: CircuitBreakerConfig): &mut App;
```

## Minimal Usage

```cp
import { cbNowSec } from "@web/circuit-breaker";

pub fn main() {
    let result = cbNowSec();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/circuit-breaker"`; install via `cap add @web/circuit-breaker`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
