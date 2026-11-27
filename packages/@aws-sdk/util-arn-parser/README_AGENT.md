# @aws-sdk/util-arn-parser@1.0.0 (Agent Quickref)

> Zero-heap AWS ARN (Amazon Resource Name) parser, formatter, partition validator, resource type extractor, and IAM wildcard matcher. | License: MIT
> Import: `import { ... } from "@aws-sdk/util-arn-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn bytesEq(a: string, b: string): bool;`
- `export fn isValidPartition(partition: string): bool;`
- `export fn parseArn(input: string): Arn;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatArn(arn: Arn, out: []u8): usize;`
- `export fn matchResourcePattern(resource: string, pattern: string): bool;`
- `export fn resourceType(resource: string): string;`
