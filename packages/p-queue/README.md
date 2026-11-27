[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# p-queue

Zero-heap priority task queue with bounded concurrency, pause/resume control, dynamic priority updates, batch draining, and active/pending/idle state tracking in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [npmjs.com/package/p-queue](https://www.npmjs.com/package/p-queue) |
| **Upstream Package** | `p-queue` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/sindresorhus/p-queue |
| **License** | MIT |

Keep the exact package name when migrating from TypeScript/npm to CP:

```ts
// TypeScript (npm)
import PQueue from "p-queue";
const queue = new PQueue({ concurrency: 2 });
```

```cp
// CP — same package name, zero-heap fixed-capacity scheduler
import { TaskQueue, QueueOptions, QueueTask } from "p-queue";
```

## Installation

```bash
cap add p-queue
```

## Quick Start & Practical Examples

### 1. Enqueue with Priorities, Dispatch & Complete

```cp
import { TaskQueue, QueueOptions, QueueTask } from "p-queue";

fn demoQueue() {
    // Configure via QueueOptions or TaskQueue.new(max_concurrency)
    mut opts = QueueOptions.default();
    opts.concurrency = 2;
    opts.auto_start = true;
    mut q = TaskQueue.withOptions(opts);

    // Higher priority numbers dispatch first; ties break in FIFO order (seq)
    let _ = q.add(101, 1);        // low priority
    let _ = q.add(102, 10);       // high priority
    let _ = q.enqueue(103, 5);    // alias for add

    // Batch enqueue with shared priority
    let batch_ids: [3]u64 = [201, 202, 203];
    let added = q.addBatch(&batch_ids, 2);

    // Inspect highest-priority task without removing it
    if (q.peek()) |top| {
        // top.id == 102, top.priority == 10
    }

    // Dispatch up to concurrency limit (2)
    if (q.dispatchNext()) |t1| {
        // t1.id == 102, q.activeCount() == 1 (alias: q.pending())
    }
    if (q.dispatchNext()) |t2| {
        // t2.id == 103, q.activeCount() == 2
    }
    // q.canDispatch() == false while active == max_concurrency

    // Complete tasks to free concurrency slots
    let _ = q.markCompleted();
    let _ = q.markCompleted();
    let done_total = q.completedCount(); // 2
}
```

### 2. Dynamic Priority, Removal, Pause/Resume & Batch Drain

```cp
import { TaskQueue, QueueTask } from "p-queue";

fn demoControls() {
    mut q = TaskQueue.new(4);
    let _ = q.add(1, 0);
    let _ = q.add(2, 0);
    let _ = q.add(3, 5);

    // Promote task 1 and cancel task 2
    let _ = q.setPriority(1, 20);
    let _ = q.remove(2);

    // Filter counts by priority
    let high_cnt = q.sizeByPriority(10);      // 1 (priority >= 10)
    let exact_cnt = q.sizeByExactPriority(5); // 1

    // Pause prevents dispatching until start() is called
    q.pause();
    let paused = q.isPaused();   // true
    let none = q.dispatchNext(); // null while paused
    q.start();

    // Drain up to available concurrency slots in one call
    mut out: [4]QueueTask = [QueueTask { id: 0, priority: 0, seq: 0 }; 4];
    let n = q.drainBatch(&mut out); // Dispatches task 1, then task 3

    // Readiness predicates matching npm p-queue semantics
    let empty = q.onEmpty();
    let idle = q.onIdle(); // Alias: q.isIdle()
    let under_limit = q.onSizeLessThan(10);

    q.setConcurrency(8);
    q.clear();
    q.resetStats();
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: `TaskQueue` uses an inline `[64]QueueTask` stack buffer — no dynamic allocations, predictable cache locality, and deterministic O(N) bounded latency.
- **Stable FIFO Tie-Breaking**: Every enqueued task receives a monotonically increasing `seq: u64`, guaranteeing FIFO ordering among equal-priority tasks.
- **Bounded Concurrency Guard**: `dispatchNext` and `drainBatch` enforce `active < max_concurrency` and `!paused`, preventing over-subscription.
- **No-Panic Capacity Handling**: `add` / `enqueue` return `false` when the 64-slot queue is full; `addBatch` returns the exact number of tasks inserted.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
