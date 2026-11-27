[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ical-generator

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 5545 iCalendar (.ics) VCALENDAR/VEVENT generator and parser in pure CP

## 安装与导入

```bash
cap add ical-generator
```

```cp
import { pushSlice } from "ical-generator";
```

## 核心导出 API

- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushEscapedText(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushPropRaw(dst: []u8, pos: usize, prop: &[]u8, val: &[]u8): i64;`
- `export fn pushPropText(dst: []u8, pos: usize, prop: &[]u8, val: &[]u8): i64;`
- `export fn toUpperAscii(b: u8): u8;`
- `export fn eqIgnoreCaseUpper(src: &[]u8, start: usize, end: usize, upper_target: &[]u8): bool;`
- `export fn emptyParsedEvent(): ParsedEvent;`
- `export fn parseFirstEvent(ics: &[]u8): ParsedEvent;`
- `export fn unescapeText(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ical-generator@1.0.0`)，许可证：MIT
