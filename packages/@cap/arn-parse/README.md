# @cap/arn-parse

Zero-heap AWS ARN (Amazon Resource Name) parser, formatter, partition validator, resource type extractor, and IAM wildcard matcher in pure CP.

## Features

- `parseArn(input)`: zero-copy parsing of `arn:partition:service:region:account-id:resource` (supports empty region/account and multi-colon or slash resources)
- `isValidPartition(partition)`: validates `aws`, `aws-cn`, `aws-us-gov`, `aws-iso`, `aws-iso-b`
- `formatArn(arn, out)`: zero-heap serialization into caller buffer
- `matchResourcePattern(resource, pattern)`: non-recursive IAM `*` and `?` wildcard matcher
- `resourceType(resource)`: extracts resource type prefix before `/` or `:`

## Usage

```cp
import { parseArn, resourceType, matchResourcePattern } from "@cap/arn-parse";

const a = parseArn("arn:aws:s3:::my-bucket/logs/2026.txt");
const ok = matchResourcePattern(a.resource, "my-bucket/logs/*.txt"); // true
```
