[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# split2

Zero-allocation streaming line splitter across arbitrary chunk boundaries (LF `\n` & CRLF `\r\n`) and stateless zero-copy line splitter for CP (npm `split2` parity).

Handles lines fragmented across arbitrary TCP/file read chunks — including `\r` at the end of chunk N and `\n` at the start of chunk N+1 — using a fixed 4096-byte stack buffer with configurable `max_line_len` overflow protection.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `split2` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/mcollina/split2` |
| License | ISC |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: split2)
import split2 from "split2";
stream.pipe(split2()).on("data", (line) => handleLine(line));
```

```cp
// CP (cap: split2) — zero-allocation streaming & stateless line splitting
import { LineSplitter, splitLines } from "split2";
let sp = LineSplitter.new();
const n = sp.pushChunk(&chunk, outBuf[0..4096], starts[0..64], lens[0..64]);
```

## Installation

```bash
cap add split2
```

## Usage

### Basic Usage: Streaming Across Chunk Boundaries (Split CRLF Handled Automatically)

```cp
import { LineSplitter } from "split2";

let sp = LineSplitter.new();
let outBuf: [1024]u8 = undefined;
let starts: [16]usize = undefined;
let lens: [16]usize = undefined;

// Chunk 1 ends mid-line with '\r' (CRLF split across chunks!)
const c1: []const u8 = "hello\nworld\r";
const n1 = sp.pushChunk(&c1, outBuf[0..1024], starts[0..16], lens[0..16]);
// n1 == 1 -> line 0 is outBuf[starts[0] .. starts[0] + lens[0]] == "hello"

// Chunk 2 starts with '\n' and ends with an unterminated trailing line
const c2: []const u8 = "\nlast-line";
const n2 = sp.pushChunk(&c2, outBuf[0..1024], starts[0..16], lens[0..16]);
// n2 == 1 -> line 0 is "world" (trailing '\r' stripped!)

// Flush remaining partial line at EOF
let tail: [256]u8 = undefined;
const tailLen = sp.flush(tail[0..256]);
// tail[0..tailLen] == "last-line"
```

### Advanced & Security Usage: Stateless Zero-Copy Split, Skip-Empty & Overflow Protection

```cp
import { LineSplitter, splitLines } from "split2";

// 1. Stateless zero-copy split into string slices (supports both LF and CRLF)
let slices: [8]string = undefined;
const count = splitLines("a\r\nb\nc", slices[0..8]);
// count == 3 -> slices[0] == "a", slices[1] == "b", slices[2] == "c"

// 2. Hard line-length cap + skip_empty to guard against unbounded network lines
let guarded = LineSplitter.withMaxLen(128);
guarded.skip_empty = true;
// If an incoming line exceeds 128 bytes, excess bytes are safely truncated and:
// guarded.overflowed() == true (call guarded.reset() to clear state & overflow flag)
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `LineSplitter.new` | `static fn(): LineSplitter` | Creates a streaming splitter with default 4096-byte line cap (`SPLIT2_BUF_CAP`). |
| `LineSplitter.withMaxLen` | `static fn(max_len: usize): LineSplitter` | Creates a splitter clamped to `min(max_len, 4096)` bytes per line. |
| `LineSplitter.pushChunk` | `fn(*_, chunk: &[]u8, out_lines_buf: []u8, out_starts: []usize, out_lens: []usize): usize` | Feeds a chunk; packs completed lines into `out_lines_buf` and returns line count. |
| `LineSplitter.flush` | `fn(*_, out_line: []u8): usize` | Emits any pending unterminated bytes at EOF; returns bytes written. |
| `LineSplitter.pendingLen` | `fn(): usize` | Returns current buffered partial-line byte length. |
| `LineSplitter.overflowed` | `fn(): bool` | Returns `true` if any line exceeded `max_line_len`. |
| `LineSplitter.reset` | `fn(*_)` | Clears pending buffer and resets `overflowed` flag. |
| `splitLines` | `fn(input: string, out_slices: []string): usize` | Zero-copy stateless splitter writing line slices into `out_slices`. |

## Security & Limits

- **Bounded memory**: Fixed 4096-byte internal buffer prevents OOM on maliciously long lines without `\n`.
- **Zero heap allocation**: Both `LineSplitter` and `splitLines` operate purely on stack/caller buffers.

## License

ISC — Clean-room CP implementation inspired by npm `split2`.
