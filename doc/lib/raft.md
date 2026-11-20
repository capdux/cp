# CP 标准库：确定性事件驱动 Raft 共识状态机手册 (`cap:raft` / `cap.raft`)

## 一、 架构总览与设计准则

`cap:raft` 是 CP 语言官方内置的**纯 CP、零网络耦合**确定性事件驱动 Raft 共识核心状态机，专为分布式强一致存储（配合 `cap:kv` / `cap:cluster`）打造：

- **三态确定性流转**：完整实现 `Follower` / `Candidate` / `Leader` 状态机与逻辑时钟 `tick()` 驱动，所有对外 RPC 均通过 `drainOutbound` 输出，便于对接任意异步网络层或内存集群模拟器。
- **选举安全性与日志新旧校验**：`handleRequestVote` 严格校验任期、`voted_for` 幂等性及 Raft §5.4.1 日志新旧比较（`last_log_term > my_last_term || (last_log_term == my_last_term && last_log_index >= my_last_index)`）。
- **日志复制与冲突截断**：`handleAppendEntries` 校验 `prev_log_index` / `prev_log_term` 一致性，支持快速冲突回退定位（`conflict_index`）及冲突后缀自动截断覆盖。
- **Figure 8 安全性提交**：`handleAppendEntriesResp` 仅在统计到当前任期（`log[N].term == current_term`）的多数派 `match_index >= N` 时推进 `commit_index`，杜绝旧任期日志被错误提交。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:raft"` 与 `import cap from "cap"; cap.raft.*` 编译期自动动态挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/raft/raft.cp` | `// Source: cp/lib/raft/raft.cp (Deterministic Event-Driven Raft Consensus State Machine)` | `RaftError`、`RaftRole`、`LogEntry`、RPC 结构、`RaftOutboundMsg` 与 `RaftNode` 状态机 |
| `lib/raft/index.cp` | `// Source: cp/lib/raft/index.cp (Unified Exports for cap:raft)` | `cap:raft` 与 `cap.raft` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与核心类型

- **`RaftError`**：`NotLeader` | `EmptyPeers` | `DuplicateNode` | `InvalidIndex` | `OutOfMemory`
- **`RaftRole`**：`Follower` | `Candidate` | `Leader`
- **`LogEntry`**：`{ term: u64, index: u64, command: string }`（1-based 索引，`0` 表示空哨兵）
- **`RequestVoteReq`** / **`RequestVoteResp`**
- **`AppendEntriesReq`** / **`AppendEntriesResp`**
- **`RaftPayload`**：Tagged Union（`.RequestVote` | `.RequestVoteResp` | `.AppendEntries` | `.AppendEntriesResp`）
- **`RaftOutboundMsg`**：`{ to: u64, from: u64, payload: RaftPayload }`

### 2. `RaftNode` 状态机

- **`RaftNode.init(alloc, node_id, peer_ids, election_timeout_ticks, heartbeat_ticks): RaftNode ! RaftError`**
  - 初始化为 `Follower`（`term = 0`）。若 `peer_ids` 为空返回 `EmptyPeers`；若 `peer_ids` 含重复节点或包含 `node_id` 自身返回 `DuplicateNode`。
- **`deinit(*_)`**：释放日志缓冲区、未排空出站消息及内部切片。
- **`tick(*_): !RaftError`**：推进 1 个逻辑时钟周期；`Follower`/`Candidate` 超时触发选举并广播 `RequestVote`；`Leader` 定期广播 `AppendEntries` 心跳/日志。
- **`propose(*_, command: string): u64 ! RaftError`**：仅 `Leader` 可提案追加新日志并立即排队复制消息，返回分配的 1-based `index`；非 `Leader` 返回 `RaftError.NotLeader`。
- **`handleRequestVote(*_, req: RequestVoteReq): RequestVoteResp`**：处理投票请求（含任期更新与日志新旧校验）。
- **`handleRequestVoteResp(*_, resp: RequestVoteResp): !RaftError`**：处理投票响应；遇更高任期自动退位为 `Follower`；达到 Quorum 多数票立即晋升 `Leader` 并初始化 `next_index` / `match_index`。
- **`handleAppendEntries(*_, req: AppendEntriesReq): AppendEntriesResp ! RaftError`**：处理心跳/日志复制请求，执行前驱一致性校验、冲突后缀截断、追加新条目及 `commit_index` 推进。
- **`handleAppendEntriesResp(*_, from_peer: u64, resp: AppendEntriesResp): !RaftError`**：更新 `match_index` / `next_index`，失败则回退重试；多数派复制当前任期日志后推进 `commit_index`。
- **`drainApplied(*_, alloc): ([]LogEntry) ! RaftError`**：提取 `last_applied < index <= commit_index` 的已提交条目并推进 `last_applied`（调用方使用 `alloc.free` 释放返回切片）。
- **`drainOutbound(*_, alloc): ([]RaftOutboundMsg) ! RaftError`**：提取所有待发送的出站消息；使用 **`RaftNode.freeOutbound(alloc, msgs)`** 统一释放。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { RaftNode, RaftRole } from "cap:raft";

pub fn demoRaft(): ! {
    const alloc = std.heap.page_allocator;
    const peers1: [2]u64 = [2, 3];
    let n1 = RaftNode.init(alloc, 1, &peers1, 5, 2)?;
    defer n1.deinit();

    // 推进时钟触发选举
    for (i of 0..5) {
        * = i;
        n1.tick()?;
    }

    const out = n1.drainOutbound(alloc)?;
    defer RaftNode.freeOutbound(alloc, out);

    // 等价动态挂载访问：
    const role: cap.raft.RaftRole = n1.role;
    * = role;
}
```
