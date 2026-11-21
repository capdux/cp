# `cap:roaring_bitmap` — 工业级 Roaring Bitmap 压缩位图集合引擎

## 概述

`cap:roaring_bitmap`（同时支持 `import cap from "cap"; cap.roaring_bitmap` 动态挂载）是基于纯 CP 实现的工业级 Roaring Bitmap 压缩位图集合引擎。

其核心设计将 32 位无符号整数 `u32` 按高 16 位 `u16` 分桶（Bucket Key），低 16 位按密度自适应选择容器：
- **稀疏容器 `ArrayContainer`**：有序 `u16` 动态数组，元素数 `<= 4096` 时使用，`O(log N)` 二分查找；超过 `4096` 自动升级为 `BitsetContainer`。
- **稠密容器 `BitsetContainer`**：固定 8KB（`1024 * u64`）位图，元素数 `> 4096` 时使用，`O(1)` 读写与硬件 `popcount` 加速；元素删减至 `<= 4096` 时自动降级回 `ArrayContainer`。

## 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `RoaringBitmap` | 压缩位图集合主结构，管理按 `key: u16` 有序排列的 `Bucket` 列表 |
| `Bucket` | 分桶单元，包含 `{ key: u16, container: Container }` |
| `Container` | 标签联合体（`.array: ArrayContainer` / `.bitset: BitsetContainer`） |
| `ArrayContainer` | 稀疏有序 `u16` 数组容器（阈值 `ARRAY_MAX_SIZE = 4096`） |
| `BitsetContainer` | 稠密 8KB `[1024]u64` 位图容器（`BITSET_WORDS = 1024`） |
| `RoaringError` | 错误集（`InvalidFormat`, `OutOfMemory`） |

## API 速查

- **构造与释放**：
  - `RoaringBitmap.init(alloc: std.mem.Allocator) RoaringBitmap`
  - `deinit(&self)`
- **增删改查**：
  - `add(&self, val: u32) !bool` — 插入元素，返回是否为新插入
  - `remove(&self, val: u32) !bool` — 删除元素，返回是否存在并被删除
  - `contains(self, val: u32) bool` — 查询元素是否存在
- **统计与极值**：
  - `cardinality(self) u64` — 返回总元素个数
  - `rank(self, val: u32) u64` — 返回 `<= val` 的元素个数
  - `minimum(self) u32?` — 返回最小值（空集返回 `null`）
  - `maximum(self) u32?` — 返回最大值（空集返回 `null`）
- **集合运算**：
  - `RoaringBitmap.unionWith(alloc, a, b) !RoaringBitmap` — 并集 `a ∪ b`
  - `RoaringBitmap.intersectWith(alloc, a, b) !RoaringBitmap` — 交集 `a ∩ b`
  - `RoaringBitmap.differenceWith(alloc, a, b) !RoaringBitmap` — 差集 `a \ b`
- **迭代、压缩与序列化**：
  - `toArray(self, alloc) ![]u32` — 导出升序 `u32` 切片
  - `runOptimize(&self) !bool` — 整理容器形态并回收多余容量
  - `toOwnedSlice(self, alloc) ![]u8` — 序列化为二进制切片
  - `RoaringBitmap.fromSlice(alloc, data) !RoaringBitmap` — 从二进制切片反序列化

## 示例

```cp
import { RoaringBitmap } from "cap:roaring_bitmap";

let bm = RoaringBitmap.init(alloc);
defer bm.deinit();

_ = try bm.add(10);
_ = try bm.add(70000);
const has = bm.contains(70000); // true
const cnt = bm.cardinality();   // 2
const r = bm.rank(70000);       // 2
```
