[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# chrome-trace-event

> 版本：`1.0.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Chrome DevTools / Perfetto Trace Event JSON serializer

## 安装与导入

```bash
cap add chrome-trace-event
```

```cp
import { phaseCode } from "chrome-trace-event";
```

## 核心导出 API

- `export enum TracePhase`
- `export fn phaseCode(ph: TracePhase): u8;`
- `export fn ctevHexDigit(n: u8): u8;`
- `export fn ctevWriteByte(out: []u8, pos: i64, b: u8): i64;`
- `export fn ctevWriteStr(out: []u8, pos: i64, s: string): i64;`
- `export fn ctevWriteJsonStr(out: []u8, pos: i64, s: string): i64;`
- `export fn ctevWriteU64(out: []u8, pos: i64, val: u64): i64;`
- `export fn formatTraceEvent(ev: TraceEvent, out: []u8): usize;`
- `export fn formatTraceArray(events: &[]TraceEvent, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`chrome-trace-event@1.0.4`)（https://github.com/samccone/chrome-trace-event），许可证：MIT
