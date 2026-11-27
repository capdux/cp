[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# timeago.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deterministic relative time formatter (EN/ZH, Long/Short/Compact) for CP

## 安装与导入

```bash
cap add timeago.js
```

```cp
import { absU64 } from "timeago.js";
```

## 核心导出 API

- `export enum TimeagoLocale`
- `export enum TimeagoStyle`
- `export enum TimeUnit`
- `export fn absU64(n: i64): u64;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn scaleDelta(abs_sec: u64): ScaledDelta;`
- `export fn enLongUnit(unit: TimeUnit, plural: bool): string;`
- `export fn enShortUnit(unit: TimeUnit): string;`
- `export fn zhLongUnit(unit: TimeUnit): string;`
- `export fn zhShortUnit(unit: TimeUnit): string;`
- `export fn formatTimeago(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string;`
- `export fn formatBetween(target_sec: i64, now_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string;`
- `export fn formatTimeagoEn(delta_sec: i64, out: []u8): string;`
- `export fn formatTimeagoZh(delta_sec: i64, out: []u8): string;`
- `export fn formatCompact(delta_sec: i64, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`timeago.js@1.0.0`)，许可证：MIT
