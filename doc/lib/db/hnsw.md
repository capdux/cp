# `cap:hnsw` — HNSW 近似最近邻向量图索引标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 1. 概述

`cap:hnsw`（挂载点 `cap.hnsw`）是纯 CP 实现的工业级 **HNSW (Hierarchical Navigable Small World)** 高维向量近似最近邻（ANN）图索引标准库。
提供多距离度量（L2 平方、余弦距离、负内积）、确定性分层跳跃表图构建、Beam Search 检索、软删除过滤以及紧凑二进制快照持久化能力，全程无全局状态、零内存泄漏。

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `DistanceMetric` | 距离度量枚举：`L2Squared`、`Cosine`、`InnerProduct` |
| `HnswError` | 错误集合：`DimensionMismatch`、`EmptyVector`、`InvalidConfig`、`DuplicateId`、`IdNotFound`、`CorruptSnapshot` |
| `HnswConfig` | 图索引超参：`m`（默认 16）、`m0`（默认 32）、`ef_construction`（默认 100）、`seed` |
| `SearchHit` | 检索命中项：`id: u64`、`distance: f32` |
| `HnswIndex` | 多层 HNSW 向量图索引主体 |

## 3. API 速查

- `computeDistance(metric, a, b): HnswError!f32` — 计算两向量在指定度量下的距离，校验非空与维度一致。
- `HnswIndex.init(alloc, dim, metric, config): !HnswIndex` — 初始化指定维度与度量的索引。
- `index.deinit()` — 释放索引内部图邻接表、向量池与 ID 映射。
- `index.insert(id, vector): !void` — 插入向量并执行分层贪心下降与双向邻居裁剪连边。
- `index.search(alloc, query, top_k, ef_search): ![]SearchHit` — 返回按距离升序排列的 Top-K 命中结果（自动跳过软删除节点）。
- `index.markDeleted(id): HnswError!void` — 软删除指定 ID 节点。
- `index.len(): usize` / `index.totalNodes(): usize` / `index.maxLevel(): usize` — 查询活跃节点数、总节点数与最大层高。
- `index.saveBytes(alloc): ![]u8` — 序列化索引图拓扑与向量数据为紧凑二进制快照。
- `HnswIndex.loadBytes(alloc, bytes): !HnswIndex` — 从二进制快照反序列化恢复索引。

## 4. 示例

```cp
import { HnswIndex, HnswConfig, DistanceMetric } from "cap:db";

let idx = try HnswIndex.init(alloc, 4, DistanceMetric.L2Squared, HnswConfig{});
defer idx.deinit();

const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
const v2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
try idx.insert(101, &v1);
try idx.insert(102, &v2);

const hits = try idx.search(alloc, &v1, 1, 16);
defer alloc.free(hits);
```
