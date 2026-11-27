# jsesc@1.0.0 (Agent Quickref)

> Zero-allocation JavaScript & JSON string escaper with ES6 code point, script-context, and minimal modes (clean-room MIT implementation of npm jsesc v3, #148 / 250M wk) | License: MIT
> Import: `import { ... } from "jsesc";` | Full types: `src/index.d.cp`

## Functions
- `export fn jescHexChar(nibble: u32, lowercase: bool): u8;`
- `export fn jescPush(out: []u8, pos: usize, b: u8): usize;`
- `export fn jescPush2(out: []u8, pos: usize, b0: u8, b1: u8): usize;`
- `export fn jescPushHex2(out: []u8, pos: usize, val: u32, lowercase: bool): usize;`
- `export fn jescPushUnicode4(out: []u8, pos: usize, val: u32, lowercase: bool): usize;`
- `export fn jescPushEs6CodePoint(out: []u8, pos: usize, cp: u32, lowercase: bool): usize;`
- `export fn jescDecodeUtf8(input: string, idx: usize): JescDecoded;`
- `export fn jescAsciiLower(b: u8): u8;`
- `export fn jescMatchesHtmlComment(input: string, idx: usize): bool;`
- `export fn jescMatchesCloseScript(input: string, idx: usize): bool;`
- `export fn jescEscapeCodePoint(out: []u8, pos: usize, cp: u32, is_json: bool, use_es6: bool, lowercase: bool): usize;`
- `export fn jsescWithOptions(input: string, opts: JsescOptions, out: []u8): usize;`
- `export fn jsesc(input: string, out: []u8): usize;`
