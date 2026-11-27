# @web/pressure — AI Agent Quick Ref

Official overload protection, body limit (413), load shedding (503), and K8s /healthz /readyz probes for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PressureConfig,
    pressureSetReady,
    pressureSetInflightForTest,
    pressureShedCount,
    pressureResetStats,
    pressStrEql,
    pressureOnRequest,
    pressureOnAfter,
} from "@web/pressure";
```

## Core Types & Constants

```cp
class PressureConfig { max_body_bytes: usize, max_inflight: u32, retry_after_secs: string, health_path: string, /* +1 fields */ }
```

## API Signatures

```cp
fn pressureSetReady(ready: bool);
fn pressureSetInflightForTest(n: u32);
fn pressureShedCount(): u64;
fn pressureResetStats();
fn pressStrEql(a: string, b: string): bool;
fn pressureOnRequest(ctx: &mut Context): bool;
fn pressureOnAfter(ctx: &mut Context, resp: &mut Response);
fn handleHealthz(ctx: &mut Context): Response;
fn handleReadyz(ctx: &mut Context): Response;
fn createPressurePlugin(cfg: PressureConfig): Plugin;
fn registerPressure(app: &mut App, cfg: PressureConfig): &mut App;
```

## Minimal Usage

```cp
import { pressStrEql } from "@web/pressure";

pub fn main() {
    let result = pressStrEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/pressure"`; install via `cap add @web/pressure`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
