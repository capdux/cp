# @cap/postcss-parse

零堆分配的 CP 原生 CSS AST 解析、查询、修改与序列化引擎，Clean-Room MIT 实现对标 npm Top 25 顶流包 `postcss` v8（1.5 亿+/周下载）。

## 特性

- **固定池零堆 AST (`PostCssSheet`)**：支持最多 32 条规则/At-Rule、64 条属性声明与 16 条注释，全程零堆分配
- **完整语法覆盖**：
  - 选择器规则 `selector { prop: val !important; }`
  - 语句型与容器型 At-Rule（`@charset` / `@import` / `@media (...) { ... }` / `@supports (...) { ... }` / `@font-face { ... }`）
  - 剥离并记录 `/* ... */` 注释
  - 引号字符串保护（如 `content: ";}"` 不会被误切分）
- **AST 查询与原地修改**：
  - `ruleCount()` / `declCount()` / `atRuleCount()` / `commentCount()`
  - `findDecl(selector, prop, out)` / `isDeclImportant(selector, prop)`
  - `setDecl(*_, selector, prop, val, important)`（原地修改已有属性或追加新属性/规则）
  - `toCss(out)`（规范 CSS 往返序列化，含缩进、嵌套 At-Rule 与 `!important`）

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`postcss/postcss`](https://github.com/postcss/postcss)。
