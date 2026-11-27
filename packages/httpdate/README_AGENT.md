# httpdate — AI Agent Quick Ref

Zero-allocation RFC 9110 Section 5.6.7 HTTP-date parser (IMF-fixdate, RFC 850, asctime) and formatter with Hinnant calendar math in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HttpDateTime,
    invalidDateTime,
    isOws,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
    buildValidated,
    parse2Digits,
} from "httpdate";
```

## Core Types & Constants

```cp
class HttpDateTime { year: u16, month: u8, day: u8, hour: u8, /* +4 fields */ }
```

## API Signatures

```cp
fn invalidDateTime(): HttpDateTime;
fn isOws(b: u8): bool;
fn isLeapYear(y: u16): bool;
fn daysInMonth(y: u16, m: u8): u8;
fn daysFromCivil(year: u16, month: u8, day: u8): u64;
fn buildValidated(year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32): HttpDateTime;
fn parse2Digits(s: string, pos: usize): i32;
fn parse4Digits(s: string, pos: usize): i32;
fn parseMonth(s: string, pos: usize): i32;
fn isShortWeekday(s: string, pos: usize): bool;
fn isLongWeekday(s: string): bool;
fn parseImfFixdate(s: string): HttpDateTime;
fn parseRfc850(s: string): HttpDateTime;
fn parseAsctime(s: string): HttpDateTime;
fn parseHttpDate(input: string): HttpDateTime;
fn toUnixSeconds(dt: HttpDateTime): i64;
fn fromUnixSeconds(ts: i64): HttpDateTime;
fn weekdayName(wd: u8): string;
fn monthName(m: u8): string;
fn formatImfFixdate(dt: HttpDateTime, out: []u8): usize;
fn formatUnixHttpDate(ts: i64, out: []u8): usize;
fn parseUnixSeconds(input: string): i64;
```

## Minimal Usage

```cp
import { parse2Digits } from "httpdate";

pub fn main() {
    let result = parse2Digits("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"httpdate"`; install via `cap add httpdate`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
