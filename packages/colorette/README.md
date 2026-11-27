# @cap/colorette

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:colorette` ([https://github.com/jorgebucaran/colorette](https://github.com/jorgebucaran/colorette))

零堆分配的纯 CP 终端 ANSI 颜色格式化库，Clean-Room MIT 实现对标 npm Top 20 包 `colorette` v2（周下载 1.5 亿+），直接复用 `@cap/ansi-styles` 的 `StylePair` 常量，零重复实现。

## 特性

- **环境检测**：`isColorSupported(no_color, force_color, term, is_tty)` 完整遵循 `NO_COLOR`、`FORCE_COLOR`、`TERM=dumb` 与 TTY 规则
- **实例构造**：`ColoretteOptions`、`createColors(use_color)` 与 `createColorsWithOptions(opts)`，支持一键开启或禁用透传
- **完整样式集**：内置全部 SGR 修饰符、标准/亮前景色与标准/亮背景色方法
- **嵌套还原**：支持嵌套 close-code 自动还原（含 `bold`/`dim` 的 `\x1b[22m` 特殊还原）与零堆 `[]u8` 缓冲安全截断

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`jorgebucaran/colorette`](https://github.com/jorgebucaran/colorette)。
