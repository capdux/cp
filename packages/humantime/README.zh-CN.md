[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# humantime

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap human-readable duration parser/formatter and RFC 3339 timestamp codec (clean-room CP implementation of crates.io humantime)

## 安装与导入

```bash
cap add humantime
```

```cp
import { htIsSpace } from "humantime";
```

## 核心导出 API

- `export fn htIsSpace(c: u8): bool;`
- `export fn htIsDigit(c: u8): bool;`
- `export fn htIsAlpha(c: u8): bool;`
- `export fn htEq(s: string, start: usize, end: usize, lit: string): bool;`
- `export fn htUnitNanos(s: string, start: usize, end: usize): u64;`
- `export fn parseDuration(input: string): DurationParseResult;`
- `export fn htPushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn htPushPart(out: []u8, pos: usize, val: u64, suffix: string): usize;`
- `export fn formatDuration(nanos: u64, out: []u8): usize;`
- `export fn htPush2Digits(out: []u8, pos: usize, v: u64): usize;`
- `export fn htPush4Digits(out: []u8, pos: usize, v: u64): usize;`
- `export fn formatRfc3339Seconds(unix_secs: u64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`humantime@1.0.0`)，许可证：MIT
