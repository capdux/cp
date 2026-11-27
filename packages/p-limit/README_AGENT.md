# p-limit — AI Agent Quick Ref

Zero-allocation concurrency limiter and FIFO task slot scheduler (p-limit parity) powered by yocto-queue in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { SubmitResult, CompleteResult, PLimit, plClampConcurrency } from "p-limit";
```

## Core Types & Constants

```cp
class SubmitResult { started_immediately: bool, queued: bool, rejected: bool }
class CompleteResult { has_next: bool, next_task_id: u64 }
class PLimit { limit: usize, active: usize, queue: YoctoQueueU64 } // methods: new(...) +12
```

## API Signatures

```cp
fn plClampConcurrency(c: usize): usize;
```

## Minimal Usage

```cp
import { plClampConcurrency } from "p-limit";

pub fn main() {
    let result = plClampConcurrency(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-limit"`; install via `cap add p-limit`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
