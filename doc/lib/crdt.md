# CP 标准库：分布式无冲突复制数据类型（CRDT）引擎手册 (`cap:crdt` / `cap.crdt`)

## 一、 架构总览与设计准则

`cap:crdt` 是 CP 语言官方内置的纯原生状态型无冲突复制数据类型（State-Based CRDT / CvRDT）标准库，专为多 Agent 分布式最终一致性状态同步、离线协作与无中心冲突收敛打造。全模块严格满足**交换律（Commutativity）、结合律（Associativity）与幂等律（Idempotence）**，在任意网络乱序、重复投递与分区恢复后保证强最终一致性（Strong Eventual Consistency, SEC）。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，所有内部 `node_id`、`elem` 与字符串值均采用所有权安全深拷贝，支持 `import ... from "cap:crdt"` 与 `import cap from "cap"; cap.crdt.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/crdt/vclock.cp` | `// Source: cp/lib/crdt/vclock.cp (Lamport Vector Clock & Causal Ordering)` | `CrdtErr`、`CausalOrder`、`VectorClock` |
| `lib/crdt/counters.cp` | `// Source: cp/lib/crdt/counters.cp (GCounter & PNCounter State-Based CRDTs)` | `GCounter`、`PnCounter` |
| `lib/crdt/register.cp` | `// Source: cp/lib/crdt/register.cp (LWW-Register & Observed-Remove Set CRDTs)` | `LwwRegister`、`OrSetTag`、`OrSet` |
| `lib/crdt/index.cp` | `// Source: cp/lib/crdt/index.cp (Unified Exports for cap:crdt)` | `cap:crdt` 与 `cap.crdt` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. Lamport 向量时钟与因果偏序（`vclock.cp`）

- **`CrdtErr`**：`OutOfMemory`、`InvalidParameter`、`Overflow`
- **`CausalOrder`**：`Equal`、`Before`、`After`、`Concurrent`（提供 `isEqual()`、`isBefore()`、`isAfter()`、`isConcurrent()` 辅助判定）
- **`VectorClock`**：
  - 生命周期：`init(alloc): VectorClock`、`deinit(*_)`、`clone(alloc): !VectorClock`
  - 时钟推进：`increment(*_, node_id): !`（拒绝空 `node_id`）、`incrementBy(*_, node_id, delta): !`
  - 查询与统计：`get(node_id): u64`（未记录节点返回 `0`）、`len(): usize`
  - 合并与因果判定：`merge(*_, other): !`（逐节点取 `max`）、`compare(other): CausalOrder`

### 2. 分布式计数器（`counters.cp`）

- **`GCounter`**（只增分布式计数器）：
  - 生命周期：`init(alloc): GCounter`、`deinit(*_)`、`clone(alloc): !GCounter`
  - 操作与查询：`increment(*_, node_id, delta): !`、`getNode(node_id): u64`、`value(): u64`、`eql(other): bool`
  - 合并：`merge(*_, other): !`（逐节点取 `max`）
- **`PnCounter`**（可增可减分布式计数器，内置 `pos` + `neg` 双 `GCounter`）：
  - 生命周期：`init(alloc): PnCounter`、`deinit(*_)`、`clone(alloc): !PnCounter`
  - 操作与查询：`increment(*_, node_id, delta): !`、`decrement(*_, node_id, delta): !`、`value(): i64`（内部 `i128` 差值并安全夹紧至 `i64`）、`eql(other): bool`
  - 合并：`merge(*_, other): !`

### 3. LWW 寄存器与 Observed-Remove 集合（`register.cp`）

- **`LwwRegister`**（Last-Write-Wins 字符串寄存器）：
  - 生命周期：`init(alloc): LwwRegister`、`deinit(*_)`、`clone(alloc): !LwwRegister`
  - 读写：`set(*_, val, ts, node_id): !`（仅当未写入、`ts` 更大、或同 `ts` 时 `node_id` 字典序 `>=` 当前值时更新）、`get(): string?`、`timestamp(): i64`、`writerNode(): string?`
  - 合并决胜：`merge(*_, other): !`（按 `(timestamp_ms, node_id)` 确定性决胜；同 `(ts, node_id)` 时按 `val` 字典序保底，严格保证交换律与幂等律）
- **`OrSet`**（Observed-Remove Set，支持并发 add-wins）：
  - 生命周期：`init(alloc): OrSet`、`deinit(*_)`、`clone(alloc): !OrSet`
  - 增删：`add(*_, elem, node_id): !`（每次生成唯一 `(node_id, seq)` 标签）、`remove(*_, elem): bool`（仅对本地当前已观测到的标签置墓碑，绝不影响并发新增标签）
  - 查询与导出：`contains(elem): bool`、`len(): usize`、`elements(alloc): ([]string)!`（按字典序返回全部活跃元素切片，调用方负责 `alloc.free`）
  - 合并：`merge(*_, other): !`（标签集并集 + 墓碑单调传播）

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    CausalOrder,
    VectorClock,
    GCounter,
    PnCounter,
    LwwRegister,
    OrSet,
} from "cap:crdt";

pub fn demoCrdt(): !void {
    const alloc = std.heap.page_allocator;

    // 1. VectorClock 因果偏序与合并
    let vc1 = VectorClock.init(alloc);
    defer vc1.deinit();
    let vc2 = VectorClock.init(alloc);
    defer vc2.deinit();

    vc1.increment("agent-a")?;
    vc2.increment("agent-b")?;
    // vc1 与 vc2 互不包含 => Concurrent
    const order = vc1.compare(vc2);
    * = order == CausalOrder.Concurrent;

    vc1.merge(vc2)?;
    * = vc1.get("agent-b"); // 1

    // 2. PnCounter 多副本增减收敛
    let pn = PnCounter.init(alloc);
    defer pn.deinit();
    pn.increment("agent-a", 10)?;
    pn.decrement("agent-b", 3)?;
    * = pn.value(); // 7

    // 3. LwwRegister 与 OrSet（支持 cap.crdt 动态挂载）
    let reg = cap.crdt.LwwRegister.init(alloc);
    defer reg.deinit();
    reg.set("ready", 1000, "agent-a")?;

    let tags = cap.crdt.OrSet.init(alloc);
    defer tags.deinit();
    tags.add("urgent", "agent-a")?;
    * = tags.contains("urgent");
}
```
