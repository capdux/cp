# `cap:redis_cluster` — Redis / Valkey Cluster 槽位路由与拓扑管理

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 1. 概述

`lib/redis_cluster/`（对外协议 `cap:redis_cluster` / `cap.redis_cluster`）是纯 CP 实现的工业级 Redis / Valkey Cluster 客户端核心路由库，提供：
- **CRC16-CCITT (XMODEM `0x1021`)** 校验与 **16384 槽位**计算；
- **Hash Tag (`{...}`)** 精确提取，支持多 Key 事务与管道同槽聚合；
- **O(1) 槽位路由表 (`ClusterTopology`)**，支持 `CLUSTER SLOTS` 区间构建、全覆盖检查与 `MOVED` 实时热更新；
- **`MOVED` / `ASK` / `CLUSTERDOWN` / `TRYAGAIN` / `CROSSSLOT`** 重定向与集群错误解析（支持 IPv4 与 IPv6 `[::1]:port`）。

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `ClusterError` | 错误集合：`InvalidSlotRange`、`InvalidSlot`、`SlotNotCovered`、`InvalidRedirect`、`CrossSlot`、`ClusterDown`、`TryAgain`、`OutOfMemory` |
| `NodeEndpoint` | 集群节点地址 `{ host: []const u8, port: u16 }`，提供 `init` 与 `equals` |
| `SlotRange` | 槽位区间描述 `{ start: u16, end: u16, master: NodeEndpoint, replicas: []const NodeEndpoint }` |
| `RedirectKind` | 重定向/错误类型：`Moved`、`Ask`、`ClusterDown`、`TryAgain`、`CrossSlot` |
| `ClusterRedirect` | 重定向解析结果 `{ kind: RedirectKind, slot: u16, endpoint: NodeEndpoint? }` |
| `ClusterTopology` | 16384 槽位 O(1) 路由表，内置节点地址去重与内存生命周期管理 |

## 3. API 速查

- `REDIS_CLUSTER_SLOTS: u16` — 常量 `16384`。
- `crc16Ccitt(data: []const u8): u16` — 计算 CRC16-CCITT (XMODEM `0x1021`)，满足 `crc16Ccitt("123456789") == 0x31C3`。
- `extractHashTag(key: []const u8): []const u8` — 提取首个非空 `{...}` 内部字节；无有效标签时返回原 `key`。
- `keySlot(key: []const u8): u16` — 返回 `[0, 16383]` 槽位号。
- `ensureSameSlot(keys: []const []const u8): !u16` — 校验多 Key 是否同槽，跨槽返回 `error.CrossSlot`。
- `ClusterTopology.init(alloc)` / `deinit()` — 初始化与释放拓扑路由表。
- `ClusterTopology.addSlotRange(start, end, master, replicas): ClusterError!void` — 批量写入槽位区间映射。
- `ClusterTopology.routeSlot(slot): ClusterError!NodeEndpoint` / `routeKey(key): ClusterError!NodeEndpoint` — O(1) 路由查询。
- `ClusterTopology.applyMovedRedirect(slot, host, port): ClusterError!void` — 收到 `MOVED` 后热更新单槽位归属。
- `ClusterTopology.isFullCoverage(): bool` / `coveredSlots(): u32` — 检查 16384 槽位是否全部覆盖。
- `parseRedirectError(err_msg: []const u8): ClusterError!ClusterRedirect` — 解析 `MOVED`/`ASK`（含 IPv6 `[::1]:7002`）及集群状态错误。

## 4. 示例

```cp
import {
    keySlot,
    ensureSameSlot,
    NodeEndpoint,
    ClusterTopology,
    parseRedirectError,
} from "cap:db";

const slot = keySlot("user:{42}:profile");
const keys: [2][]const u8 = [ "user:{42}:profile", "user:{42}:settings" ];
const batch_slot = try ensureSameSlot(&keys);
* = slot;
* = batch_slot;

const empty_replicas: [0]NodeEndpoint = [];
let topo = ClusterTopology.init(alloc);
defer topo.deinit();
try topo.addSlotRange(0, 16383, NodeEndpoint.init("127.0.0.1", 7001), &empty_replicas);
const target = try topo.routeKey("user:{42}:profile");
const redir = try parseRedirectError("MOVED 3999 [::1]:7002");
try topo.applyMovedRedirect(redir.slot, redir.endpoint.?.host, redir.endpoint.?.port);
```
