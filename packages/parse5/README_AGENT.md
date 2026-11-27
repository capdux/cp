# parse5@1.0.0 (Agent Quickref)

> Zero-heap WHATWG HTML5 streaming tokenizer, attribute extractor, and text stripper (clean-room port of npm parse5) | License: MIT
> Import: `import { ... } from "parse5";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn p5IsWs(b: u8): bool;`
- `export fn p5IsTagNameChar(b: u8): bool;`
- `export fn p5Trim(s: string): string;`
- `export fn p5FindSubstr(s: string, start_pos: usize, needle: string): usize;`
- `export fn isVoidElement(tag: string): bool;`
- `export fn parseAttributes(raw_attrs: string, out_attrs: []HtmlAttr): usize;`
- `export fn extractTextContent(html: string, out: []u8): usize;`
