# `cap:raft_log` — Raft 共识日志引擎

## 概述

`cap:raft_log` 提供纯 CP 实现的工业级 Raft 共识日志引擎，支持 1-based 日志追加、冲突分叉自动截断覆盖、状态机 `commitIndex`/`appliedIndex` 推进与非法回退拦截、前缀快照压缩（`compactTo`）以及选举约束比较（`isUpToDate`）。支持通过 `import { ... } from "cap:raft_log"` 或 `import cap from "cap"`（`cap.raft_log`）使用。

## 核心类型

- **`RaftLogError`**：错误集，包含 `CompactedIndex`、`OutOfBounds`、`ConflictTermMismatch`、`InvalidCommitIndex`。
- **`LogEntry`**：日志条目 `{ index: u64, term: u64, data: []u8 }`，内部持有堆上深拷贝的 `data` 切片。
- **`LogEntryInput`**：批量追加输入 `{ term: u64, data: []const u8 }`。
- **`RaftLog`**：核心 Raft 日志状态机类，管理未压缩日志切片、`snapshot_index`/`snapshot_term`、`commit_index` 与 `applied_index`。

## API 速查

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `init(alloc: std.mem.Allocator) -> RaftLog` | 初始化空日志实例 |
| `deinit` | `deinit(&self)` | 释放全部日志条目负载与内部数组 |
| `firstIndex` | `firstIndex(self) -> u64` | 返回首个未压缩逻辑索引（`snapshot_index + 1`） |
| `lastIndex` | `lastIndex(self) -> u64` | 返回最新日志索引（无日志时返回 `snapshot_index`） |
| `lastTerm` | `lastTerm(self) -> u64` | 返回最新日志任期（无日志时返回 `snapshot_term`） |
| `termAt` | `termAt(self, index: u64) -> RaftLogError!u64` | 查询指定索引的任期 |
| `entryAt` | `entryAt(self, index: u64) -> RaftLogError!LogEntry` | 查询指定索引的日志条目 |
| `append` | `append(&self, term: u64, data: []const u8) -> !u64` | 追加单条日志（深拷贝负载）并返回新索引 |
| `appendIfMatch` | `appendIfMatch(&self, prev_index: u64, prev_term: u64, new_entries: []const LogEntryInput) -> !bool` | 一致性检查，匹配后追加并在任期冲突时自动截断覆盖 |
| `commitTo` | `commitTo(&self, index: u64) -> RaftLogError!void` | 单调推进 `commit_index` |
| `appliedTo` | `appliedTo(&self, index: u64) -> RaftLogError!void` | 单调推进 `applied_index`（不得超过 `commit_index`） |
| `compactTo` | `compactTo(&self, index: u64) -> RaftLogError!void` | 将 `[firstIndex, index]` 截断压缩为快照并保留元数据 |
| `isUpToDate` | `isUpToDate(self, candidate_last_term: u64, candidate_last_index: u64) -> bool` | 依据 Raft §5.4.1 规则比较候选人日志是否至少与本地一样新 |

## 示例

```cp
import { RaftLog, LogEntryInput } from "cap:raft_log";
const std = @import("std");

pub fn main() -> !void {
    let log = RaftLog.init(std.heap.page_allocator);
    defer log.deinit();

    const idx1 = try log.append(1, "set x=1");
    const idx2 = try log.append(1, "set y=2");
    try log.commitTo(idx2);
    try log.compactTo(idx1);
}
```
