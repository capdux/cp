# marked@1.0.0 (Agent Quickref)

> Zero-heap CommonMark/GFM Markdown to HTML compiler and plain-text stripper in pure CP with XSS-safe HTML escaping via escape-html (clean-room MIT counterpart to npm marked v14). | License: MIT
> Import: `import { ... } from "marked";` | Full types: `src/index.d.cp`
> Deps: `escape-html`

## Functions
- `export fn defaultMarkedOptions(): MarkedOptions;`
- `export fn mkdWriteByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn mkdWriteStr(out: []u8, pos: &mut usize, s: string);`
- `export fn mkdWriteEscaped(out: []u8, pos: &mut usize, s: string);`
- `export fn mkdWriteEscapedByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn mkdIsSpace(c: u8): bool;`
- `export fn mkdIsPunctEscape(c: u8): bool;`
- `export fn mkdStartsWithIgnoreCaseClean(url: string, prefix: string): bool;`
- `export fn mkdIsSafeUrl(url: string): bool;`
- `export fn mkdFindLineEnd(s: string, start: usize): usize;`
- `export fn mkdTrimCr(s: string, start: usize, end: usize): usize;`
- `export fn mkdSkipIndent(s: string, start: usize, end: usize, max_spaces: usize): usize;`
- `export fn mkdIsBlankLine(s: string, start: usize, end: usize): bool;`
- `export fn mkdMatchHeading(s: string, line_start: usize, line_end: usize, content_start: &mut usize, content_end: &mut usize): usize;`
- `export fn mkdIsHorizontalRule(s: string, line_start: usize, line_end: usize): bool;`
- `export fn mkdMatchFenceOpen(s: string, line_start: usize, line_end: usize, fence_char: &mut u8, lang_start: &mut usize, lang_end: &mut usize): usize;`
- `export fn mkdIsFenceClose(s: string, line_start: usize, line_end: usize, fence_char: u8, min_len: usize): bool;`
- `export fn mkdIsBlockquoteLine(s: string, line_start: usize, line_end: usize): bool;`
- `export fn mkdMatchUnorderedItem(s: string, line_start: usize, line_end: usize): usize;`
- `export fn mkdMatchOrderedItem(s: string, line_start: usize, line_end: usize, num_out: &mut usize): usize;`
- `export fn mkdStartsNewBlock(s: string, line_start: usize, line_end: usize): bool;`
- `export fn mkdFindClosingDouble(s: string, start: usize, ch: u8): i64;`
- `export fn mkdFindClosingSingle(s: string, start: usize, ch: u8): i64;`
- `export fn mkdMatchLinkOrImage(s: string, open_bracket: usize, label_start: &mut usize, label_end: &mut usize, url_start: &mut usize, url_end: &mut usize): usize;`
- *(+9 more in `src/index.d.cp`)*
