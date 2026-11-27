# fast-fifo (AI Agent Quick Ref)

- **Import**: `import { FifoI64, FifoStr, newFifoI64, newFifoStr, FIFO_CAP } from "fast-fifo";`
- **Source**: `npm:fast-fifo@1.3.2` (MIT, zero-heap O(1) 256-slot ring buffer, `undefined` backing buf).

## Core Signatures

```cp
export const FIFO_CAP: usize = 256;

export class FifoI64 {
    buf: [256]i64, head: usize, tail: usize, count: usize,
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn push(_: auto, val: i64): bool;
    fn shift(_: auto): i64?;
    fn peek(): i64?;
    fn clear(_: auto);
}
export fn newFifoI64(): FifoI64;

export class FifoStr {
    buf: [256]string, head: usize, tail: usize, count: usize,
    fn len(): usize;
    fn isEmpty(): bool;
    fn push(_: auto, val: string): bool;
    fn shift(_: auto): string?;
    fn peek(): string?;
    fn clear(_: auto);
}
export fn newFifoStr(): FifoStr;
```

## Minimal Pattern

```cp
let mut q = newFifoI64();
if (q.push(42)) {
    if (let v = q.shift()) { /* v == 42 */ }
}
```

## Pitfalls

1. **Fixed capacity 256**: `push` returns `false` when `count >= FIFO_CAP` (256); always check return value if queue may fill up.
2. **`FifoStr` has no `isFull()`**: Check `q.len() >= FIFO_CAP` or `push(...) == false`.
3. **Mutable variable required**: Declare `let mut q = newFifoI64()` because `push`, `shift`, and `clear` mutate queue state.
4. **Never read `q.buf` directly**: Unwritten slots are `undefined`; always access via `shift()` / `peek()`.
