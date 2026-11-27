# rss@1.0.0 (Agent Quickref)

> Zero-allocation RSS 2.0 XML feed generator and CDATA-aware tag extractor in pure CP | License: MIT
> Import: `import { ... } from "rss";` | Full types: `src/index.d.cp`

## Functions
- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushXmlEscaped(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushTag(dst: []u8, pos: usize, tag: &[]u8, val: &[]u8): i64;`
- `export fn extractTagValue(xml: &[]u8, tag: &[]u8, start_pos: usize): &[]u8;`
- `export fn countItems(xml: &[]u8): usize;`
