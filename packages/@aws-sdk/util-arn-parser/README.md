[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @aws-sdk/util-arn-parser

Zero-heap AWS ARN (Amazon Resource Name) parser, formatter, partition validator, resource type extractor, and IAM wildcard matcher.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@aws-sdk/util-arn-parser` ([https://github.com/aws/aws-sdk-js-v3](https://github.com/aws/aws-sdk-js-v3)) / `crates.io:aws-arn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `@aws-sdk/util-arn-parser` → CP 包名 `@aws-sdk/util-arn-parser`）

| Metadata | Value |
| --- | --- |
| Package | `@aws-sdk/util-arn-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @aws-sdk/util-arn-parser
```

```cp
import {
    Arn,
    bytesEq,
    isValidPartition,
    parseArn,
    appendSlice,
    formatArn,
    matchResourcePattern,
    resourceType,
} from "@aws-sdk/util-arn-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Arn` | `class` | Fields: `partition: string, service: string, region: string, account_id: string, resource: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bytesEq` | `(a: string, b: string): bool` | Invokes `bytesEq` and returns `bool`. |
| `isValidPartition` | `(partition: string): bool` | Checks predicate `isValidPartition` and returns `bool`. |
| `parseArn` | `(input: string): Arn` | Parses input via `parseArn` and returns `Arn`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `formatArn` | `(arn: Arn, out: []u8): usize` | Serializes/formats data via `formatArn` returning `usize`. |
| `matchResourcePattern` | `(resource: string, pattern: string): bool` | Invokes `matchResourcePattern` and returns `bool`. |
| `resourceType` | `(resource: string): string` | Invokes `resourceType` and returns `string`. |

## Minimal Example

```cp
import { isValidPartition } from "@aws-sdk/util-arn-parser";

pub fn main() {
    let result = isValidPartition("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
