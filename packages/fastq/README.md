[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fastq` ([https://github.com/mcollina/fastq](https://github.com/mcollina/fastq)) | 许可：`ISC`

# `fastq`

零堆分配纯 CP 固定并发工作队列，对标 npm **#253** `fastq`（真实周下载 **1.92 亿/周**，Fastify/Pino 作者 Matteo Collina 出品），支持 O(1) 环形缓冲 `push`/`unshift` 优先级插队、`pause`/`unpause`、`nextTask`/`completeTask` 与 `kill` 清空。

## 核心 API

- `FastQueue.new(concurrency)` / `.push(id)` / `.unshift(id)`
- `.nextTask(): u64?` / `.completeTask(): bool`
- `.pause()` / `.unpause()` / `.kill()` / `.length()` / `.running()` / `.idle()`
