[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# p-queue

面向 CP 的零堆内存优先级任务调度队列，支持并发上限控制、暂停/恢复、动态调优优先级、批量出队以及活跃/等待/空闲状态追踪。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [npmjs.com/package/p-queue](https://www.npmjs.com/package/p-queue) |
| **上游原包名** | `p-queue`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/sindresorhus/p-queue |
| **许可证** | MIT |

从 TypeScript/npm 迁移到 CP 保持原包名不变：

```ts
// TypeScript (npm)
import PQueue from "p-queue";
const queue = new PQueue({ concurrency: 2 });
```

```cp
// CP —— 原包名直接导入，零堆定长调度队列
import { TaskQueue, QueueOptions, QueueTask } from "p-queue";
```

## 安装

```bash
cap add p-queue
```

## 实战示例

### 1. 优先级入队、并发调度与完成确认

```cp
import { TaskQueue, QueueOptions, QueueTask } from "p-queue";

fn demoQueue() {
    // 通过 QueueOptions 或 TaskQueue.new(max_concurrency) 创建
    mut opts = QueueOptions.default();
    opts.concurrency = 2;
    opts.auto_start = true;
    mut q = TaskQueue.withOptions(opts);

    // 优先级数值越大越先调度；同优先级按 seq 保持 FIFO 顺序
    let _ = q.add(101, 1);        // 低优先级
    let _ = q.add(102, 10);       // 高优先级
    let _ = q.enqueue(103, 5);    // add 别名

    // 批量入队同一优先级任务
    let batch_ids: [3]u64 = [201, 202, 203];
    let added = q.addBatch(&batch_ids, 2);

    // 预览队首最高优先级任务（不出队）
    if (q.peek()) |top| {
        // top.id == 102, top.priority == 10
    }

    // 按并发上限（2）依次调度
    if (q.dispatchNext()) |t1| {
        // t1.id == 102, q.activeCount() == 1（别名：q.pending()）
    }
    if (q.dispatchNext()) |t2| {
        // t2.id == 103, q.activeCount() == 2
    }
    // 达到并发上限时 q.canDispatch() == false

    // 标记任务完成以释放并发槽位
    let _ = q.markCompleted();
    let _ = q.markCompleted();
    let done_total = q.completedCount(); // 2
}
```

### 2. 动态改优先级、取消任务、暂停/恢复与批量拉取

```cp
import { TaskQueue, QueueTask } from "p-queue";

fn demoControls() {
    mut q = TaskQueue.new(4);
    let _ = q.add(1, 0);
    let _ = q.add(2, 0);
    let _ = q.add(3, 5);

    // 提升任务 1 的优先级并移除任务 2
    let _ = q.setPriority(1, 20);
    let _ = q.remove(2);

    // 按优先级统计排队任务数
    let high_cnt = q.sizeByPriority(10);      // 1（priority >= 10）
    let exact_cnt = q.sizeByExactPriority(5); // 1

    // 暂停后无法调度，直到调用 start() 恢复
    q.pause();
    let paused = q.isPaused();   // true
    let none = q.dispatchNext(); // 暂停期间返回 null
    q.start();

    // 一次批量拉取当前可用并发槽位内的所有任务
    mut out: [4]QueueTask = [QueueTask { id: 0, priority: 0, seq: 0 }; 4];
    let n = q.drainBatch(&mut out); // 依次拉取任务 1、任务 3

    // 对标 npm p-queue 的状态判断
    let empty = q.onEmpty();
    let idle = q.onIdle(); // 别名：q.isIdle()
    let under_limit = q.onSizeLessThan(10);

    q.setConcurrency(8);
    q.clear();
    q.resetStats();
}
```

## 性能与安全亮点

- **零堆分配**：`TaskQueue` 内置固定 `[64]QueueTask` 栈数组，无动态堆分配与碎片，缓存友好。
- **稳定 FIFO 排序**：每条任务自动分配递增序号 `seq: u64`，相同优先级严格遵循先入先出。
- **并发与暂停守卫**：`dispatchNext` 与 `drainBatch` 严格校验 `active < max_concurrency` 与 `!paused`。
- **满队列无 Panic**：队列满（64 条）时 `add`/`enqueue` 返回 `false`，`addBatch` 返回实际成功入队数量。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
