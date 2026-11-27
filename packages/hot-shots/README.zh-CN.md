[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hot-shots

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap StatsD and DogStatsD UDP metric packet encoder/decoder supporting Counter, Timing, Gauge, Set, sample rates, and tags.

## 安装与导入

```bash
cap add hot-shots
```

```cp
import { isValidMetricName } from "hot-shots";
```

## 核心导出 API

- `export fn isValidMetricName(name: &[]u8): bool;`
- `export fn isValidTags(tags: &[]u8): bool;`
- `export fn appendByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;`
- `export fn appendI64(dst: []u8, pos: usize, v: i64): i64;`
- `export fn formatMetric(name: &[]u8, value: i64, kind: u8, sample_rate_milli: u16, tags: &[]u8, dst: []u8): i64;`
- `export fn parseSampleRateMilli(s: &[]u8): i32;`
- `export fn parseMetric(packet: &[]u8): StatsdMetric;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`hot-shots@1.0.0`)，许可证：MIT
