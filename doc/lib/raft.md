# `cap:raft` — Sans-IO Raft 分布式共识状态机与日志复制引擎

## 概述

`cap:raft`（同步挂载为 `cap.raft`）提供纯 CP 实现的工业级 Sans-IO Raft 共识状态机与日志复制引擎。
节点与网络、定时器、线程及磁盘 I/O 完全解耦：调用方通过 `tick()`、`propose()` 及 RPC 处理函数驱动状态机，状态机返回确定性的 `RaftAction` 动作列表（发送投票请求、发送日志复制、持久化任期与投票状态、应用已提交日志条目）。

## 核心类型

- **`RaftError`**：`NotLeader`、`InvalidIndex`、`CompactedIndex`、`TermMismatch`、`OutOfMemory`
- **`Role`**：`Follower` | `Candidate` | `Leader`
- **`LogEntry`**：`{ term: u64, index: u64, command: []const u8 }`
- **`RequestVoteReq` / `RequestVoteResp`**：候选人选举投票请求与响应
- **`AppendEntriesReq` / `AppendEntriesResp`**：心跳与日志复制请求及带冲突加速回退（`conflict_index` / `conflict_term`）的响应
- **`RaftAction`**：`SendRequestVote` | `SendAppendEntries` | `PersistState` | `ApplyEntry`
- **`RaftLog`**：支持快照基线截断（`compactTo`）与冲突回滚（`truncateFrom`）的复制日志
- **`RaftNode`**：核心 Sans-IO Raft 共识状态机

## API 速查

### `RaftLog`
- `RaftLog.init(allocator)` / `deinit()`
- `lastIndex(): u64` / `lastTerm(): u64`
- `termAt(index: u64): u64?` / `matchTerm(index: u64, term: u64): bool`
- `entryAt(index: u64): LogEntry?`
- `append(term: u64, command: []const u8): !u64`
- `truncateFrom(from_index: u64)`
- `compactTo(snap_index: u64, snap_term: u64): !void`
- `sliceFrom(allocator, from_index: u64): ![]LogEntry`

### `RaftNode`
- `RaftNode.init(allocator, config: RaftConfig): !RaftNode` / `deinit()`
- `tick(): ![]RaftAction`：推进选举或心跳定时器
- `propose(command: []const u8): ![]RaftAction`：Leader 追加新日志并广播复制（非 Leader 返回 `RaftError.NotLeader`）
- `handleRequestVote(req: RequestVoteReq): !VoteResult`
- `handleRequestVoteResp(resp: RequestVoteResp): ![]RaftAction`
- `handleAppendEntries(req: AppendEntriesReq): !AppendResult`
- `handleAppendEntriesResp(resp: AppendEntriesResp): ![]RaftAction`
- `deinitActions(allocator, actions: []RaftAction)`：释放动作列表持有的内存

## 示例

```cp
import { RaftNode, RaftConfig, deinitActions } from "cap:raft";

const peers = [_]u64{ 2, 3 };
let node = try RaftNode.init(allocator, RaftConfig{ .id = 1, .peers = &peers });
defer node.deinit();

const actions = try node.tick();
defer deinitActions(allocator, actions);
```
