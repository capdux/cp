# `cap:logfmt` — Structured `logfmt` Log Parser & Encoder

> 参考来源：/data/dev/bun/src/logger.zig | 吸收日期：2026-11-22

Zero-allocation `logfmt` (`key=value`) structured logging parser, key lookup, and canonical quoting/escaping encoder.

## Exports

- `LogfmtField` — `key`, `raw_value`, `is_quoted`, `has_value`, `decodeValue(out): ([]const u8)?`.
- `LogfmtIter` — `init(line)`, `next(*_): LogfmtField?`, `hasError(): bool` (handles quoted escapes, bare boolean keys, unclosed quote detection).
- `getLogfmtValue(line, key, out): ([]const u8)?`
- `appendLogfmtPair(out, pos: *usize, key, val): bool` — Automatically quotes/escapes values with spaces, `=`, `"`, `\`, `\n`, `\r`, `\t`.

## 上游行为契约与日志解析边界

1. **裸布尔键与转义值语义**：
   - `logfmt` 允许无 `=` 的裸键（如 `level=info debug`），`LogfmtIter` 将其解析为 `has_value = false`；带引号值支持 `\"`/`\\`/`\n`/`\r`/`\t` 解转义（通过 `decodeValue(out)` 零堆分配解码）。
2. **日志注入防御 (`appendLogfmtPair`)**：
   - 序列化不可信用户输入时，`appendLogfmtPair` 自动对包含空白、`=`、`"`、`\` 或 `\r`/`\n` 的值加双引号并转义换行符，彻底防止单行结构化日志被 CRLF 伪造多行记录。
3. **畸形行检测 (`hasError`)**：
   - 遇到未闭合双引号时 `LogfmtIter.next()` 停止并返回 `null`，同时 `hasError()` 返回 `true` 供上游监控告警。
