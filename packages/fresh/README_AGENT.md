# fresh@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9110 HTTP conditional request freshness testing (ETag, If-None-Match, If-Modified-Since, Cache-Control) in pure CP | License: MIT
> Import: `import { ... } from "fresh";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSpace(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn trimRange(s: &[]u8, start: usize, end: usize): TrimRange;`
- `export fn eqIgnoreCase(s: &[]u8, start: usize, end: usize, target_lower: &[]u8): bool;`
- `export fn hasNoCache(cache_control: &[]u8): bool;`
- `export fn parseEtagSlice(s: &[]u8, raw_start: usize, raw_end: usize): ParsedEtag;`
- `export fn slicesEqual(a: &[]u8, a_start: usize, a_end: usize, b: &[]u8, b_start: usize, b_end: usize): bool;`
- `export fn etagWeakMatch(a: &[]u8, b: &[]u8): bool;`
- `export fn etagStrongMatch(a: &[]u8, b: &[]u8): bool;`
- `export fn parse2Digits(s: &[]u8, pos: usize): i32;`
- `export fn parse4Digits(s: &[]u8, pos: usize): i32;`
- `export fn parseMonth(s: &[]u8, pos: usize): i32;`
- `export fn isValidWeekday(s: &[]u8, pos: usize): bool;`
- `export fn isLeapYear(y: u64): bool;`
- `export fn daysInMonth(month: i32, leap: bool): i32;`
- `export fn parseHttpDate(date_str: &[]u8): i64;`
- `export fn matchIfNoneMatch(inm: &[]u8, res_etag: &[]u8): bool;`
- `export fn isFresh(req: RequestHeaders, res: ResponseHeaders): bool;`
