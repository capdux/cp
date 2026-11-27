[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @faker-js/faker

Zero-allocation deterministic fake data generator (xoshiro256** PRNG, person/network/text/commerce/UUIDv4/Luhn card/collections) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@faker-js/faker` ([https://github.com/faker-js/faker](https://github.com/faker-js/faker))（原库名 `@faker-js/faker` → CP 包名 `@faker-js/faker`）

| Metadata | Value |
| --- | --- |
| Package | `@faker-js/faker` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @faker-js/faker
```

```cp
import {
    Faker,
    splitmixStep,
    rotl7,
    rotl45,
    appendByte,
    appendBytes,
    appendLowerAscii,
    appendCapitalized,
} from "@faker-js/faker";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Faker` | `class` | Fields: `s0: u64, s1: u64, s2: u64, s3: u64` · Methods: `init, reseed, nextU64, nextInt, nextBool, pickStr, +23 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `splitmixStep` | `(state: &mut u64): u64` | Invokes `splitmixStep` and returns `u64`. |
| `rotl7` | `(x: u64): u64` | Invokes `rotl7` and returns `u64`. |
| `rotl45` | `(x: u64): u64` | Invokes `rotl45` and returns `u64`. |
| `appendByte` | `(out: []u8, pos: &mut usize, b: u8)` | Executes `appendByte` in-place. |
| `appendBytes` | `(out: []u8, pos: &mut usize, data: &[]u8)` | Executes `appendBytes` in-place. |
| `appendLowerAscii` | `(out: []u8, pos: &mut usize, data: &[]u8)` | Executes `appendLowerAscii` in-place. |
| `appendCapitalized` | `(out: []u8, pos: &mut usize, data: &[]u8)` | Executes `appendCapitalized` in-place. |
| `appendU64Dec` | `(out: []u8, pos: &mut usize, val: u64)` | Executes `appendU64Dec` in-place. |
| `appendDecPadded` | `(out: []u8, pos: &mut usize, val: u64, width: usize)` | Executes `appendDecPadded` in-place. |
| `appendHexByte` | `(out: []u8, pos: &mut usize, b: u8)` | Executes `appendHexByte` in-place. |
| `isLeapYear` | `(year: u64): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(year: u64, month: u64): u64` | Invokes `daysInMonth` and returns `u64`. |
| `isValidLuhn` | `(digits: &[]u8): bool` | Checks predicate `isValidLuhn` and returns `bool`. |

## Minimal Example

```cp
import { isLeapYear } from "@faker-js/faker";

pub fn main() {
    let result = isLeapYear(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
