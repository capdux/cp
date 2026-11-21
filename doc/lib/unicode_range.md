# `cap:unicode_range` — CSS `@font-face` Unicode 码点区间解析与通配符优化器

源自 Bun `css/rules/font_face.cp`，纯 CP 零堆分配实现 CSS `<urange>`（`U+0025` / `U+0025-00FF` / `U+4??` / `U+???`）解析、包含/重叠/合并判定与十六进制半字节边界自动 `?` 通配符压缩输出。

## 导出符号

- `MAX_CODEPOINT`(0x10FFFF)、`UnicodeRangeError`
- `UnicodeRange`: `single` / `init` / `parse` / `contains` / `count` / `isSingle` / `eql` / `overlaps` / `adjacentOrOverlaps` / `merge` / `toCss`
