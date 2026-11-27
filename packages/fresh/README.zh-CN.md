[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fresh

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110 HTTP conditional request freshness testing (ETag, If-None-Match, If-Modified-Since, Cache-Control) in pure CP

## 安装与导入

```bash
cap add fresh
```

```cp
import { trimRange } from "fresh";
```

## 核心导出 API

- `export fn trimRange(s: &[]u8, start: usize, end: usize): TrimRange;`
- `export fn hasNoCache(cache_control: &[]u8): bool;`
- `export fn parseEtagSlice(s: &[]u8, raw_start: usize, raw_end: usize): ParsedEtag;`
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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fresh@1.0.0`)，许可证：MIT
