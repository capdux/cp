# `cap:bloom_cuckoo` — 概率集合数据结构标准库

## 概述

`cap:bloom_cuckoo` 提供纯 CP 实现的三种工业级概率集合数据结构，适用于高并发去重、缓存穿透防护与低内存成员检索：

- **`BloomFilter`**：经典位图布隆过滤器，根据预期元素数与目标假阳性率自动计算最优比特数 $m$ 与哈希函数数 $k$，支持 `merge` 并集与 `clear` 清零。
- **`CountingBloomFilter`**：基于 8-bit 饱和计数器（0–255，防溢出回零）的可删除计数布隆过滤器。
- **`CuckooFilter`**：基于 4 槽位桶与 16-bit 指纹的布谷鸟过滤器（`MAX_KICKS = 500`），兼具高空间利用率与精确删除能力。

支持静态导入 `import { BloomFilter, CountingBloomFilter, CuckooFilter, FilterError } from "cap:bloom_cuckoo";` 以及动态挂载 `cap.bloom_cuckoo`。

## 核心类型

| 类型 | 说明 |
| --- | --- |
| `FilterError` | 错误集合：`InvalidCapacity`、`InvalidFalsePositiveRate`、`FilterFull`、`IncompatibleFilters` |
| `BloomFilter` | 紧凑位图布隆过滤器（Kirsch-Mitzenmacher 双重哈希） |
| `CountingBloomFilter` | 8-bit 饱和计数器布隆过滤器，支持安全删除 |
| `CuckooFilter` | 4-slot 桶 + 16-bit 指纹布谷鸟过滤器（`MAX_KICKS = 500`） |
| `Bucket` | `CuckooFilter` 内部 4 槽位指纹桶 `[4]u16` |

## API 速查

### `BloomFilter`
- `BloomFilter.initWithRate(alloc, expected_items, fp_rate): FilterError!BloomFilter`
- `deinit(&self, alloc)`
- `insert(&self, item: []const u8)`
- `contains(self, item: []const u8): bool`
- `merge(&self, other: BloomFilter): FilterError!void`
- `clear(&self)`
- `bitCount(self): usize`
- `hashCount(self): u32`

### `CountingBloomFilter`
- `CountingBloomFilter.initWithRate(alloc, expected_items, fp_rate): FilterError!CountingBloomFilter`
- `deinit(&self, alloc)`
- `insert(&self, item: []const u8)`
- `remove(&self, item: []const u8): bool`
- `contains(self, item: []const u8): bool`
- `clear(&self)`

### `CuckooFilter`
- `CuckooFilter.init(alloc, capacity): FilterError!CuckooFilter`
- `deinit(&self, alloc)`
- `insert(&self, item: []const u8): FilterError!void`
- `contains(self, item: []const u8): bool`
- `remove(&self, item: []const u8): bool`
- `count(self): usize`

## 示例

```cp
import { BloomFilter, CountingBloomFilter, CuckooFilter } from "cap:bloom_cuckoo";
const std = @import("std");

pub fn main() {
    const alloc = std.heap.page_allocator;

    let bf = try BloomFilter.initWithRate(alloc, 10000, 0.01);
    defer bf.deinit(alloc);
    bf.insert("user:42");
    _ = bf.contains("user:42"); // true

    let cbf = try CountingBloomFilter.initWithRate(alloc, 1024, 0.01);
    defer cbf.deinit(alloc);
    cbf.insert("session:1");
    _ = cbf.remove("session:1"); // true

    let cf = try CuckooFilter.init(alloc, 1024);
    defer cf.deinit(alloc);
    try cf.insert("token:abc");
    _ = cf.contains("token:abc"); // true
    _ = cf.remove("token:abc");   // true
}
```
