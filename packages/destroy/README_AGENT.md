# `destroy` — AI Agent Quick Reference

- **Package**: `destroy` (`1.0.0`, MIT)
- **Origin**: `npm:destroy` (reuses `on-finished`)
- **Import**: `import { DestroyableStream, DestroyState, destroy, destroyStream, destroyWithError, isDestroyed, destroyAll, destroyAllStates, isAnyDestroyed, areAllDestroyed } from "destroy";`

## Types & Action Codes

```cp
// Action codes returned by destroy / destroyStream / destroyWithError / onOpen:
// 0 = NONE, 1 = DESTROYED, 2 = DEFERRED_CLOSE, 3 = ALREADY_DESTROYED

class DestroyableStream {
    kind: u8, has_fd: bool, fd: i32, fd_open: bool, destroyed: bool,
    destroy_calls: usize, close_calls: usize, lifecycle: StreamLifecycle,
    fn generic(): DestroyableStream,
    fn readFile(fd: i32, fd_open: bool): DestroyableStream,
    fn zlib(): DestroyableStream,
    fn isDestroyed(): bool,
    fn onOpen(_: auto, fd: i32): u8,
}
class DestroyState {
    resource_id: u64, destroyed: bool, destroy_calls: usize, err_code: i32,
    fn new(resource_id: u64): DestroyState,
    fn isDestroyed(): bool,
    fn destroy(_: auto): bool,
    fn destroyWithError(_: auto, err_code: i32): bool,
    fn reset(_: auto),
}
```

## Core Signatures

```cp
fn destroy(s: &mut DestroyableStream): u8
fn destroyStream(s: &mut DestroyableStream): u8
fn destroyWithError(s: &mut DestroyableStream, err_code: i32): u8
fn isDestroyed(s: DestroyableStream): bool
fn destroyAll(streams: []DestroyableStream): usize
fn destroyAllStates(states: []DestroyState): usize
fn isAnyDestroyed(streams: &[]DestroyableStream): bool
fn areAllDestroyed(streams: &[]DestroyableStream): bool
```

## Minimal Pattern

```cp
let s = DestroyableStream.readFile(3, true);
let action = destroy(&mut s); // 1 (DESTROYED)
```

## Pitfalls

1. **Mutable borrow required**: `destroy` / `destroyStream` / `destroyWithError` take `&mut DestroyableStream`; `isAnyDestroyed` / `areAllDestroyed` take `&[]DestroyableStream`.
2. **Deferred close**: Calling `destroy` on a `readFile(-1, false)` stream returns `2`; call `s.onOpen(fd)` once the descriptor opens to complete teardown (`1`).
3. **Reusable `DestroyState`**: Call `state.reset()` to reuse a `DestroyState` slot in object pools.
