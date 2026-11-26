# @cap/jest-diff

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jest-diff` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest))

零堆分配的纯 CP Jest 风格统一差异（Unified Diff）对比引擎，Clean-Room MIT 实现对标 npm Top 50 包 `jest-diff`（周下载 9000 万+），跨包复用 `@cap/similar-diff` 的行级 LCS 差分算法与 `@cap/picocolors` 的 ANSI 终端着色能力。

## 特性

- **Jest 风格统一输出**：`diffLinesUnified` 生成带头部注释（默认 `"- Expected"` / `"+ Received"`）、可选变更计数、`"- "` / `"+ "` / `"  "` 前缀行的统一差异视图
- **上下文折叠与展开**：通过 `context_lines` 与 `expand` 控制上下文窗口及 `@@ ... @@` 折叠标记
- **ANSI 终端着色**：按 `use_color` 自动委托 `@cap/picocolors` 对删除行（红）、新增行（绿）与上下文行（灰）着色
- **差异统计与无差异提示**：`diffStats` 返回精确的增/删/相等行数；完全相同时输出 `NO_DIFF_MESSAGE`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`jestjs/jest` (`jest-diff`)](https://github.com/jestjs/jest/tree/main/packages/jest-diff)。
