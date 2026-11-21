# `cap:treap` — 零分配随机平衡树 / 顺序统计 Treap 库

零堆分配（Zero-Alloc）的纯 CP 随机平衡二叉搜索树（Treap）与顺序统计（Order-Statistic Tree）标准库，基于调用方固定容量 `TreapNode` 节点池、空闲链表与确定性 SplitMix64 伪随机优先级生成器，支持 O(log n) 插入、删除、查找、第 k 小元素、排名、前后界查询与闭区间计数。

## 核心特性

- **零堆分配（Zero-Alloc）**：所有树节点来自调用方预分配的 `[]TreapNode` 切片，删除节点即时归还内部空闲链表，支持无限次高频插入/删除循环且零内存泄漏。
- **确定性平衡（SplitMix64 PRNG）**：无全局状态，相同种子下构建的树拓扑完全可复现，期望树高 O(log n)（即使面对严格递增/递减键序列也不会退化）。
- **O(log n) 顺序统计与区间查询**：每个节点维护精确子树大小 `size`，原生支持 `kth`、`rank`、`lowerBound`、`upperBound`、`minEntry`、`maxEntry` 与溢出安全的 `rangeCount`。
- **内置结构不变量校验**：`verifyInvariants()` 一次性验证严格 BST 序、大顶堆优先级序、子树 `size` 精确性及 `live + free == capacity` 节点池守恒。

## 导出 API

### 常量与数据模型

- `TREAP_NIL: u32 = 0xFFFFFFFF`
- `TreapNode { key: i64, value: i64, priority: u64, left: u32, right: u32, size: u32 }`
- `TreapEntry { key: i64, value: i64 }`

### `Treap` 方法

- **生命周期与容量**：
  - `Treap.init(nodes: []TreapNode, seed: u64): Treap`
  - `len(): usize`、`isEmpty(): bool`、`capacity(): usize`、`clear(*_)`
- **增删改查**：
  - `insert(*_, key: i64, value: i64): bool`（已存在 key 原地更新 `value`；池满且需新增返回 `false`）
  - `remove(*_, key: i64): bool`（删除并归还节点至空闲链表）
  - `get(key: i64): i64?`、`contains(key: i64): bool`
- **顺序统计与范围查询**：
  - `kth(k: usize): TreapEntry?`（0-based 第 k 小元素）
  - `rank(key: i64): usize`（严格小于 `key` 的元素个数）
  - `lowerBound(key: i64): TreapEntry?`（首个 `>= key` 的元素）
  - `upperBound(key: i64): TreapEntry?`（首个 `> key` 的元素）
  - `minEntry(): TreapEntry?`、`maxEntry(): TreapEntry?`
  - `rangeCount(lo: i64, hi: i64): usize`（闭区间 `[lo, hi]` 内元素个数）
- **诊断校验**：
  - `verifyInvariants(): bool`

## 使用示例

```cp
import std from "std";
import { TreapNode, Treap } from "cap:treap";

fn example() {
    let pool: [64]TreapNode = std.mem.zeroes([64]TreapNode);
    let t = Treap.init(pool[0..], 42);

    * = t.insert(30, 300);
    * = t.insert(10, 100);
    * = t.insert(20, 200);

    * = t.get(20);             // 200
    * = t.kth(1);              // TreapEntry { key: 20, value: 200 }
    * = t.rank(25);            // 2
    * = t.rangeCount(10, 25);  // 2
    * = t.verifyInvariants();  // true
}
```
