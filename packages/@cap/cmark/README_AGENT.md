# @cap/cmark@0.31.1 (Agent Quickref)

> CommonMark 0.31.1 official C reference Markdown to HTML/XML/LaTeX renderer for CP | License: BSD-2-Clause
> Import: `import { ... } from "@cap/cmark";` | Full types: `src/index.d.cp`

## Functions
- `export fn cmark_markdown_to_html(text: [*]&u8!, len: usize, options: i32): [*:0]u8!?;`
- `export fn cmark_version(): i32;`
- `export fn cmark_version_string(): [*:0]&u8!?;`
- `export fn free(ptr: *auto!?);`
- `export fn versionString(): string;`
- `export fn versionNumber(): i32;`
- `export fn markdownToHtml(md: string, opts: i32): !HtmlString;`
