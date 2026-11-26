# @cap/minimatch

零堆分配的 CP 原生 Glob 匹配工具，Clean-Room MIT 实现对标 npm 顶流包 `minimatch`（4 亿+/周下载），跨包零重复复用 `@cap/micromatch` 与 `@cap/picomatch`。

## 特性

- 完整支持 `minimatch` 选项：`dot`、`nocase`、`match_base`、`noglobstar`、`nonegate`、`nocomment`、`flip_negate`
- 支持 `#` 注释模式、`!` 取反、`{a,b}` / `{1..5}` 花括号展开与 `**` globstar
- `minimatchList` / `minimatchFilterAll` / `minimatchRejectAll` 零堆批量过滤

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`isaacs/minimatch`](https://github.com/isaacs/minimatch)。
