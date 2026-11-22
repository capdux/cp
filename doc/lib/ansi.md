# `cap:ansi` — Unified ANSI Terminal Styling, SGR, Hyperlinks & Stripping

> 参考来源：`/data/dev/bun/docs/runtime/color.mdx`、`/data/dev/bun/src/bun_core/output.cp` | 吸收日期：2026-11-22

Zero-allocation pure-CP ANSI/VT100 terminal toolkit combining:
- **OSC 8 Hyperlinks** (`link.cp`): ST/BEL hyperlink encoding, zero-copy parsing, detection, and stripping.
- **SGR Style & Color Codec** (`sgr.cp`): 16-color, 256-color palette, and 24-bit TrueColor RGB formatting and atomic incremental SGR decoding.
- **ANSI Escape Stripper** (`strip.cp`): CSI/OSC/DCS/APC/PM/SOS/charset/2-byte ESC/C1 CSI stripping and visible length/codepoint counting.
- **16-Color Style Builder** (`style.cp`): ergonomic `AnsiColor` / `AnsiStyle` formatter and `visibleLen`.

## Exports

### OSC 8 Hyperlinks (`./link.cp`)
- `Osc8Link` (`url: []const u8`, `text: []const u8`, `params: []const u8`)
- `formatOsc8(url, text, params, out): ([]const u8)?` — ST (`\x1b\\`) hyperlink formatter
- `formatOsc8Bel(url, text, out): ([]const u8)?` — BEL (`\x07`) hyperlink formatter
- `parseOsc8(input): Osc8Link?` — Zero-copy single OSC 8 hyperlink parser (ST & BEL)
- `stripOsc8(input, out): ([]const u8)?` — Strips OSC 8 control sequences while preserving visible text
- `hasOsc8(input): bool` — Fast `\x1b]8;` scan
- `osc8EncodedLen(url, text, params): usize` / `osc8BelEncodedLen(url, text): usize`

### SGR Codec (`./sgr.cp`)
- `SgrColorKind` (`none`, `ansi16`, `ansi256`, `rgb`)
- `SgrColor` (`none()`, `ansi16(code)`, `ansi256(idx)`, `rgb(r, g, b)`, `isNone()`, `equals(other)`)
- `SgrStyle` (`plain()`, `reset(*_)`, `isPlain()`, `formatOpen(out)`)
- `formatStyled(text, style, out): ([]const u8)?`
- `applySgrSequence(seq, style: *SgrStyle): bool`

### Escape Stripper (`./strip.cp`)
- `hasAnsi(src): bool`
- `stripAnsi(src, out): ([]const u8)?`
- `strippedByteLen(src): usize`
- `visibleCodepointCount(src): usize`

### 16-Color Style Builder (`./style.cp`)
- `AnsiColor` (`black`..`white`, `bright_black`..`bright_white`, `fgCode()`, `bgCode()`)
- `AnsiStyle` (`isEmpty()`, `format(text, out)`, `formatRgb(r, g, b, text, out)`)
- `visibleLen(input): usize`

## Example

```cp
import {
    SgrColor,
    SgrStyle,
    formatStyled,
    formatOsc8,
    stripAnsi,
    hasAnsi,
} from "cap:ansi";

let buf: [256]u8 = [0; 256];
let style = SgrStyle.plain();
style.bold = true;
style.fg = SgrColor.rgb(80, 200, 120);

if (styled := formatStyled("Ready", style, buf[0..])) {
    let plain_buf: [64]u8 = [0; 64];
    * = stripAnsi(styled, plain_buf[0..]); // "Ready"
}
```

## 上游终端兼容边界与零分配注意事项

1. **三档色彩深度支持（`SgrColor`）**：
   - 对齐 Bun `ansi-16` / `ansi-256` / `ansi-16m` 色彩能力：`SgrColor.ansi16(code)`、`SgrColor.ansi256(idx)` 与 `SgrColor.rgb(r, g, b)` 分别生成标准 4-bit、8-bit 与 24-bit TrueColor SGR 序列；
   - `applySgrSequence` 支持增量原子解析复合分号参数（如 `\x1b[1;38;2;80;200;120m`），状态全部保存在栈结构体 `SgrStyle` 中，零堆分配。
2. **OSC 8 超链接双终结符兼容**：
   - `formatOsc8` 默认生成标准 ST（`\x1b\\`）终结符，兼顾现代终端与严格 ECMA-48 解析器；对仅识别 BEL（`\x07`）的传统终端可使用 `formatOsc8Bel`；`parseOsc8` 与 `stripOsc8` 同时兼容两种终结符。
3. **无半截转义序列的缓冲边界保护**：
   - 所有写入型 API（`formatStyled`、`formatOsc8`、`stripAnsi`、`stripOsc8`）在调用方输出缓冲空间不足时均安全返回 `null`，绝不截断输出不完整的 `\x1b[...` 残留控制码污染终端状态；可用 `osc8EncodedLen` / `strippedByteLen` 预先精确计算所需字节数。
