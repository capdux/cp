# `@cap/mitt`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mitt` ([https://github.com/developit/mitt](https://github.com/developit/mitt))

Zero-heap-allocation tiny functional event emitter with wildcard `"*"` support in pure CP, modeled after npm `mitt` v3.

## Features

- **Fixed-slot zero-allocation `Emitter`** — Holds up to `32` active named or wildcard (`"*"`) listeners inline with zero heap allocations.
- **`on(*_, event_type, handler_id)`** — Registers a listener in FIFO registration order (supports `"*"` wildcard).
- **`off(*_, event_type, handler_id)`** — Removes a specific `(event_type, handler_id)` listener, or removes all listeners for `event_type` when `handler_id == 0`.
- **`emit(*_, event_type, payload)`** — Dispatches exact-match listeners first, followed by `"*"` wildcard listeners; records every dispatch in `last_dispatched[0..last_dispatch_count]`, increments `emit_count`, and returns the triggered count.
- **Introspection & lifecycle** — `listenerCount(event_type)`, `totalListeners()`, `hasListener(event_type, handler_id)`, `clear(*_)`, plus `mitt()` factory helper.

## Usage

```cp
import { Emitter, mitt } from "@cap/mitt";

let bus: Emitter = mitt();
* = bus.on("user:login", 101);
* = bus.on("*", 999);

const fired: usize = bus.emit("user:login", 42);
// fired == 2; bus.last_dispatched[0].handler_id == 101, bus.last_dispatched[1].handler_id == 999
```

## License

MIT
