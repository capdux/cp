# @cap/ms

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ms` ([https://github.com/vercel/ms](https://github.com/vercel/ms))

零堆分配的 CP 原生毫秒转换工具库，Clean-Room MIT 实现对标 npm **Top 6** 顶流包 `ms`（4 亿+/周下载），跨包复用 `@cap/itoa`。

## 特性

- `parseMs`：解析 `"2 days"`、`"10h"`、`"2.5 hrs"`、`"-3 days"`、`"100"` 等格式为毫秒 `i64`，内置 100 字符防 DoS 保护
- `formatMs(ms, long_fmt, out)`：支持紧凑（`"1m"`, `"2d"`）与长文本（`"1 minute"`, `"2 days"`）双向输出

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`vercel/ms`](https://github.com/vercel/ms)。
