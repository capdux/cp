# dompurify@1.0.0 (Agent Quickref)

> Zero-allocation HTML sanitizer stripping blocked tags, event attributes, and unsafe URI schemes (npm DOMPurify parity) in pure CP | License: Apache-2.0
> Import: `import { ... } from "dompurify";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultSanitizeOptions(): SanitizeHtmlOptions;`
- `export fn dmpToLower(c: u8): u8;`
- `export fn dmpIsSpace(c: u8): bool;`
- `export fn dmpIsNameChar(c: u8): bool;`
- `export fn dmpEqCI(a: string, b: string): bool;`
- `export fn dmpHasPrefixCI(s: string, prefix: string): bool;`
- `export fn dmpWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn dmpWriteStr(out: []u8, pos: usize, s: string, lowercase: bool): usize;`
- `export fn dmpIsBlockedContentTag(tag: string): bool;`
- `export fn dmpIsBlockedVoidTag(tag: string): bool;`
- `export fn dmpIsAllowedTag(tag: string, opts: SanitizeHtmlOptions): bool;`
- `export fn dmpIsEventAttr(attr: string): bool;`
- `export fn dmpIsSafeUrl(url: string): bool;`
- `export fn dmpIsSafeAttr(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool;`
- `export fn dmpFindTagClose(html: string, start: usize): usize;`
- `export fn dmpFindClosingTagEnd(html: string, start: usize, tag: string): usize;`
- `export fn sanitizeHtml(html: string, opts: SanitizeHtmlOptions, out: []u8): usize;`
- `export fn isSafeHtml(html: string): bool;`
