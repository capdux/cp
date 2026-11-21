# `cap:text_indent` — Zero-Alloc Multi-line Text Dedent & Indent Formatter

Zero-allocation pure-CP multi-line text indentation detector, common-indent stripper (`dedent`), line indenter (`indentLines`), and fixed-prefix stripper (`stripCommonPrefix`), preserving both `\n` and `\r\n` line endings.

## Exports

- `minCommonIndent(text: []const u8): usize` — Computes the minimum leading space/tab byte count across all non-blank lines. Returns `0` for empty or whitespace-only inputs.
- `dedent(text: []const u8, out: []u8): ([]const u8)?` — Strips `minCommonIndent(text)` leading bytes from each non-blank line, normalizes whitespace-only lines to empty lines, and preserves `\n` / `\r\n` endings. Returns `null` if `out` is too small.
- `indentLines(text: []const u8, prefix: []const u8, indent_empty: bool, out: []u8): ([]const u8)?` — Prepends `prefix` to each line. When `indent_empty` is `false`, blank/whitespace-only lines are skipped. Returns `null` if `out` is too small.
- `stripCommonPrefix(text: []const u8, prefix: []const u8, out: []u8): ([]const u8)?` — Strips a fixed `prefix` (e.g., `"> "` or `"// "`) from lines starting with it while leaving other lines unchanged. Returns `null` if `out` is too small.

## Example

```cp
import { minCommonIndent, dedent, indentLines, stripCommonPrefix } from "cap:text_indent";

const block = "    fn main() {\n        return;\n    }\n";
const common = minCommonIndent(block); // 4

let buf: [256]u8 = [0; 256];
if (clean := dedent(block, buf[0..])) {
    // "fn main() {\n    return;\n}\n"
}

let quoted: [256]u8 = [0; 256];
if (out := indentLines("a\n\nb", "> ", false, quoted[0..])) {
    // "> a\n\n> b"
}
```
