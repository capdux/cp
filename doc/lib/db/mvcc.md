# `cap:mvcc` — 多版本并发控制与快照隔离标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 1. 概述

`cap:mvcc`（`lib/mvcc/`）为 CP 语言提供工业级纯内存 MVCC（Multi-Version Concurrency Control）事务引擎，基于单调递增时间戳 Oracle 与版本链实现完整的 **Snapshot Isolation（快照隔离）**：

- **无锁快照读（Snapshot Read）**：事务启动时分配固定 `read_ts`，整个事务生命周期内可重复读取 `commit_ts <= read_ts` 的最新已提交版本，完全隔离并发未提交修改。
- **Read-Your-Own-Writes**：事务读取优先命中本地写缓冲 `write_set`，立即感知自身未提交的 `put` / `delete`。
- **First-Committer-Wins 冲突检测**：提交阶段校验 `write_set` 中所有键是否存在 `latest_commit_ts > read_ts` 的并发提交；若冲突则返回 `MvccError.WriteConflict` 并自动回滚。
- **安全历史版本垃圾回收（GC）**：通过 `minActiveReadTs()` 计算活跃事务最小快照水位，配合 `gcVersions(safe_watermark_ts)` 安全清理已被覆盖的历史版本并保留基准版本。

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `MvccError` | 错误集合：`WriteConflict`、`TxnNotActive`、`TxnAborted`、`ReadOnlyTxn`、`KeyNotFound`、`OutOfMemory` |
| `TxnState` | 事务状态枚举：`Active`、`Committed`、`Aborted` |
| `VersionValue` | 版本链节点：`commit_ts: u64`、`txn_id: u64`、`value: ([]const u8)?`（`null` 表示墓碑删除） |
| `WriteEntry` | 事务本地写缓冲条目：`key: []const u8`、`value: ([]const u8)?` |
| `MvccStore` | MVCC 存储引擎与时间戳 Oracle，维护各键降序版本链与活跃事务列表 |
| `MvccTxn` | 事务句柄，提供隔离读写、冲突校验提交与显式/自动回滚 |

## 3. API 速查

### `MvccStore`

- `MvccStore.init(alloc: std.mem.Allocator): MvccStore` — 初始化 MVCC 存储实例。
- `store.deinit()` — 释放所有键、版本链节点及活跃事务表内存。
- `store.beginTxn(read_only: bool): MvccError!MvccTxn` — 开启事务，分配唯一 `txn_id` 与当前 `read_ts`。
- `store.minActiveReadTs(): u64` — 获取当前所有活跃事务的最小 `read_ts`（无活跃事务时返回 `current_ts`）。
- `store.gcVersions(safe_watermark_ts: u64): usize` — 清理 `commit_ts < safe_watermark_ts` 的被覆盖旧版本（保留每个键 `<= safe_watermark_ts` 的最新基准版本），返回回收版本数。
- `store.versionCount(key: []const u8): usize` — 查询指定键当前保留的版本链长度。

### `MvccTxn`

- `txn.get(key: []const u8): ([]const u8)?` — 优先读取本地 `write_set`，否则扫描版本链返回 `commit_ts <= read_ts` 的最新已提交值（遇墓碑或未找到返回 `null`）。
- `txn.put(key: []const u8, val: []const u8): MvccError!void` — 写入本地写集；只读事务返回 `MvccError.ReadOnlyTxn`。
- `txn.delete(key: []const u8): MvccError!void` — 写入删除墓碑（`null`）；只读事务返回 `MvccError.ReadOnlyTxn`。
- `txn.commit(): MvccError!u64` — 执行 First-Committer-Wins 冲突检测，无冲突则分配 `commit_ts` 并写入版本链；冲突则自动回滚并返回 `MvccError.WriteConflict`。
- `txn.rollback()` — 显式回滚事务，释放本地 `write_set` 并标记为 `Aborted`。
- `txn.deinit()` — 清理事务资源；若仍处于 `Active` 状态则自动注销活跃记录。

## 4. 示例

```cp
import { MvccStore } from "cap:db";
const std = @import("std");

let store = MvccStore.init(std.heap.page_allocator);
defer store.deinit();

let t1 = try store.beginTxn(false);
defer t1.deinit();
try t1.put("account:1", "100");
const ts1 = try t1.commit();
```
