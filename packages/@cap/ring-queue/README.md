# `@cap/ring-queue`

Zero-heap bounded ring queue (`RingQueueU64`) and wrap-around byte ring buffer (`ByteRing`) in pure CP (clean-room counterpart to Rust `crossbeam-queue` `ArrayQueue`).

## Features

- **100% Zero Heap Allocation**: Operates directly on caller-owned `[]u64` / `[]u8` backing buffers.
- **`RingQueueU64`**: O(1) `push`, `forcePush` (evicts oldest when full), `pop`, `peek`, `len`, `isEmpty`, `isFull`, `remaining`, `clear`.
- **`ByteRing`**: Bulk `writeBytes` and `readBytes` with automatic two-segment wrap-around copy.

## Quick Start

```cp
import { RingQueueU64, ByteRing } from "@cap/ring-queue";

let storage: [8]u64 = undefined;
let q = RingQueueU64.new(8);
q.push(&storage, 42);
let out: u64 = 0;
q.pop(&storage, &out); // out == 42
```

## License

MIT
