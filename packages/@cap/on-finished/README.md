# `@cap/on-finished`

Zero-heap HTTP request/response stream completion tracker with once-only finish/end/close/error lifecycle dispatch — clean-room CP equivalent of npm `on-finished`.

## Features
- `StreamLifecycle.new()` — fixed 16-listener zero-heap lifecycle state machine
- `onFinished(listener_id)` — registers listener or immediately fires if stream already finished
- `markEnd()`, `markFinish()`, `markClose()`, `markError(code)` — idempotent once-only dispatch preserving first completion reason
- `isFinished()`, `isReadableFinished()`, `isWritableFinished()`, `getListener(id)`
