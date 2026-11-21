# `cap:jsonc` — Zero-Alloc JSONC Comment & Trailing-Comma Stripper

Zero-allocation pure-CP scanner and stripper for JSON with Comments (`//`, `/* ... */`) and trailing commas (such as `tsconfig.json` and `cap.json`).

## Exports

- `stripJsonc(input: []const u8, out: []u8): ([]const u8)?` — Strips `//` line comments, `/* ... */` block comments, and trailing commas before `}` or `]` into `out`, preserving double-quoted string contents (including escapes like `\"` and `\\`). Returns `null` on unterminated block comment, unterminated string, or insufficient buffer capacity.
- `stripJsoncPreserveLines(input: []const u8, out: []u8): ([]const u8)?` — Same as `stripJsonc`, but preserves `\n` newlines inside multi-line `/* ... */` block comments so line numbers stay aligned for config diagnostics.
- `hasJsoncComments(input: []const u8): bool` — String-aware fast check for any `//` or `/*` comments outside string literals.
- `countJsoncTrailingCommas(input: []const u8): usize` — Counts trailing commas before `}` / `]` (skipping intervening whitespace and comments, ignoring strings and comments).

## Example

```cp
import { stripJsonc, stripJsoncPreserveLines, hasJsoncComments } from "cap:jsonc";

const raw = "{\n  // compiler options\n  \"strict\": true,\n}\n";
let buf: [256]u8 = [0; 256];
if (clean := stripJsonc(raw, buf[0..])) {
    // "{\n  \n  \"strict\": true\n}\n"
}
```
