# CP 标准库：一致性哈希环与分布式路由手册 (`cap:ringhash`)

## 一、 架构总览与设计准则

`cap:ringhash` 是 CP 语言官方内置的纯 CP **一致性哈希与分布式路由算法库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为分布式缓存分片、有状态服务路由、多副本容灾与无状态负载均衡打造。既可通过 `import { ... } from "cap:ringhash"` 导入，也支持 `import cap from "cap"; cap.ringhash.*` 动态懒挂载调用。

### 三大核心算法原语
- **`ConsistentRing`（虚拟节点二分查找一致性哈希环）**：基于 `Wyhash` + `SplitMix64` 均匀映射 `weight * default_vnodes` 个虚拟节点至 64-bit 哈希环，支持 $O(\log V)$ 顺时针二分查找、`removeNode` 原地保序摘除、以及 `getReplicas` 顺时针严格物理节点去重多副本容灾。
- **`jumpConsistentHash`（Google Lamping-Veach 跳跃一致性哈希）**：$O(\ln N)$ 时间复杂度、$O(1)$ 零内存占用的无状态一致性哈希，满足严格均匀分布与单调最小迁移不变量（扩容 $N \to N+1$ 时仅约 $\frac{1}{N+1}$ 键迁移至新桶，其余键零迁移）。
- **`RendezvousHasher`（HRW 最高随机权重一致性哈希）**：基于指数加权 $\text{score}_i = \frac{w_i}{-\ln(u_i)}$ 的确定性路由，无需维护虚拟节点环即可实现严格按权重比例分配与单节点下线最小扰动。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ringhash/ringhash.cp` | `// Source: cp/lib/ringhash/ringhash.cp (...)` | `RingHashError`、`ConsistentRing`、`jumpConsistentHash`、`RendezvousHasher` |
| `lib/ringhash/index.cp` | `// Source: cp/lib/ringhash/index.cp (...)` | `cap:ringhash` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `RingHashError`
- `EmptyRing`：哈希环或节点集合为空，无法执行路由查询。
- `InvalidWeight`：`default_vnodes == 0`、`weight == 0` 或虚拟节点总数乘法溢出。
- `InvalidBucketCount`：`jumpConsistentHash` 传入 `num_buckets == 0` 或超出 `i64` 安全范围。
- `DuplicateNode`：尝试重复添加已存在的 `node_id`。
- `NodeNotFound`：尝试移除不存在的 `node_id`。
- `OutOfMemory`：底层节点列表、虚拟节点环或副本结果切片内存分配失败。

### 2. `ConsistentRing`
- **生命周期**：
  - `ConsistentRing.init(allocator: std.mem.Allocator, default_vnodes: usize): ConsistentRing ! RingHashError`
  - `deinit(*_)`：释放内部拷贝的所有 `node_id` 字符串及节点/虚拟节点缓冲。
- **成员操作**：
  - `addNode(*_, node_id: []const u8, weight: usize): void ! RingHashError`：按 `weight * default_vnodes` 生成虚拟节点并维护有序环；重复节点返回 `DuplicateNode`。
  - `removeNode(*_, node_id: []const u8): void ! RingHashError`：原地保序移除目标物理节点的全部虚拟节点；不存在返回 `NodeNotFound`。
  - `getNode(key: []const u8): []const u8 ! RingHashError`：顺时针二分查找最近虚拟节点对应的物理节点 ID（返回内部持有切片，在节点被移除或 `deinit()` 前有效）。
  - `getReplicas(allocator: std.mem.Allocator, key: []const u8, count: usize): [][]const u8 ! RingHashError`：顺时针返回最多 `min(count, nodeCount())` 个**去重后的不同物理节点**；调用方拥有外层返回切片（需 `allocator.free(replicas)`）。
  - `nodeCount(): usize` / `vnodeCount(): usize`：返回物理节点数与虚拟节点总数。

### 3. `jumpConsistentHash`
- `jumpConsistentHash(key_hash: u64, num_buckets: usize): usize ! RingHashError`
  - 将 64-bit 键哈希均匀映射到 `[0, num_buckets)`。

### 4. `RendezvousHasher`
- **生命周期**：
  - `RendezvousHasher.init(allocator: std.mem.Allocator): RendezvousHasher`
  - `deinit(*_)`
- **成员操作**：
  - `addNode(*_, node_id: []const u8, weight: usize): void ! RingHashError`
  - `removeNode(*_, node_id: []const u8): void ! RingHashError`
  - `getNode(key: []const u8): []const u8 ! RingHashError`
  - `getReplicas(allocator: std.mem.Allocator, key: []const u8, count: usize): [][]const u8 ! RingHashError`
  - `nodeCount(): usize`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { ConsistentRing, RendezvousHasher, jumpConsistentHash } from "cap:ringhash";
import cap from "cap";

pub fn demoRingHash(): ! {
    const alloc = std.heap.page_allocator;

    let ring = ConsistentRing.init(alloc, 128)?;
    defer ring.deinit();
    ring.addNode("cache-a", 1)?;
    ring.addNode("cache-b", 2)?;

    const primary = ring.getNode("session:42")?;
    const replicas = ring.getReplicas(alloc, "session:42", 2)?;
    defer alloc.free(replicas);
    * = primary;

    const bucket = jumpConsistentHash(0x123456789abcdef0, 16)?;
    * = bucket;

    let hrw = cap.ringhash.RendezvousHasher.init(alloc);
    defer hrw.deinit();
    hrw.addNode("worker-1", 1)?;
    const chosen = hrw.getNode("job:99")?;
    * = chosen;
}
```
