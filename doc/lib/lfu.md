# `cap:lfu` — 零堆分配 LFU 缓存与 W-TinyLFU 4-bit Count-Min Sketch 准入过滤器

纯 CP、零堆分配的高性能频率缓存与频次估计标准库，包含 4 行 4-bit 饱和 Count-Min Sketch（周期右移减半衰减）`TinyLfuSketch` 以及带 LRU 破平局的固定容量 LFU 缓存 `LfuCache`。

## 导入

```cp
import {
    TINYLFU_ROWS,
    TINYLFU_MAX_COUNT,
    TinyLfuSketch,
    LfuSlot,
    LfuCache,
} from "cap:lfu";
```

## 核心 API

- **`TinyLfuSketch`**（W-TinyLFU 4-bit 饱和频率估计器）：
  - `TinyLfuSketch.init(counters: []u8, reset_threshold: u32): TinyLfuSketch?` — 要求 `counters.len >= 8`、为 2 的幂且 `reset_threshold > 0`。
  - `increment(*_, key: u64): u8` — 4 行饱和自增（上限 `15`），达到 `reset_threshold` 时自动触发 `halve()`。
  - `estimate(key: u64): u8` — 返回 4 行最小估计频次（`0..15`）。
  - `shouldAdmit(candidate: u64, victim: u64): bool` — 当 `estimate(candidate) > estimate(victim)` 时准入。
  - `halve(*_)` / `clear(*_)` — 全表 4-bit 计数器无分支右移减半衰减 / 清零。
- **`LfuCache` & `LfuSlot`**（零堆分配固定容量 `u64 -> u64` LFU 缓存）：
  - `LfuCache.init(slots: []LfuSlot): LfuCache?`
  - `len(): usize`、`capacity(): usize`、`peek(key): u64?`、`get(*_, key): u64?`、`frequencyOf(key): u64?`
  - `put(*_, key, value): bool` — 满容量按最低 `freq` 淘汰，同频次按最早 `last_touch`（LRU）破平局；更新已有键递增频次不触发淘汰。
  - `remove(*_, key): bool`、`clear(*_)`。

## 示例

```cp
import std from "std";
import { LfuSlot, LfuCache, TinyLfuSketch } from "cap:lfu";

fn cacheExample(): u64? {
    let slots: [4]LfuSlot = std.mem.zeroes([4]LfuSlot);
    let cache = LfuCache.init(slots[0..]) ?? return null;
    * = cache.put(1, 100);
    * = cache.put(2, 200);
    * = cache.get(1);
    return cache.peek(1);
}
```
