# rrule — AI Agent Quick Ref

Zero-allocation RFC 5545 iCalendar RRULE parser, validator & formatter (npm rrule v2 parity) composing unicase for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RRule,
    rrlInvalid,
    rrlParsePosU32,
    rrlParseFreq,
    rrlParseDayCode,
    rrlParseByDayItem,
    rrlParse2Digits,
    rrlIsValidUntil,
} from "rrule";
```

## Core Types & Constants

```cp
class RRule { valid: bool, freq: u8, interval: u32, count: u32, /* +5 fields */ }
```

## API Signatures

```cp
fn rrlInvalid(): RRule;
fn rrlParsePosU32(s: string): u32;
fn rrlParseFreq(v: string): u8;
fn rrlParseDayCode(code: string): u8;
fn rrlParseByDayItem(item: string): u8;
fn rrlParse2Digits(s: string, pos: usize): u32;
fn rrlIsValidUntil(v: string): bool;
fn parseRRule(input: string): RRule;
fn rrlFreqName(freq: u8): string;
fn rrlDayName(mask: u8): string;
fn formatRRule(r: RRule, out: []u8): usize;
fn matchesWeekday(r: RRule, dow: u8): bool;
fn isValidRRule(input: string): bool;
```

## Minimal Usage

```cp
import { parseRRule } from "rrule";

pub fn main() {
    let result = parseRRule("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rrule"`; install via `cap add rrule`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
