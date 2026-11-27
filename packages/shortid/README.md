# @cap/shortid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shortid` ([https://github.com/dylang/shortid](https://github.com/dylang/shortid))

零堆分配的 CP 原生 URL 安全短 ID 生成与校验库，Clean-Room MIT 实现对标 npm `shortid`。

## 特性

- 64 字符 URL 安全字母表（`[0-9a-zA-Z_-]`），基于 SplitMix64 确定性乱序
- 内嵌集群 `worker_id`（`0..15`）与版本号，支持同秒内无冲突单调递增计数器
- `isValidShortId`、`decodeWorker`、`decodeVersion` 零堆校验与反解

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`dylang/shortid`](https://github.com/dylang/shortid)。
