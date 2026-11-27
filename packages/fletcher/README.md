[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# fletcher

Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 position-sensitive checksums with both one-shot helpers and streaming incremental hashers (`Fletcher16`, `Fletcher32`, `Fletcher64`). Ported from Rust `fletcher`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `fletcher` (`aaronriekenberg/rust-fletcher`) |
| **CP Package** | `fletcher` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: fletcher)
use fletcher::{calc_fletcher16, Fletcher16};
let sum = calc_fletcher16(b"abcde");
```

```typescript
// CP (same package name: "fletcher")
import { fletcher16, calcFletcher16, Fletcher16 } from "fletcher";
let sum = fletcher16("abcde");
```

## Installation

```bash
cap add fletcher
```

## Quick Start & Practical Examples

### 1. One-Shot Checksums (Strings, Byte Slices & Word Slices)

```typescript
import {
    fletcher16,
    fletcher16Bytes,
    calcFletcher16,
    fletcher32,
    fletcher32Bytes,
    calcFletcher32,
    fletcher64,
    fletcher64Bytes,
    calcFletcher64,
} from "fletcher";

fn oneShotChecksums(): void {
    // Fletcher-16 (mod 255)
    let s16 = fletcher16("abcde");
    let raw: [5]u8 = [97, 98, 99, 100, 101];
    let b16 = fletcher16Bytes(raw[0..5]);
    let c16 = calcFletcher16(raw[0..5]); // alias matching Rust calc_fletcher16

    // Fletcher-32 (mod 65535, 16-bit words or byte stream)
    let s32 = fletcher32("abcde");
    let b32 = fletcher32Bytes(raw[0..5]);
    let words16: [2]u16 = [0x6261, 0x6463];
    let c32 = calcFletcher32(words16[0..2]);

    // Fletcher-64 (mod 4294967295, 32-bit words or byte stream)
    let s64 = fletcher64("abcdefgh");
    let b64 = fletcher64Bytes(raw[0..5]);
    let words32: [2]u32 = [0x64636261, 0x68676665];
    let c64 = calcFletcher64(words32[0..2]);
}
```

### 2. Streaming Incremental Hashers (`Fletcher16`, `Fletcher32`, `Fletcher64`)

```typescript
import { Fletcher16, Fletcher32, Fletcher64 } from "fletcher";

fn streamingChecksums(): void {
    // Fletcher16 with chunked updates & custom initial seed
    let mut h16 = Fletcher16.new(); // alias: Fletcher16.init()
    h16.update("abc");
    h16.update("de");
    let d16 = h16.digest(); // alias: h16.value()
    h16.reset();

    let mut seeded16 = Fletcher16.withInitialValues(1, 2);
    seeded16.update("hello");

    // Fletcher32 handles odd-length byte chunks seamlessly via internal pending byte
    let mut h32 = Fletcher32.new();
    h32.update("abc");
    h32.update("de");
    let d32 = h32.digest();

    // Fletcher64 handles arbitrary unaligned byte chunks or u32 word slices
    let mut h64 = Fletcher64.new();
    h64.update("abcd");
    h64.update("efgh");
    let d64 = h64.value();
}
```

## API Reference

### Streaming Classes
- `Fletcher16`: `new()` / `init()`, `withInitialValues(a: u8, b: u8)`, `update(data: string)`, `updateBytes(data: &[]u8)`, `digest(): u16` / `value(): u16`, `reset()`
- `Fletcher32`: `new()` / `init()`, `withInitialValues(a: u16, b: u16)`, `update(data: string)`, `updateBytes(data: &[]u8)`, `updateWords(words: &[]u16)`, `digest(): u32` / `value(): u32`, `reset()`
- `Fletcher64`: `new()` / `init()`, `withInitialValues(a: u32, b: u32)`, `update(data: string)`, `updateBytes(data: &[]u8)`, `updateWords(words: &[]u32)`, `digest(): u64` / `value(): u64`, `reset()`

### One-Shot Functions
- `fletcher16(data: string): u16`, `fletcher16Bytes(data: &[]u8): u16`, `calcFletcher16(data: &[]u8): u16`
- `fletcher32(data: string): u32`, `fletcher32Bytes(data: &[]u8): u32`, `calcFletcher32(words: &[]u16): u32`
- `fletcher64(data: string): u64`, `fletcher64Bytes(data: &[]u8): u64`, `calcFletcher64(words: &[]u32): u64`
