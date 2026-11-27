# cronstrue@1.0.0 (Agent Quickref)

> Zero-allocation Cron expression to human-readable English & Chinese natural language translator for CP | License: MIT
> Import: `import { ... } from "cronstrue";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Locale`

## Functions
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
