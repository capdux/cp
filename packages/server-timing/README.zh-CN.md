[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# server-timing

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation W3C Server-Timing HTTP response header parser and formatter in pure CP

## 安装与导入

```bash
cap add server-timing
```

```cp
import { isSpace } from "server-timing";
```

## 核心导出 API

- `export fn isSpace(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn eqIgnoreCase(a: string, lower_b: string): bool;`
- `export fn trimSlice(s: string): string;`
- `export fn parseFloatSimple(s: string): ParsedFloat;`
- `export fn parseSingleMetric(seg: string, out: &mut TimingMetric): bool;`
- `export fn parseHeader(header: string, out: []TimingMetric): usize;`
- `export fn findMetric(header: string, name: string, out: &mut TimingMetric): bool;`
- `export fn totalDurationMs(header: string): f64;`
- `export fn isTokenChar(b: u8): bool;`
- `export fn descNeedsQuotes(s: string): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushU64(dst: []u8, pos: usize, val: u64): i64;`
- `export fn pushDurMs(dst: []u8, pos: usize, dur_ms: f64): i64;`
- `export fn formatMetric(metric: TimingMetric, out: []u8): usize;`
- `export fn formatHeader(metrics: &[]TimingMetric, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`server-timing@1.0.0`)，许可证：MIT
