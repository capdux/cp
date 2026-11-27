[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# uuidv7

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9562 UUIDv7 constructor, 36-byte formatter, parser, timestamp extractor, and comparator in pure CP

## 安装与导入

```bash
cap add uuidv7
```

```cp
import { fromParts } from "uuidv7";
```

## 核心导出 API

- `export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): UuidV7;`
- `export fn timestampMs(u: UuidV7): u64;`
- `export fn version(u: UuidV7): u8;`
- `export fn isRfcVariant(u: UuidV7): bool;`
- `export fn formatHyphenated(u: UuidV7, out: []u8): usize;`
- `export fn parse(s: string, out: &mut UuidV7): bool;`
- `export fn isValidUuidV7(s: string): bool;`
- `export fn compare(a: UuidV7, b: UuidV7): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`uuidv7@1.0.0`)，许可证：Apache-2.0
- **上游生态**：`crates.io` (`uuid7@1.0.0`)，许可证：Apache-2.0
- **上游生态**：`crates.io` (`uuid@1.0.0`)，许可证：MIT
