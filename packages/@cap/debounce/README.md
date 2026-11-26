# `@cap/debounce`

Zero-heap-allocation, deterministic clock-driven `Debouncer` and `Throttler` state machines in pure CP.

## Features

- **`Debouncer`**: Full `leading`, `trailing`, `maxWait`, `poll`, `flush`, `cancel`, `isPending`, `execCount`, and `nextDueMs`.
- **`Throttler`**: Fixed-interval rate limiting with `leading`, `trailing`, `poll`, `cancel`, `isPending`, and `execCount`.
- **Deterministic & Zero Dependencies**: Driven entirely by caller-supplied `now_ms: u64`; no OS clock calls, no heap allocations, no FFI.

## API

```cp
import { Debouncer, Throttler } from "@cap/debounce";
```

## License

MIT
