# `cap:unicode_width` — Terminal Display Column Width (`wcwidth`)

Zero-allocation Unicode East Asian Width / terminal display column width calculator for UTF-8 strings (zero-width combining marks = 0, ASCII = 1, CJK/Fullwidth/Emoji = 2).

## Exports

- `isZeroWidth(cp: u32): bool`
- `isWideChar(cp: u32): bool`
- `codepointWidth(cp: u32): u8` (`0`, `1`, or `2`)
- `strDisplayWidth(s: []const u8): usize`
- `sliceToDisplayWidth(s: []const u8, max_cols: usize): []const u8`
