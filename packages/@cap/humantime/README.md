# @cap/humantime

零堆分配的 CP 原生人类可读时长解析/格式化与 RFC 3339 时间戳库，Clean-Room MIT 实现对标 crates.io Top 115 包 `humantime`（9.5 亿+ 下载），跨包复用 `@cap/itoa`。

## 特性

- `parseDuration`：解析 `"2h 37min 5s"`、`"150ms"`、`"1day 2hr 3m 4s 500us 10ns"` 等人类可读时长为纳秒
- `formatDuration`：将纳秒时长格式化为简洁人类可读文本（复用 `@cap/itoa`）
- `formatRfc3339Seconds`：O(1) 公历算法将 Unix 秒时间戳格式化为 RFC 3339 (`YYYY-MM-DDTHH:MM:SSZ`)

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`tailhook/humantime`](https://github.com/tailhook/humantime)。
