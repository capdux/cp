# @cap/ignore

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ignore` ([https://github.com/kaelzhang/node-ignore](https://github.com/kaelzhang/node-ignore))

零堆分配的 CP 原生 `.gitignore` 规范规则引擎，Clean-Room MIT 实现对标 npm 顶流包 `ignore`（2.5 亿+/周下载），跨包复用 `@cap/minimatch` 与 `@cap/normalize-path`。

## 特性

- 严格遵循 Git `.gitignore` 规范：`#` 注释、`!` 否定重包含、前导 `/` 锚定、尾随 `/` 仅目录匹配与子路径继承、Windows 反斜杠自动归一化
- `IgnoreMatcher.addLines(content)` 一次性加载完整 `.gitignore` 文本
- `ignores(path)` 与 `filter(paths, out_indices)` 零堆过滤

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`kaelzhang/node-ignore`](https://github.com/kaelzhang/node-ignore)。
