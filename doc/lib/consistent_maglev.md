# `cap:consistent_maglev` — Google Maglev $O(1)$ 一致性哈希查表路由引擎

## 概述

`cap:consistent_maglev`（支持 `import { ... } from "cap:consistent_maglev"` 与 `import cap from "cap"` 动态挂载 `cap.consistent_maglev`）提供纯 CP 实现的工业级 Google Maglev 一致性哈希路由引擎。通过为每个后端节点生成独立置换序列并交错填充素数长度的查找表，在 $O(1)$ 时间复杂度内完成请求路由，同时在后端扩缩容时保持极低的键迁移扰动率，并原生支持按权重比例分配流量。

## 核心类型

- `DEFAULT_TABLE_SIZE: usize = 65537`：默认素数查找表长度。
- `MaglevError`：错误集合，包含 `EmptyBackends`、`InvalidTableSize`、`BackendNotFound`、`DuplicateBackend`、`OutOfMemory`。
- `Backend`：后端节点描述结构 `{ name: []const u8, weight: usize }`。
- `MaglevRouter`：Maglev 一致性哈希路由器核心类，管理后端列表与素数查找表。

## API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `MaglevRouter.init` | `init(alloc: std.mem.Allocator, table_size: usize) -> MaglevError!MaglevRouter` | 初始化路由器，校验 `table_size` 为素数 |
| `MaglevRouter.deinit` | `deinit(&self)` | 释放查找表与全部内部拷贝的后端名称 |
| `MaglevRouter.addBackend` | `addBackend(&self, name: []const u8, weight: usize) -> MaglevError!void` | 添加带权重后端节点并自动重建查找表 |
| `MaglevRouter.removeBackend` | `removeBackend(&self, name: []const u8) -> MaglevError!void` | 移除指定后端节点并重建查找表 |
| `MaglevRouter.rebuild` | `rebuild(&self) -> MaglevError!void` | 手动重建 Maglev 置换查找表 |
| `MaglevRouter.lookup` | `lookup(self, key: []const u8) -> MaglevError![]const u8` | $O(1)$ 查找 `key` 路由到的后端名称 |
| `MaglevRouter.backendCount` | `backendCount(self) -> usize` | 返回当前已注册后端数量 |
| `MaglevRouter.tableSize` | `tableSize(self) -> usize` | 返回查找表素数长度 |

## 示例

```cp
import { MaglevRouter, DEFAULT_TABLE_SIZE } from "cap:consistent_maglev";
import { print } from "cap:io";

pub fn main() -> !void {
    let router = try MaglevRouter.init(std.heap.page_allocator, DEFAULT_TABLE_SIZE);
    defer router.deinit();

    try router.addBackend("node-a", 2);
    try router.addBackend("node-b", 1);

    const target = try router.lookup("user-session-42");
    print("routed to: {s}\n", .{target});
}
```
