# `cap:geo_quadtree` — 纯 CP 2D 空间四叉树索引引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:geo`，导入请使用 `import { ... } from "cap:geo"`。

## 概述

`cap:geo_quadtree`（亦支持 `cap.geo_quadtree` 动态挂载）实现二维平面空间四叉树 `QuadTree(V)`，支持自定义节点容量与深度上限保护、矩形包围盒检索 `queryRect` 及欧氏圆半径检索 `queryRadius`。

## 核心类型与 API

- `QuadError`: `InvalidCapacity`、`InvalidBounds`
- `Rect`: `{ cx, cy, half_w, half_h }`（`contains(x, y)`、`intersects(other)`）
- `QuadTree(V)`: `init(alloc, bounds, capacity)`、`deinit(&self)`、`insert(&self, x, y, value)`、`queryRect(self, alloc, range)`、`queryRadius(self, alloc, cx, cy, radius)`、`len(self)`

## 示例

```cp
import { Rect, QuadTree } from "cap:geo";

let qt = try QuadTree(u32).init(alloc, Rect{ .cx = 0, .cy = 0, .half_w = 100, .half_h = 100 }, 4);
defer qt.deinit();
_ = try qt.insert(10.0, 20.0, 1);
```
