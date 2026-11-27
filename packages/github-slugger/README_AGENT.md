# github-slugger — AI Agent Quick Ref

Zero-allocation Markdown heading outline extractor, GitHub-compatible anchor slugger & automatic nested TOC generator (github-slugger / markdown-toc parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Heading,
    TocResult,
    appendByte,
    githubSlug,
    trimHeadingText,
    isFenceLine,
    slugExists,
    extractToc,
} from "github-slugger";
```

## Core Types & Constants

```cp
class Heading { level: u8, text: string, slug_start: usize, slug_len: usize, /* +1 fields */ }
class TocResult { headings: [MAX_HEADINGS]Heading, count: usize, slug_pool: [MAX_SLUG_POOL]u8, slug_pool_len: usize } // methods: empty(...) +4
```

## API Signatures

```cp
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn githubSlug(heading: string, out: []u8): string;
fn trimHeadingText(raw: string): string;
fn isFenceLine(line: string): bool;
fn slugExists(res: &mut TocResult, cand: string): bool;
fn extractToc(markdown: string): TocResult;
fn renderTocMarkdown(toc: TocResult, min_level: u8, max_level: u8, out: []u8): string;
```

## Minimal Usage

```cp
import { isFenceLine } from "github-slugger";

pub fn main() {
    let result = isFenceLine("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"github-slugger"`; install via `cap add github-slugger`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
