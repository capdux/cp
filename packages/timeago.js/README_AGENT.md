# timeago.js@1.0.0 (Agent Quickref)

> Zero-allocation deterministic relative time formatter (EN/ZH, Long/Short/Compact) for CP | License: MIT
> Import: `import { ... } from "timeago.js";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum TimeagoLocale`
- `export enum TimeagoStyle`
- `export enum TimeUnit`

## Functions
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
