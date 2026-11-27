# saxes@1.0.0 (Agent Quickref)

> Zero-allocation streaming SAX XML 1.0 event parser with tag stack validation and entity decoding (npm saxes parity) for CP | License: ISC
> Import: `import { ... } from "saxes";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum XmlEventKind`

## Functions
- `export fn saxEmptyAttrs(): [8]XmlAttr;`
- `export fn saxMakeEvent(kind: XmlEventKind, name: string, text: string): XmlEvent;`
- `export fn saxIsWs(ch: u8): bool;`
- `export fn saxIsNameStart(ch: u8): bool;`
- `export fn saxIsNameChar(ch: u8): bool;`
- `export fn saxStrEq(a: string, b: string): bool;`
- `export fn saxStartsWithAt(s: string, pos: usize, prefix: string): bool;`
- `export fn saxFindSubstrFrom(s: string, start: usize, needle: string): i64;`
- `export fn saxAllWs(s: string): bool;`
- `export fn saxEmit(out: []XmlEvent, count: usize, ev: XmlEvent): usize;`
- `export fn saxEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn saxHexVal(ch: u8): i32;`
- `export fn saxDecodeEntitiesImpl(raw: string, out: []u8): usize;`
