# `cap:consistent_hash` — 纯 CP 虚拟节点一致性哈希环引擎

## 概述

`cap:consistent_hash`（亦支持 `cap.consistent_hash` 动态挂载）实现基于 Wyhash + 有序虚拟节点环的一致性哈希路由，支持动态增删物理节点、最小键迁移扰动以及顺时针去重 N 副本 (`getReplicas`) 选择。

## 核心类型与 API

- `ConsistentHashError`: `EmptyRing`、`InvalidVirtualNodes`、`NodeNotFound`
- `HashRing.init(alloc, vnodes_per_node)` / `deinit(&self)`
- `addNode(&self, node_id)` / `removeNode(&self, node_id)` / `nodeCount(self)`
- `getNode(self, key) -> ![]const u8`
- `getReplicas(self, alloc, key, count) -> ![]const []const u8`

## 示例

```cp
import { HashRing } from "cap:consistent_hash";

let ring = try HashRing.init(alloc, 128);
defer ring.deinit();
try ring.addNode("cache-1");
try ring.addNode("cache-2");
const owner = try ring.getNode("session:42");
```
