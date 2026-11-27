# bl — AI Agent Quick Ref

Zero-heap segmented `BufferList` for streaming byte reads, cross-chunk BE/LE integer decoding, `indexOf`, and `consume` (npm `bl` 6.1.0 parity).

## Import

```cp
import {
    BL_MAX_CHUNKS,
    BufferList,
    newBufferList,
} from "bl";
```

## API Signatures

```cp
const BL_MAX_CHUNKS: usize; // 64

class BufferList {
    chunks: [64]&[]u8,
    chunk_count: usize,
    total_len: usize,
    fn len(): usize;
    fn isEmpty(): bool;
    fn append(*self, chunk: &[]u8): bool;
    fn get(index: usize): u8?;
    fn copyRange(start: usize, end: usize, dst: []u8): usize;
    fn consume(*self, n: usize);
    fn readU16BE(offset: usize): u16?;
    fn readU16LE(offset: usize): u16?;
    fn readU32BE(offset: usize): u32?;
    fn readU32LE(offset: usize): u32?;
    fn indexOf(needle: &[]u8, from_offset: usize): usize?;
}

fn newBufferList(): BufferList;
```

## Minimal Example

```cp
import { newBufferList } from "bl";

let bl = newBufferList();
let _ = bl.append("hello ");
let _ = bl.append("world");
let pos = bl.indexOf("wo", 0); // 6
let out: [16]u8 = undefined;
let n = bl.copyRange(0, 5, out[0..16]); // "hello"
bl.consume(6); // remaining: "world"
```

## Pitfalls & Borrowing Rules

- **Borrowed chunk lifetimes**: `append` stores read-only slice references (`&[]u8`) without copying; every appended buffer must outlive the `BufferList` (or until `consume` drops it).
- **Chunk limit**: Holds at most `BL_MAX_CHUNKS` (64) active chunks; `append` returns `false` when full (empty chunks are ignored and return `true`).
- **Optional bounds checks**: `get`, `readU16*`, `readU32*`, and `indexOf` return `T?` (`null` on out-of-bounds or not found); always check `!= null` before unwrapping.
- **Mutable receiver**: Only `append` and `consume` mutate `BufferList`; all read/search methods are read-only.
