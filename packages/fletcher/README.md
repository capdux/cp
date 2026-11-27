[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fletcher

Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 position-sensitive checksums with both one-shot functions and streaming hashers in pure CP.

- **Package**: `fletcher` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/fletcher` (`MIT`, `rust-fletcher`), supporting byte, word, and string feeds plus custom initial seeds.

## Installation & Import

```bash
cap add fletcher
```

```typescript
import {
    Fletcher16, Fletcher32, Fletcher64,
    fletcher16, fletcher16Bytes, calcFletcher16,
    fletcher32, fletcher32Bytes, calcFletcher32,
    fletcher64, fletcher64Bytes, calcFletcher64,
} from "fletcher";
```

## Core API

### Streaming Hasher Classes

| Class | Constructors & Methods | Description |
| --- | --- | --- |
| `Fletcher16` | `new()`, `init()`, `withInitialValues(a: u8, b: u8)`, `update(data: string)`, `updateBytes(data: &[]u8)`, `digest(): u16`, `value(): u16`, `reset()` | 16-bit streaming Fletcher checksum over bytes (`mod 255`). |
| `Fletcher32` | `new()`, `init()`, `withInitialValues(a: u16, b: u16)`, `updateWords(words: &[]u16)`, `updateBytes(data: &[]u8)`, `update(data: string)`, `digest(): u32`, `value(): u32`, `reset()` | 32-bit streaming Fletcher checksum over 16-bit words (`mod 65535`), handling odd trailing bytes automatically. |
| `Fletcher64` | `new()`, `init()`, `withInitialValues(a: u32, b: u32)`, `updateWords(words: &[]u32)`, `updateBytes(data: &[]u8)`, `update(data: string)`, `digest(): u64`, `value(): u64`, `reset()` | 64-bit streaming Fletcher checksum over 32-bit words (`mod 0xFFFFFFFF`), buffering partial 4-byte tails. |

### One-Shot Checksum Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fletcher16` | `(data: string): u16` | One-shot Fletcher-16 over a string. |
| `fletcher16Bytes` / `calcFletcher16` | `(data: &[]u8): u16` | One-shot Fletcher-16 over a byte slice. |
| `fletcher32` | `(data: string): u32` | One-shot Fletcher-32 over a string. |
| `fletcher32Bytes` | `(data: &[]u8): u32` | One-shot Fletcher-32 over a byte slice. |
| `calcFletcher32` | `(words: &[]u16): u32` | One-shot Fletcher-32 over a `u16` word slice (matches Rust `calc_fletcher32`). |
| `fletcher64` | `(data: string): u64` | One-shot Fletcher-64 over a string. |
| `fletcher64Bytes` | `(data: &[]u8): u64` | One-shot Fletcher-64 over a byte slice. |
| `calcFletcher64` | `(words: &[]u32): u64` | One-shot Fletcher-64 over a `u32` word slice (matches Rust `calc_fletcher64`). |

## Examples

### 1. One-Shot Checksums

```typescript
import { fletcher16, fletcher32, fletcher64 } from "fletcher";

fn main() {
    let c16 = fletcher16("abcde");   // 0xC8F0
    let c32 = fletcher32("abcdef");  // 32-bit checksum
    let c64 = fletcher64("abcdefgh");
}
```

### 2. Streaming Multi-Chunk Hashing

```typescript
import { Fletcher32 } from "fletcher";

fn main() {
    let h = Fletcher32.new();
    h.update("abc");
    h.update("def");
    let sum = h.digest();
    h.reset();
}
```

### 3. Custom Initial Values & Word Slices

```typescript
import { Fletcher16, calcFletcher32 } from "fletcher";

fn main() {
    let h = Fletcher16.withInitialValues(0xFF, 0xFF);
    h.update("hello");
    let seeded = h.value();

    let words: [3]u16 = [0x0102, 0x0304, 0x0506];
    let wsum = calcFletcher32(words[0..]);
}
```

## Differences from Upstream & Highlights

- **Zero Heap & Pending Byte Buffering**: `Fletcher32` and `Fletcher64` store incomplete trailing bytes inline (`pending`), so arbitrary chunk boundaries produce identical digests to contiguous input.
- **Word & Byte Overloads**: Provides both `calcFletcher32(&[]u16)` / `calcFletcher64(&[]u32)` matching upstream Rust word semantics and `fletcher32Bytes` / `fletcher64Bytes` for raw byte streams.
- **Non-Destructive Digest**: `digest()` / `value()` read the current checksum without resetting internal state; call `reset()` explicitly to reuse a hasher.
