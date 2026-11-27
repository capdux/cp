[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# fast-fifo

> **Source Attribution**: Clean-room MIT CP port of **npm** [`fast-fifo`](https://github.com/mafintosh/fast-fifo) (`v1.3.2` / `MIT`), CP package `fast-fifo@1.3.2`.

Zero-heap O(1) fixed-capacity ring-buffer FIFO queue for `i64` and `string` items in pure CP.

## Why CP `fast-fifo`?

- **Zero Heap Allocation**: Fixed 256-slot (`FIFO_CAP = 256`) power-of-two ring buffers (`FifoI64`, `FifoStr`) with bitmask index wraparound (`& 255`) — no modulo division, no dynamic allocation.
- **Zero-Overhead Stack Initialization**: `newFifoI64()` and `newFifoStr()` initialize backing `buf` arrays as `undefined`, eliminating 2KB–4KB stack zeroing on construction.
- **Strict O(1) Operations**: `push`, `shift`, `peek`, `len`, `isEmpty`, and `clear` execute in constant time with predictable cache locality.

## Zero-Cost Migration (JS/TS → CP)

Keep the exact same package name (`fast-fifo`) — no renaming required.

**TypeScript (`package.json` / `index.ts`):**
```typescript
import FIFO from "fast-fifo";
const q = new FIFO<number>();
q.push(42);
const v = q.shift();
```

**CP (`cap.json` / `main.cp`):**
```typescript
import { FifoI64, FifoStr, newFifoI64, newFifoStr, FIFO_CAP } from "fast-fifo";
```

## Quick Start

### Integer Queue (`FifoI64`)

```typescript
import { newFifoI64 } from "fast-fifo";

export fn demoIntQueue(): i64 {
    let mut q = newFifoI64();
    _ = q.push(10);
    _ = q.push(20);
    _ = q.push(30);

    let mut sum: i64 = 0;
    while (!q.isEmpty()) {
        if (let v = q.shift()) {
            sum += v;
        }
    }
    return sum; // 60
}
```

### String Task Queue (`FifoStr`)

```typescript
import { newFifoStr } from "fast-fifo";

export fn demoStrQueue(): usize {
    let mut q = newFifoStr();
    _ = q.push("job-1");
    _ = q.push("job-2");

    if (let next = q.peek()) {
        _ = next; // "job-1" without removing
    }
    let _first = q.shift();
    return q.len(); // 1
}
```

## API Reference

- **Constant**: `FIFO_CAP: usize = 256`
- **Constructors**: `newFifoI64(): FifoI64`, `newFifoStr(): FifoStr`
- **`FifoI64` Methods**:
  - `len(): usize`, `isEmpty(): bool`, `isFull(): bool`
  - `push(val: i64): bool` — returns `false` when full (`count >= 256`)
  - `shift(): i64?` — removes and returns oldest item, or `null` if empty
  - `peek(): i64?` — returns oldest item without removing, or `null` if empty
  - `clear()` — resets head, tail, and count in O(1)
- **`FifoStr` Methods**: `len()`, `isEmpty()`, `push(val: string): bool`, `shift(): string?`, `peek(): string?`, `clear()`.

## License

MIT
