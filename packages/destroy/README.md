[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `destroy`

> **Source Attribution**: `npm:destroy` ([stream-utils/destroy](https://github.com/stream-utils/destroy)) — License: MIT

Zero-heap safe, idempotent stream and file-descriptor destroyer with `ReadStream` deferred-close handling, lightweight `DestroyState` tracking, batch teardown helpers, and `on-finished` lifecycle integration in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`destroy`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: destroy)
import destroy from "destroy";
destroy(readStream);
```

```cp
// CP (zero-heap idempotent stream teardown)
import { DestroyableStream, destroy, isDestroyed } from "destroy";

let s = DestroyableStream.readFile(3, true);
let action = destroy(&mut s); // 1 (DESTROYED)
let done = isDestroyed(s);    // true
```

## Installation

Add `destroy` to your `cap.json`:

```json
{
  "dependencies": {
    "destroy": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `DestroyableStream` | `class` (`generic()`, `readFile(fd, fd_open)`, `zlib()`, `isDestroyed()`, `onOpen(fd)`) | Stream descriptor with `on-finished` `StreamLifecycle` |
| `DestroyState` | `class` (`new(resource_id)`, `isDestroyed()`, `destroy()`, `destroyWithError(err_code)`, `reset()`) | Lightweight idempotent destruction state tracker for arbitrary resources |
| `destroy` / `destroyStream` | `(s: &mut DestroyableStream): u8` | Idempotent teardown returning action code (`0`=NONE, `1`=DESTROYED, `2`=DEFERRED_CLOSE, `3`=ALREADY_DESTROYED) |
| `destroyWithError` | `(s: &mut DestroyableStream, err_code: i32): u8` | Destroy stream while propagating `err_code` to `lifecycle` |
| `isDestroyed` | `(s: DestroyableStream): bool` | True if stream is destroyed or its `lifecycle` is finished |
| `destroyAll` | `(streams: []DestroyableStream): usize` | Batch-destroy a slice of streams; returns newly destroyed count |
| `destroyAllStates` | `(states: []DestroyState): usize` | Batch-destroy a slice of `DestroyState`; returns newly destroyed count |
| `isAnyDestroyed` | `(streams: &[]DestroyableStream): bool` | True if at least one stream in the slice is destroyed |
| `areAllDestroyed` | `(streams: &[]DestroyableStream): bool` | True if every stream in the slice is destroyed |

## Practical Examples

### 1. Handling `ReadStream` Late Open (Deferred Close)

```cp
import { DestroyableStream, destroyStream, destroyWithError } from "destroy";

// File descriptor not opened yet (fd = -1, fd_open = false)
let rs = DestroyableStream.readFile(-1, false);

// Destroy before open -> returns 2 (DEFERRED_CLOSE)
let a1 = destroyStream(&mut rs);

// Later when OS opens fd 7, onOpen() immediately closes it -> returns 1 (DESTROYED)
let a2 = rs.onOpen(7);

// Zlib stream with error propagation
let zs = DestroyableStream.zlib();
let a3 = destroyWithError(&mut zs, -5);
```

### 2. Batch Teardown & `DestroyState` Tracking

```cp
import {
    DestroyableStream,
    DestroyState,
    destroyAll,
    destroyAllStates,
    isAnyDestroyed,
    areAllDestroyed,
} from "destroy";

let streams: [2]DestroyableStream = [
    DestroyableStream.generic(),
    DestroyableStream.zlib(),
];
let count = destroyAll(streams[0..2]); // 2
let all = areAllDestroyed(&streams[0..2]); // true
let any = isAnyDestroyed(&streams[0..2]);  // true

let states: [2]DestroyState = [DestroyState.new(101), DestroyState.new(102)];
let closed = destroyAllStates(states[0..2]); // 2
```

## Notes & Edge Cases

- **Idempotency**: Calling `destroy` / `destroyStream` multiple times on the same stream safely returns `3` (`ALREADY_DESTROYED`) without double-closing file descriptors.
- **Action codes**: `0` = none, `1` = destroyed immediately, `2` = deferred close (waiting for `onOpen`), `3` = already destroyed.
- **Zero heap allocations**: Reuses `on-finished` (`StreamLifecycle`) directly by value.
