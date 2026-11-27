# calver@1.0.0 (Agent Quickref)

> Zero-allocation Calendar Versioning (CalVer) parser, validator, comparator, bumper, and formatter for CP | License: MIT
> Import: `import { ... } from "calver";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CalverFormat`

## Functions
- `export fn isWs(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isAlpha(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn cmpIgnoreCase(a: string, b: string): i32;`
- `export fn trimWs(s: string): string;`
- `export fn isLeapYear(year: u16): bool;`
- `export fn daysInMonth(year: u16, month: u8): u8;`
- `export fn parseU32Strict(s: string): u32?;`
- `export fn parseModifierTail(s: string): ModifierParts;`
- `export fn parseCalverCore(raw: string, force_day_on_3: bool): Calver;`
- `export fn parseCalver(raw: string): Calver;`
- `export fn parseCalverWith(raw: string, fmt: CalverFormat): Calver;`
- `export fn modifierRank(name: string): u8;`
- `export fn compareCalver(a: Calver, b: Calver): i32;`
- `export fn bumpCalver(cur: Calver, year: u16, month: u8, day: u8, use_day: bool): Calver;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendU32(out: []u8, pos: usize, val: u32): usize;`
- `export fn append2Digits(out: []u8, pos: usize, val: u32): usize;`
- `export fn formatCalver(v: Calver, fmt: CalverFormat, out: []u8): string;`
