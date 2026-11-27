[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# httpdate

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110 Section 5.6.7 HTTP-date parser (IMF-fixdate, RFC 850, asctime) and formatter with Hinnant calendar math in pure CP

## 安装与导入

```bash
cap add httpdate
```

```cp
import { invalidDateTime } from "httpdate";
```

## 核心导出 API

- `export fn invalidDateTime(): HttpDateTime;`
- `export fn isSpace(b: u8): bool;`
- `export fn isLeapYear(y: u16): bool;`
- `export fn daysInMonth(y: u16, m: u8): u8;`
- `export fn daysFromCivil(year: u16, month: u8, day: u8): u64;`
- `export fn buildValidated(year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32): HttpDateTime;`
- `export fn parse2Digits(s: string, pos: usize): i32;`
- `export fn parse4Digits(s: string, pos: usize): i32;`
- `export fn parseMonth(s: string, pos: usize): i32;`
- `export fn isShortWeekday(s: string, pos: usize): bool;`
- `export fn sliceEq(a: string, b: string): bool;`
- `export fn isLongWeekday(s: string): bool;`
- `export fn parseImfFixdate(s: string): HttpDateTime;`
- `export fn parseRfc850(s: string): HttpDateTime;`
- `export fn parseAsctime(s: string): HttpDateTime;`
- `export fn parseHttpDate(input: string): HttpDateTime;`
- `export fn toUnixSeconds(dt: HttpDateTime): i64;`
- `export fn fromUnixSeconds(ts: i64): HttpDateTime;`
- `export fn weekdayName(wd: u8): string;`
- `export fn monthName(m: u8): string;`
- `export fn formatImfFixdate(dt: HttpDateTime, out: []u8): usize;`
- `export fn formatUnixHttpDate(ts: i64, out: []u8): usize;`
- `export fn parseUnixSeconds(input: string): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`httpdate@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`http-date@1.0.0`)，许可证：MIT
