# ical-generator@1.0.0 (Agent Quickref)

> Zero-allocation RFC 5545 iCalendar (.ics) VCALENDAR/VEVENT generator and parser in pure CP | License: MIT
> Import: `import { ... } from "ical-generator";` | Full types: `src/index.d.cp`

## Functions
- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushEscapedText(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushPropRaw(dst: []u8, pos: usize, prop: &[]u8, val: &[]u8): i64;`
- `export fn pushPropText(dst: []u8, pos: usize, prop: &[]u8, val: &[]u8): i64;`
- `export fn emptyParsedEvent(): ParsedEvent;`
- `export fn parseFirstEvent(ics: &[]u8): ParsedEvent;`
- `export fn unescapeText(src: &[]u8, dst: []u8): i64;`
