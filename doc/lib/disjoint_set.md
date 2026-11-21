# `cap:disjoint_set` — 纯 CP 并查集 (Union-Find / Disjoint Set Union)

## 概述

`cap:disjoint_set`（亦支持 `cap.disjoint_set` 动态挂载）实现带路径减半压缩 (Path Halving) 与按秩合并 (Union by Rank) 的高效并查集，支持实时连通分量计数与分量大小查询，单次操作均摊复杂度 $O(\alpha(N))$。

## 核心类型与 API

- `DsuError`: `IndexOutOfBounds`
- `DisjointSet.init(alloc, n)` / `deinit(&self)`
- `find(&self, x)` / `unite(&self, a, b) -> !bool` / `connected(&self, a, b) -> !bool`
- `componentSize(&self, x)` / `components(self)` / `len(self)` / `reset(&self)`

## 示例

```cp
import { DisjointSet } from "cap:disjoint_set";

let dsu = try DisjointSet.init(alloc, 10);
defer dsu.deinit();
_ = try dsu.unite(1, 2);
```
