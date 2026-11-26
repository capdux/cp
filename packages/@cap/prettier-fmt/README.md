# @cap/prettier-fmt

零堆分配的 CP 原生 JSON 与 CSS 代码美化格式化器，Clean-Room MIT 实现对标 npm `prettier`。

## 特性

- `formatJson(input, opts, out)`：支持自定义缩进宽度（`tab_width`）、制表符（`use_tabs`）、尾随换行（`trailing_newline`），保持空 `{}`/`[]` 紧凑并完整保护字符串字面量
- `formatCss(input, opts, out)`：规范缩进 CSS 规则块、`prop: value;` 间距与多规则空行分隔

## 致谢

Clean-Room MIT 实现，API 灵感对标开源生态 [`prettier/prettier`](https://github.com/prettier/prettier)。
