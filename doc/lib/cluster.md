# CP 标准库：分布式集群协调与分片路由原语手册 (`cap:cluster` / `cap.cluster`)

## 一、 架构总览与设计准则

`cap:cluster` 是 CP 语言官方内置的纯原生分布式集群协调与分片路由标准库，专为百万级 AI Agent 集群分片路由、SWIM Gossip 健康成员表与防脑裂选主租约打造。包含三大核心组件：

1. **`ConsistentHashRing`**（`lib/cluster/hashRing.cp`）：基于 FNV-1a 64 + SplitMix64 雪崩双阶段散列与有序虚拟节点环的 $O(\log(N \cdot V))$ 二分路由环，支持物理节点动态增删、最小键迁移、所有权安全深拷贝与顺时针物理节点去重副本选取 (`getReplicaNodes`)。
2. **`MembershipTable`**（`lib/cluster/membership.cp`）：基于 SWIM Incarnation CRDT 语义的确定性集群成员健康状态表，支持外部注入 `now_ms` 时钟驱动 (`tick`) 自动推进 `Alive -> Suspect -> Dead`，并严格遵循「高 `incarnation` 胜出，同 `incarnation` 按 `Dead > Suspect > Alive` 收敛」的防震荡合并法则。
3. **`LeaderLease`**（`lib/cluster/lease.cp`）：支持确定性时钟与单调递增屏障令牌（`fencing_token`）的防脑裂选主租约协调器，通过 `validateToken(token)` 严格拦截旧主过时写入。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:cluster"` 与 `import cap from "cap"; cap.cluster.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/cluster/hashRing.cp` | `// Source: cp/lib/cluster/hashRing.cp (Virtual-Node Consistent Hashing Ring)` | `ClusterErr`、`RingVNode`、`ConsistentHashRing` |
| `lib/cluster/membership.cp` | `// Source: cp/lib/cluster/membership.cp (SWIM-Style Gossip Membership & Incarnation CRDT)` | `MemberState`、`MemberInfo`、`MembershipTable` |
| `lib/cluster/lease.cp` | `// Source: cp/lib/cluster/lease.cp (Monotonic Fencing-Token Leader Lease Coordinator)` | `LeaseInfo`、`LeaderLease` |
| `lib/cluster/index.cp` | `// Source: cp/lib/cluster/index.cp (Unified Exports for cap:cluster)` | `cap:cluster` 与 `cap.cluster` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 虚拟节点一致性哈希环（`hashRing.cp`）

- **`ClusterErr`**：`OutOfMemory`、`InvalidParameter`、`DuplicateNode`、`NodeNotFound`
- **`RingVNode`**：`hash: u64`、`node_id: string`
- **`ConsistentHashRing`**：
  - 生命周期：`init(alloc): ConsistentHashRing`、`deinit(*_)`
  - 节点变更：`addNode(*_, node_id, virtual_nodes): !`（拒绝空 `node_id`、`virtual_nodes == 0` 或重复节点）、`removeNode(*_, node_id): bool`
  - 观测查询：`nodeCount(): usize`、`vnodeCount(): usize`、`hasNode(node_id): bool`
  - 分片路由：`getNode(key): string?`（空环安全返回 `null`）、`getReplicaNodes(alloc, key, replication_factor): ([]string)!`（沿环顺时针去重选取最多 `min(replication_factor, nodeCount())` 个物理节点，调用方负责 `alloc.free`）

### 2. SWIM Gossip 成员健康表（`membership.cp`）

- **`MemberState`**：`Alive = 0`、`Suspect = 1`、`Dead = 2`（提供 `severity(): u8` 与 `isAlive(): bool`）
- **`MemberInfo`**：`node_id: string`、`address: string`、`incarnation: u64`、`state: MemberState`、`last_seen_ms: i64`
- **`MembershipTable`**：
  - 生命周期：`init(alloc): MembershipTable`、`deinit(*_)`
  - 更新合并：`upsertSelf(*_, node_id, address, incarnation, now_ms): !`、`mergeUpdate(*_, info): !`、`remove(*_, node_id): bool`
  - 确定性超时推进：`tick(*_, now_ms, suspect_after_ms, dead_after_ms)`
  - 查询统计：`get(node_id): MemberInfo?`、`aliveMembers(alloc): ([]MemberInfo)!`（按 `node_id` 字典序返回，调用方负责 `alloc.free`）、`countByState(state): usize`、`len(): usize`

### 3. 单调 Fencing Token 选主租约（`lease.cp`）

- **`LeaseInfo`**：`holder_id: string`、`fencing_token: u64`、`expires_at_ms: i64`
- **`LeaderLease`**：
  - 生命周期：`init(alloc): LeaderLease`、`deinit(*_)`
  - 租约控制：`tryAcquire(*_, candidate_id, ttl_ms, now_ms): LeaseInfo?`（空闲或已过期时授予租约并严格递增 `fencing_token`）、`renew(*_, holder_id, ttl_ms, now_ms): LeaseInfo?`、`releaseLease(*_, holder_id): bool`（`release(holder_id)` 的 CP 安全别名，避开 `cpc` 对 `.release()` 的无参 ARC 拦截）
  - 状态与防脑裂校验：`currentLeader(now_ms): LeaseInfo?`、`validateToken(token): bool`（仅接受 `token >= current_fencing_token` 且 `current_fencing_token > 0`）、`currentFencingToken(): u64`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    ConsistentHashRing,
    MembershipTable,
    MemberState,
    LeaderLease,
} from "cap:cluster";

pub fn demoCluster(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 一致性哈希分片路由与多副本去重
    let ring = ConsistentHashRing.init(alloc);
    defer ring.deinit();
    ring.addNode("shard-a", 64)?;
    ring.addNode("shard-b", 64)?;
    ring.addNode("shard-c", 64)?;

    const primary = ring.getNode("agent:session:9001").?;
    const replicas = ring.getReplicaNodes(alloc, "agent:session:9001", 2)?;
    defer alloc.free(replicas);
    * = primary;

    // 2. SWIM Gossip 成员表合并与超时探测
    let table = MembershipTable.init(alloc);
    defer table.deinit();
    table.upsertSelf("shard-a", "10.0.1.10:9000", 1, 1000)?;
    table.tick(2000, 500, 1500); // Alive -> Suspect
    * = table.countByState(MemberState.Suspect);

    // 3. 防脑裂选主租约（或通过 cap.cluster 动态挂载访问）
    let lease = cap.cluster.LeaderLease.init(alloc);
    defer lease.deinit();
    const l1 = lease.tryAcquire("shard-a", 500, 1000).?;
    const valid = lease.validateToken(l1.fencing_token); // true
    * = valid;
}
```
