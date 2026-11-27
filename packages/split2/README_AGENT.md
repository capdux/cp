# split2 — AI Agent Quick Reference

- **Package**: `split2` (`v1.0.0`) | **Source**: npm `split2` | **Import**: `import { ... } from "split2";`
- **Role**: Zero-heap streaming LF/CRLF line splitter (`LineSplitter`, 4096B cap) + stateless zero-copy `splitLines`.

## Core Exports (`src/index.d.cp`)

```cp
class LineSplitter {
    buf: [4096]u8,
    len: usize,
    max_line_len: usize,
    skip_empty: bool,
    is_overflowed: bool,
    fn new(): LineSplitter;
    fn withMaxLen(max_len: usize): LineSplitter;
    fn pushChunk(_: auto, chunk: &[]u8, out_lines_buf: []u8, out_starts: []usize, out_lens: []usize): usize;
    fn flush(_: auto, out_line: []u8): usize;
    fn pendingLen(): usize;
    fn overflowed(): bool;
    fn reset(_: auto);
}

fn splitLines(input: string, out_slices: []string): usize;
```

## Minimal Usage

```cp
import { LineSplitter, splitLines } from "split2";

// Streaming across arbitrary chunks
let sp = LineSplitter.new();
let out: [1024]u8 = undefined;
let starts: [16]usize = undefined;
let lens: [16]usize = undefined;
const n = sp.pushChunk(&"a\r\nb\nc", out[0..1024], starts[0..16], lens[0..16]); // n == 2 ("a", "b")
let tail: [64]u8 = undefined;
const t = sp.flush(tail[0..64]); // t == 1 ("c")

// Stateless zero-copy
let lines: [8]string = undefined;
const count = splitLines("a\r\nb\nc", lines[0..8]); // 3
```

## Pitfalls & Constraints

1. **Always call `flush()` at EOF**: `pushChunk` only emits newline-terminated lines; the final unterminated line stays buffered until `flush(out_line)` is called.
2. **Buffer capacity**: `LineSplitter` caps line length at `min(max_line_len, 4096)`. Excess bytes are truncated and `overflowed()` becomes `true` until `reset()`.
3. **Output buffer sizing in `pushChunk`**: Lines exceeding `out_starts.len`, `out_lens.len`, or remaining space in `out_lines_buf` are dropped for that call — size `out_lines_buf >= chunk.len + sp.pendingLen()`.
4. **Empty lines**: Set `sp.skip_empty = true` to omit blank lines; `splitLines` preserves empty lines and strips trailing `\r` from `\r\n`.
