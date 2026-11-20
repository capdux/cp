# `cap:gossip` — Sans-IO SWIM 分布式集群成员发现与节点健康探测标准库

## 概述

`cap:gossip`（动态挂载 `cap.gossip`）是基于 SWIM（Scalable Weakly-consistent Infection-style Process Group Membership Protocol）协议实现的纯 CP Sans-IO 集群成员管理与故障探测引擎。

核心特性：
- **Sans-IO 架构**：所有状态机推进由 `tick(now_ms)` 与 `handleMessage(from_id, msg, now_ms)` 驱动，不绑定具体网络传输层，支持 UDP/TCP/内存模拟路由。
- **Incarnation 冲突消解**：严格遵循更高 `incarnation` 胜出、同 `incarnation` 下 `Left > Offline > Suspect > Active` 的优先级规则；支持本地节点收到误判 `Suspect`/`Offline` 时的自动递增 `incarnation` 声明恢复（Refutation）。
- **Piggyback 感染式传播**：状态变更随 `Ping`/`Ack`/`PingReq`/`Sync` 报文附带传播，并在达到 `lambda * ceil(log2(N + 1))` 次转发后自动出队。
- **紧凑二进制 Wire 编解码**：内置 `encodeMessage` / `decodeMessage`，带 Magic/Version 校验与严格边界检查。

## 核心类型

| 类型 | 说明 |
| --- | --- |
| `MemberState` | 节点状态枚举：`Active`、`Suspect`、`Offline`、`Left` |
| `MemberInfo` | 成员信息：`id`, `addr`, `state`, `incarnation`, `state_changed_ms` |
| `GossipUpdate` | Piggyback 状态更新条目：`node_id`, `addr`, `state`, `incarnation` |
| `SwimConfig` | 协议配置：`protocol_period_ms`, `ack_timeout_ms`, `suspect_timeout_ms`, `indirect_ping_count`, `max_piggyback`, `retransmit_mult` |
| `MessageKind` | 协议消息类型：`Ping`, `Ack`, `PingReq`, `Sync` |
| `GossipMessage` | 完整协议消息体 |
| `OutboundMessage` | Sans-IO 输出待发送消息：`(target_id, target_addr, msg)` |
| `SwimClusterNode` | SWIM 集群成员状态机节点 |
| `GossipError` | 错误集合：`OutOfMemory`, `InvalidMessage`, `TruncatedMessage`, `UnknownTag`, `InvalidNodeId`, `BufferTooSmall` |

## API 速查

- `SwimClusterNode.init(alloc, self_id, addr, config) -> GossipError!SwimClusterNode`
- `node.deinit()`
- `node.joinSeed(peer_id, addr, now_ms) -> GossipError![]OutboundMessage`
- `node.leave(now_ms) -> GossipError!void`
- `node.tick(now_ms) -> GossipError![]OutboundMessage`
- `node.handleMessage(from_id, msg, now_ms) -> GossipError![]OutboundMessage`
- `node.activeMembers(alloc) -> GossipError![]MemberInfo`
- `node.getMember(id) -> MemberInfo?`
- `node.memberCount() -> usize`
- `node.selfIncarnation() -> u64`
- `freeOutbound(alloc, list)`
- `encodeMessage(alloc, msg) -> GossipError![]u8`
- `decodeMessage(alloc, data) -> GossipError!GossipMessage`
- `freeMessage(alloc, *msg)`
- `shouldSupersede(existing_state, existing_inc, new_state, new_inc) -> bool`

## 示例

```cp
import { SwimClusterNode, SwimConfig, freeOutbound } from "cap:gossip";
const std = @import("std");

let node = try SwimClusterNode.init(std.heap.page_allocator, "node-1", "10.0.0.1:7946", SwimConfig.default());
defer node.deinit();

const out = try node.joinSeed("node-0", "10.0.0.0:7946", 100);
freeOutbound(std.heap.page_allocator, out);
```
