# ajv-formats — AI Agent Quick Ref

Zero-allocation JSON Schema Draft 2019-09/2020-12 string format validators: date, time, date-time, email, hostname, ipv4, ipv6, uuid, uri, json-pointer (clean-room MIT implementation of npm ajv-formats, #205 / 212M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ajvfIsDigit,
    ajvfIsAlpha,
    ajvfIsAlphaNum,
    ajvfIsHex,
    ajvfParse2Digits,
    ajvfParse4Digits,
    ajvfIsLeapYear,
    ajvfDaysInMonth,
} from "ajv-formats";
```

## API Signatures

```cp
fn ajvfIsDigit(b: u8): bool;
fn ajvfIsAlpha(b: u8): bool;
fn ajvfIsAlphaNum(b: u8): bool;
fn ajvfIsHex(b: u8): bool;
fn ajvfParse2Digits(s: string, offset: usize): i32;
fn ajvfParse4Digits(s: string, offset: usize): i32;
fn ajvfIsLeapYear(year: u32): bool;
fn ajvfDaysInMonth(year: u32, month: u32): u32;
fn isDate(s: string): bool;
fn isTime(s: string): bool;
fn isDateTime(s: string): bool;
fn isHostname(s: string): bool;
fn ajvfIsEmailLocalChar(b: u8): bool;
fn isEmail(s: string): bool;
fn isIpv4(s: string): bool;
fn isIpv6(s: string): bool;
fn isUuid(s: string): bool;
fn ajvfIsSchemeChar(b: u8): bool;
fn isUri(s: string): bool;
fn isJsonPointer(s: string): bool;
fn ajvfStrEq(a: string, b: string): bool;
fn validateFormat(format_name: string, value: string): bool;
```

## Minimal Usage

```cp
import { isDate } from "ajv-formats";

pub fn main() {
    let result = isDate("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ajv-formats"`; install via `cap add ajv-formats`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
