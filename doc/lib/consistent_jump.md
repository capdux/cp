# `cap:consistent_jump` — 纯 CP 零内存 Jump Consistent Hash 与 HRW 路由引擎

## 概述

`cap:consistent_jump`（亦支持 `cap.consistent_jump` 动态挂载）实现 Google Jump Consistent Hash（零内存占用、$O(\ln N)$ 分片，扩容仅迁移 $1/(N+1)$ 键）与无状态 Rendezvous / Highest Random Weight (HRW) 等权/加权节点路由。

## 核心类型与 API

- `JumpError`: `InvalidBucketCount`、`EmptyNodes`
- `WeightedNode`: `{ id: []const u8, weight: f64 }`
- `jumpConsistentHash(key: u64, num_buckets: u32) -> JumpError!u32`
- `jumpConsistentHashBytes(key_bytes, num_buckets) -> JumpError!u32`
- `hrwSelect(key_bytes, nodes) -> JumpError![]const u8`
- `hrwSelectWeighted(key_bytes, nodes) -> JumpError![]const u8`

## 示例

```cp
import { jumpConsistentHash, hrwSelect } from "cap:consistent_jump";

const shard = try jumpConsistentHash(123456789, 16);
```
