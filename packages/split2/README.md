# @cap/split2

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:split2` ([https://github.com/mcollina/split2](https://github.com/mcollina/split2), `ISC`)

Zero-heap-allocation streaming line splitter across arbitrary chunk boundaries in pure CP, modeled after Matteo Collina's `split2`.

## Features

- `LineSplitter`: fixed-capacity `[4096]u8` partial-line buffer with configurable `max_line_len` and `skip_empty`
  - `LineSplitter.new()` / `LineSplitter.withMaxLen(max_len: usize)`
  - `pushChunk(*_, chunk, out_lines_buf, out_starts, out_lens): usize` — splits `\n` and `\r\n` seamlessly across chunk boundaries (including `\r` and `\n` split across consecutive chunks)
  - `flush(*_, out_line): usize` — emits any remaining unterminated trailing line at EOF
  - `pendingLen(): usize`, `overflowed(): bool`, `reset(*_)`
- `splitLines(input: string, out_slices: []string): usize` — zero-copy stateless `\n` / `\r\n` line splitter

## License

ISC
