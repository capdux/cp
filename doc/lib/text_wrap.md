# `cap:text_wrap` — CLI Word-Wrap, Dedent, Indent & UTF-8 Truncation

Zero-allocation text formatting utilities for CLI help output, diagnostics, and tables.

## Exports

- `wrapText(text, max_width, out): ([]const u8)?` — Greedy word-wrapping preserving `\n` breaks.
- `indentLines(text, prefix, out): ([]const u8)?` — Prefixes every non-empty line.
- `commonIndent(text): usize` / `dedentLines(text, out): ([]const u8)?` — Computes & strips common leading spaces.
- `truncateEllipsis(text, max_len, ellipsis, out): ([]const u8)?` — UTF-8 codepoint-safe truncation with ellipsis.
