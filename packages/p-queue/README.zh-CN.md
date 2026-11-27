[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-queue

纯 CP 实现的零堆优先级任务队列，支持并发上限控制、暂停/恢复、批量派发与运行态/等待态统计。

- **源包对齐**：参考 npm [`p-queue`](https://github.com/sindresorhus/p-queue)（`MIT`）净室重构。
- **包版本**：`1.0.0`

## 安装与导入

```bash
cap add p-queue
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "p-queue": "^1.0.0"
  }
}
```

```typescript
import { TaskQueue, QueueOptions, QueueTask, PQ_MAX_TASKS } from "p-queue";
```

## 核心 API 参考

### 常量与数据结构

| 符号 | 定义 | 说明 |
| :--- | :--- | :--- |
| `PQ_MAX_TASKS` | `const PQ_MAX_TASKS: usize = 64` | 单个 `TaskQueue` 固定内联容量上限 |
| `QueueTask` | `class { id: u64, priority: i32, seq: u64 }` | 任务条目（`priority` 越大越优先，同优先级按 `seq` FIFO） |
| `QueueOptions` | `class { concurrency: usize, auto_start: bool, interval_cap: usize, interval_ms: u64 }` | 队列配置项，可通过 `QueueOptions.default()` 获取默认值 |

### `class TaskQueue`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `new` | `TaskQueue.new(max_concurrency: usize): TaskQueue` | 创建指定并发上限的活跃队列 |
| `withOptions` | `TaskQueue.withOptions(opts: QueueOptions): TaskQueue` | 使用 `QueueOptions` 创建队列（`auto_start: false` 时初始暂停） |
| `enqueue` / `add` | `enqueue(*_, id: u64, priority: i32): bool` | 入队任务 `id` 与优先级 `priority`；队列满（`PQ_MAX_TASKS`）返回 `false` |
| `addBatch` | `addBatch(*_, ids: &[]u64, priority: i32): usize` | 批量入队任务 ID，返回成功入队数量 |
| `dispatchNext` | `dispatchNext(*_): QueueTask?` | 弹出最高优先级任务并递增 `active`；受限或为空返回 `null` |
| `drainBatch` | `drainBatch(*_, out: []QueueTask): usize` | 在并发与暂停约束下批量派发至 `out`，返回派发条数 |
| `markCompleted` | `markCompleted(*_): bool` | 递减 `active` 并递增 `completed_total`；`active == 0` 返回 `false` |
| `peek` | `peek(): QueueTask?` | 查看队首最高优先级任务但不移除 |
| `remove` | `remove(*_, id: u64): bool` | 按 `id` 移除等待队列中的任务 |
| `setPriority` | `setPriority(*_, id: u64, new_priority: i32): bool` | 动态修改已入队任务的优先级 |
| `pause` / `start` | `pause(*_)` / `start(*_)` | 暂停 / 恢复派发 |
| `setConcurrency` | `setConcurrency(*_, max_concurrency: usize)` | 动态调整最大并发上限 |
| `clear` / `resetStats` | `clear(*_)` / `resetStats(*_)` | 清空等待队列 / 重置 `completed_total` 统计 |
| `size` / `activeCount` / `pending` | `size(): usize` / `activeCount(): usize` / `pending(): usize` | 等待任务数 / 正在执行数 |
| `concurrency` / `completedCount` | `concurrency(): usize` / `completedCount(): u64` | 当前并发上限 / 累计完成数 |
| `isPaused` / `isIdle` / `canDispatch` | `isPaused(): bool` / `isIdle(): bool` / `canDispatch(): bool` | 状态判断（是否暂停 / 是否空闲 / 是否可派发） |
| `onEmpty` / `onIdle` / `onSizeLessThan` | `onEmpty(): bool` / `onIdle(): bool` / `onSizeLessThan(limit: usize): bool` | 同步状态检查（队列为空 / 空闲 / 等待数小于 `limit`） |
| `sizeByPriority` / `sizeByExactPriority` | `sizeByPriority(min_priority: i32): usize` / `sizeByExactPriority(priority: i32): usize` | 按最低优先级或精确优先级统计等待任务数 |

## 使用示例

### 1. 优先级调度与并发控制

```typescript
import { TaskQueue } from "p-queue";

fn runScheduler(): u64 {
    let mut q = TaskQueue.new(2);
    q.add(101, 1);
    q.add(102, 10);
    q.add(103, 5);

    if (first := q.dispatchNext()) {
        // first.id == 102（最高优先级）
        q.markCompleted();
    }
    return q.completedCount();
}
```

### 2. 初始暂停、批量入队与批量派发

```typescript
import { TaskQueue, QueueOptions, QueueTask } from "p-queue";

fn batchDispatch(): usize {
    let mut opts = QueueOptions.default();
    opts.concurrency = 4;
    opts.auto_start = false;

    let mut q = TaskQueue.withOptions(opts);
    let ids = [10 as u64, 20 as u64, 30 as u64];
    q.addBatch(&ids, 5);

    let mut batch: [4]QueueTask = undefined;
    let n0 = q.drainBatch(&mut batch); // 暂停态返回 0

    q.start();
    let n1 = q.drainBatch(&mut batch); // 恢复后派发 3 条
    return n0 + n1;
}
```

## 与源包差异与 CP 安全亮点

- **零堆内联存储**：`items: [64]QueueTask` 直接位于栈上，无动态分配与 GC 停顿，适合高频 Actor 调度。
- **解耦式任务 ID 模型**：不同于 JS 绑定 `Promise` 回调，本包以 `u64` 任务 ID 驱动调度，便于与 CP Actor、线程池及状态机无缝结合。
- **同步布尔谓词**：`onEmpty()`、`onIdle()`、`onSizeLessThan(limit)` 直接返回 `bool`，零额外内存开销。
