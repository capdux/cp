# calver — AI Agent Quick Ref

Zero-allocation Calendar Versioning (CalVer) parser, validator, comparator, bumper, and formatter for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CalverFormat,
    Calver,
    ModifierParts,
    cmpIgnoreCase,
    isLeapYear,
    daysInMonth,
    parseU32Strict,
    parseModifierTail,
} from "calver";
```

## Core Types & Constants

```cp
enum CalverFormat { YyyyMmMicro, YyMmMicro, Yyyy0mMicro, YyyyMmDd, YyyyMmDdMicro }
class Calver { valid: bool, year: u16, month: u8, day: u8, /* +3 fields */ } // methods: invalid(...)
class ModifierParts { ok: bool, name: string, num: u32 }
```

## API Signatures

```cp
fn cmpIgnoreCase(a: string, b: string): i32;
fn isLeapYear(year: u16): bool;
fn daysInMonth(year: u16, month: u8): u8;
fn parseU32Strict(s: string): u32?;
fn parseModifierTail(s: string): ModifierParts;
fn parseCalverCore(raw: string, force_day_on_3: bool): Calver;
fn parseCalver(raw: string): Calver;
fn parseCalverWith(raw: string, fmt: CalverFormat): Calver;
fn modifierRank(name: string): u8;
fn compareCalver(a: Calver, b: Calver): i32;
fn bumpCalver(cur: Calver, year: u16, month: u8, day: u8, use_day: bool): Calver;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn append2Digits(out: []u8, pos: usize, val: u32): usize;
fn formatCalver(v: Calver, fmt: CalverFormat, out: []u8): string;
```

## Minimal Usage

```cp
import { parseU32Strict } from "calver";

pub fn main() {
    let result = parseU32Strict("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"calver"`; install via `cap add calver`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
