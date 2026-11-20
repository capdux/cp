# CP 标准库：跳表 + 哈希表双索引有序集合引擎 (`cap:zset`)

## 一、 架构总览与设计准则

`cap:zset` 是 CP 语言官方内置的纯 CP **SkipList 跳表（带 `span` 跨度）+ `StringHashMapUnmanaged(f64)` 双索引有序集合引擎（类 Redis ZSet）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。严格按照 `(score, member)` 升序排列（同分按成员字节字典序打破平局），兼顾 $O(1)$ 分数查询与 $O(\log N)$ 插入、更新、删除、排名定位与区间查询，适用于实时排行榜、延迟任务时间轮、滑动窗口限流统计与内存索引。

既可通过 `import { ... } from "cap:zset"` 导入，也支持 `import cap from "cap"; cap.zset.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/zset/zset.cp` | `// Source: cp/lib/zset/zset.cp (...)` | `ZSetError`、`ZSetEntry`、`ZSetAgg`、`SortedSet` |
| `lib/zset/index.cp` | `// Source: cp/lib/zset/index.cp (...)` | `cap:zset` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与辅助类型

- **`ZSetError`**：
  - `InvalidScore`：传入 `NaN` 分数或增量运算产生 `NaN`。
  - `InvalidRange`：非法区间参数。
  - `OutOfMemory`：底层跳表节点、层级切片、哈希索引或输出切片分配失败。
- **`ZSetEntry`**：`{ member: string, score: f64 }`。
- **`ZSetAgg`**：`Sum` | `Min` | `Max`（用于 `SortedSet.unionInto`）。

### 2. `SortedSet` 核心方法

- **生命周期**：
  - `SortedSet.init(alloc): SortedSet`（延迟分配，空集合零堆开销）
  - `deinit(*_)`
  - `SortedSet.freeEntries(alloc, entries)`（释放 `zpopMin` / `zpopMax` 返回的带独立 `member` 拷贝的切片）
- **写入与更新**：
  - `zadd(*_, member, score): bool ! ZSetError`：插入或更新成员分值；拒绝 `NaN`；若分值变更则先从跳表摘除再按新序重插；返回是否为全新插入。
  - `zincrby(*_, member, delta): f64 ! ZSetError`：按 `delta` 增量更新成员分值（不存在则从 `0.0` 起算），返回最新分值。
  - `zrem(*_, member): bool`：删除成员，返回是否存在。
- **点查与排名**：
  - `zscore(member): f64?`：$O(1)$ 查询成员分值。
  - `zcard(): usize`：$O(1)$ 返回基数。
  - `zrank(member): usize?`：$O(\log N)$ 利用跳表 `span` 跨度计算 0-based 升序排名。
  - `zrevrank(member): usize?`：$O(\log N)$ 计算 0-based 降序排名。
- **区间查询与弹出**：
  - `zrange(alloc, start: isize, stop: isize): []ZSetEntry ! ZSetError`：0-based 升序切片，支持负数索引（如 `0, -1`）；调用方用 `alloc.free(entries)` 释放。
  - `zrevrange(alloc, start: isize, stop: isize): []ZSetEntry ! ZSetError`：0-based 降序切片，支持负数索引；调用方用 `alloc.free(entries)` 释放。
  - `zrangeByScore(alloc, min_score, max_score): []ZSetEntry ! ZSetError`：返回 `[min_score, max_score]` 闭区间内升序条目；调用方用 `alloc.free(entries)` 释放。
  - `zcount(min_score, max_score): usize`：$O(\log N)$ 统计 `[min_score, max_score]` 区间内元素个数。
  - `zremRangeByScore(*_, min_score, max_score): usize`：删除 `[min_score, max_score]` 区间内全部成员并返回删除数量。
  - `zpopMin(*_, alloc, count): []ZSetEntry ! ZSetError` / `zpopMax(*_, alloc, count): []ZSetEntry ! ZSetError`：弹出最低/最高分的前 `count` 个条目；调用方用 `SortedSet.freeEntries(alloc, entries)` 释放。
- **集合聚合**：
  - `SortedSet.unionInto(alloc, sets: []const *SortedSet, agg: ZSetAgg): SortedSet ! ZSetError`：按 `Sum`/`Min`/`Max` 聚合多个有序集合生成新 `SortedSet`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SortedSet, ZSetAgg } from "cap:zset";
import cap from "cap";

pub fn demoZSet(): ! {
    const alloc = std.heap.page_allocator;

    let zs = SortedSet.init(alloc);
    defer zs.deinit();

    * = zs.zadd("alice", 100.0)?;
    * = zs.zadd("bob", 100.0)?;
    * = zs.zadd("carol", 250.0)?;

    // 同分按字典序：alice < bob < carol
    const rank_alice = zs.zrank("alice"); // 0
    const rank_bob = zs.zrank("bob");     // 1
    * = rank_alice;
    * = rank_bob;

    const top2 = zs.zrevrange(alloc, 0, 1)?;
    defer alloc.free(top2);

    let zs2 = cap.zset.SortedSet.init(alloc);
    defer zs2.deinit();
    * = zs2.zadd("bob", 50.0)?;

    const inputs: [2]*SortedSet = [&zs, &zs2];
    let merged = SortedSet.unionInto(alloc, inputs[0..], ZSetAgg.Sum)?;
    defer merged.deinit();
}
```
