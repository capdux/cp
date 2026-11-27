# github-slugger@1.0.0 (Agent Quickref)

> Zero-allocation Markdown heading outline extractor, GitHub-compatible anchor slugger & automatic nested TOC generator (github-slugger / markdown-toc parity) for CP | License: MIT
> Import: `import { ... } from "github-slugger";` | Full types: `src/index.d.cp`

## Functions
- `export fn githubSlug(heading: string, out: []u8): string;`
- `export fn extractToc(markdown: string): TocResult;`
- `export fn renderTocMarkdown(toc: TocResult, min_level: u8, max_level: u8, out: []u8): string;`
