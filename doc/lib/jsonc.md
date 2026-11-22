# `cap:jsonc` — Zero-Alloc JSONC Comment & Trailing-Comma Stripper

> 参考来源：/data/dev/bun/src/js_parser.zig | 吸收日期：2026-11-22

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

## 上游行为契约与配置诊断边界

1. **字符串内注释与转义保护**：
   - `stripJsonc` 与 `hasJsoncComments` 在双引号字符串内完整保留 `"https://example.com"`、`"/* not comment */"` 及 `\"`/`\\` 转义序列，绝不误删 URL 或路径中的 `//`。
2. **行号对齐诊断模式 (`stripJsoncPreserveLines`)**：
   - 解析 `tsconfig.json` / `cap.json` 等配置文件后交给标准 `cap:json` 解析器时，推荐使用 `stripJsoncPreserveLines` 保留多行块注释中的 `\n`，确保下游 JSON 语法报错行号与原始文件 1:1 对齐。
3. **未闭合结构安全拒绝**：
   - 遇到未闭合的 `/* ...` 块注释、未闭合双引号字符串或 `out` 缓冲容量不足（建议至少 `out.len >= input.len`）时，确定性返回 `null`。
