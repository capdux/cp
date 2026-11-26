# @cap/ansi-styles

零堆分配的 CP 原生终端 ANSI SGR 转义样式库，Clean-Room MIT 实现对标 npm 顶流包 `ansi-styles`（4 亿+/周下载，`chalk` 核心依赖），跨包复用 `@cap/color-convert`（及传递依赖 `@cap/color-name`）。

## 特性

- 完整内置 16 色前景/背景、亮色与全部 SGR 修饰符（`BOLD`/`DIM`/`ITALIC`/`UNDERLINE`/`STRIKETHROUGH`/`INVERSE`/`OVERLINE`）
- `formatAnsi256` / `formatAnsi16m` 零堆生成 256 色与 24-bit Truecolor SGR 序列
- `formatHexToAnsi16m` / `formatKeywordToAnsi16m` / `formatRgbToAnsi256` 直接委托 `@cap/color-convert`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`chalk/ansi-styles`](https://github.com/chalk/ansi-styles)。
