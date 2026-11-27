[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ulid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ULID (48-bit Unix ms timestamp + 80-bit entropy, Crockford Base32) encoder, decoder, validator, and monotonic incrementer for CP.

## 安装与导入

```bash
cap add ulid
```

```cp
import { fromParts } from "ulid";
```

## 核心导出 API

- `export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): Ulid;`
- `export fn timestampMs(u: Ulid): u64;`
- `export fn encode(u: Ulid, out: []u8): usize;`
- `export fn decode(s: string, out: &mut Ulid): bool;`
- `export fn isValidUlid(s: string): bool;`
- `export fn increment(u: Ulid, out: &mut Ulid): bool;`
- `export fn compare(a: Ulid, b: Ulid): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ulid@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`ulid@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`ulidx@1.0.0`)，许可证：MIT
