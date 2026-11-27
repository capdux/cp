[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/yocto-queue

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yocto-queue` ([https://github.com/sindresorhus/yocto-queue](https://github.com/sindresorhus/yocto-queue))

零堆分配的 CP 原生 O(1) FIFO 队列，Clean-Room MIT 实现对标 npm 顶流包 `yocto-queue`（2 亿+/周下载，`p-limit` / `p-queue` 核心底层依赖）。

## 特性

- 固定栈环形缓冲（`YOCTO_QUEUE_CAP = 256`），无 GC、无堆分配、`= undefined` 零初始化开销
- O(1) `enqueue` / `dequeue` / `peek` / `peekAt` / `clear` / `size` / `drain`
- 同时提供无符号 `YoctoQueueU64` 与有符号 `YoctoQueueI64`

## 使用示例

```cp
import { YoctoQueueU64 } from "@cap/yocto-queue";

let q = YoctoQueueU64.new();
* = q.enqueue(10);
* = q.enqueue(20);
let r = q.dequeue(); // { ok: true, value: 10 }
```

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`sindresorhus/yocto-queue`](https://github.com/sindresorhus/yocto-queue)。
