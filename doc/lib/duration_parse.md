# `cap:duration_parse` — 人类可读时长字符串解析与格式化库

> 来源：吸收自 `/data/dev/bun/src/cli.zig`（Human Duration Parser & Formatter）

`cap:duration_parse` 提供纯 CP、零外部依赖、零堆分配的人类可读时长解析与规范格式化能力，全程采用 `u64`/`u128` 纯整数算术，彻底消除浮点舍入误差与溢出风险。

## 核心常量与函数

```cp
pub const NS_PER_US: u64 = 1000;
pub const NS_PER_MS: u64 = 1000000;
pub const NS_PER_SEC: u64 = 1000000000;
pub const NS_PER_MIN: u64 = 60000000000;
pub const NS_PER_HOUR: u64 = 3600000000000;
pub const NS_PER_DAY: u64 = 86400000000000;
pub const NS_PER_WEEK: u64 = 604800000000000;

pub fn parseDurationNs(input: []const u8): u64?
pub fn parseDurationMs(input: []const u8): u64?
pub fn formatDurationNs(ns: u64, out: []u8): ([]const u8)?
pub fn formatDurationCompact(ns: u64, out: []u8): ([]const u8)?
```

- `parseDurationNs`：支持 `ns`、`us`、`µs`、`μs`、`ms`、`s`、`m`、`h`、`d`、`w`，支持复合片段（如 `"1h30m15s"`、`"2d 4h"`）、小数（如 `"1.5s"`）与 `"0"`；拒绝负数、未知单位、缺失单位与 `u64` 溢出。
- `parseDurationMs`：解析为毫秒 `u64`（不足 1ms 向下取整）。
- `formatDurationNs`：精确无损的规范复合格式（如 `0` → `"0s"`、`1_500_000_000` → `"1s500ms"`、`5_400_000_000_000` → `"1h30m"`），可与 `parseDurationNs` 精确往返。
- `formatDurationCompact`：单主单位紧凑输出（如 `"1.50s"`、`"250ms"`、`"42us"`）。

## 使用示例

```cp
import { parseDurationNs, parseDurationMs, formatDurationNs, formatDurationCompact } from "cap:duration_parse";

const ns = parseDurationNs("1h30m15s").?;
const ms = parseDurationMs("1.5s").?; // 1500
let buf: [64]u8 = [0; 64];
const full = formatDurationNs(1500000000, buf[0..]).?;    // "1s500ms"
const compact = formatDurationCompact(1500000000, buf[0..]).?; // "1.50s"
```
