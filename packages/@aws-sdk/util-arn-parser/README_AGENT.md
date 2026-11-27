# @aws-sdk/util-arn-parser — AI Agent Quick Ref

Zero-heap AWS ARN (Amazon Resource Name) parser, formatter, partition validator, resource type extractor, and IAM wildcard matcher. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Arn { partition: string, service: string, region: string, account_id: string, /* +2 fields */ }
```

## API Signatures

```cp
fn bytesEq(a: string, b: string): bool;
fn isValidPartition(partition: string): bool;
fn parseArn(input: string): Arn;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn formatArn(arn: Arn, out: []u8): usize;
fn matchResourcePattern(resource: string, pattern: string): bool;
fn resourceType(resource: string): string;
```

## Minimal Usage

```cp
import { isValidPartition } from "@aws-sdk/util-arn-parser";

pub fn main() {
    let result = isValidPartition("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@aws-sdk/util-arn-parser"`; install via `cap add @aws-sdk/util-arn-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
