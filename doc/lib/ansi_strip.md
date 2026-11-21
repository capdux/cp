# `cap:ansi_strip` — ANSI / VT100 Escape Sequence Stripper

Zero-allocation ANSI/VT100 escape sequence stripper supporting CSI (`ESC [`), OSC hyperlinks (`ESC ] ... BEL` / `ESC \`), DCS/APC/PM/SOS, charset designations, 2-byte ESC sequences, and 8-bit C1 CSI (`0x9B`).

## Exports

- `hasAnsi(src: []const u8): bool` — Fast scan for ANSI escape initiators.
- `stripAnsi(src: []const u8, out: []u8): ([]const u8)?` — Writes stripped text into `out`.
- `strippedByteLen(src: []const u8): usize` — Exact byte length after stripping.
- `visibleCodepointCount(src: []const u8): usize` — Visible UTF-8 codepoint count ignoring ANSI sequences.
