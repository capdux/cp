[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @aws-sdk/util-arn-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap AWS ARN (Amazon Resource Name) parser, formatter, partition validator, resource type extractor, and IAM wildcard matcher.

## 安装与导入

```bash
cap add @aws-sdk/util-arn-parser
```

```cp
import { bytesEq } from "@aws-sdk/util-arn-parser";
```

## 核心导出 API

- `export fn bytesEq(a: string, b: string): bool;`
- `export fn isValidPartition(partition: string): bool;`
- `export fn parseArn(input: string): Arn;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatArn(arn: Arn, out: []u8): usize;`
- `export fn matchResourcePattern(resource: string, pattern: string): bool;`
- `export fn resourceType(resource: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@aws-sdk/util-arn-parser@1.0.0`)，许可证：Apache-2.0
- **上游生态**：`crates.io` (`aws-arn@1.0.0`)，许可证：MIT
