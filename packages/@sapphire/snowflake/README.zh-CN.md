[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @sapphire/snowflake

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 64-bit Twitter/Discord Snowflake distributed ID generator, decomposer, clock-rollback guard, and decimal formatter.

## 安装与导入

```bash
cap add @sapphire/snowflake
```

```cp
import { compose } from "@sapphire/snowflake";
```

## 核心导出 API

- `export fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16): u64;`
- `export fn decompose(id: u64, epoch_ms: u64): SnowflakeParts;`
- `export fn initGenerator(epoch_ms: u64, datacenter_id: u8, worker_id: u8): SnowflakeGenerator;`
- `export fn nextId(gen: &mut SnowflakeGenerator, now_ms: u64): i64;`
- `export fn formatId(id: u64, out: []u8): usize;`
- `export fn parseId(s: string): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@sapphire/snowflake@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`sonyflake@1.0.0`)，许可证：MIT
