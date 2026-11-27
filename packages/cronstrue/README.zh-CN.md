[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cronstrue

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Cron expression to human-readable English & Chinese natural language translator for CP

## 安装与导入

```bash
cap add cronstrue
```

```cp
import { toUpperAscii } from "cronstrue";
```

## 核心导出 API

- `export enum Locale`
- `export fn toUpperAscii(c: u8): u8;`
- `export fn isSpace(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn pushU8(out: []u8, pos: usize, val: u8): usize;`
- `export fn pushU8Pad2(out: []u8, pos: usize, val: u8): usize;`
- `export fn pushTime(out: []u8, pos: usize, hour: u8, minute: u8, opts: CronDescOptions): usize;`
- `export fn dayName(val: u8, locale: Locale): string;`
- `export fn monthName(val: u8, locale: Locale): string;`
- `export fn parseAtom(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): i32;`
- `export fn parseFieldItem(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): ItemParseRes;`
- `export fn parseField(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): FieldParseRes;`
- `export fn expandMacro(trimmed: string): string;`
- `export fn parseCron(raw: string): ParsedCron;`
- `export fn writeItemNumeric(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize;`
- `export fn writeItemDay(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize;`
- `export fn writeItemMonth(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize;`
- `export fn isValidCron(expr: string): bool;`
- `export fn describeCronWith(expr: string, opts: CronDescOptions, out: []u8): string;`
- `export fn describeCron(expr: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cronstrue@1.0.0`)，许可证：MIT
