# CP 标准库：纯 CP 压缩基数树与前缀路由引擎 (`cap:radix`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 一、 架构总览与设计准则

`cap:radix` 是 CP 语言官方内置的纯 CP **压缩基数树（Compressed Patricia Radix Tree）与分段前缀路由引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。

- **`RadixTree<T>`**：每个内部/叶子节点仅存储共享边前缀 `prefix: []u8`，插入时自动执行公共前缀分裂（Split），删除后自动执行孤立单子节点合并压缩（Merge），保持树高度最简；提供 `longestPrefix`（CIDR/规则最长前缀匹配）与 `scanPrefix`（字典序前缀扫描）。
- **`RadixRouter<T>`**：基于分段基数树的路径路由器，支持静态段、`:param` 命名参数与 `*wildcard` 通配符参数提取，按 `static > :param > *wildcard` 优先级匹配。

既可通过 `import { ... } from "cap:ds"` 导入，也支持 `import cap from "cap"; cap.radix.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/radix/radix.cp` | `// Source: cp/lib/radix/radix.cp (...)` | `RadixError`、`RadixMatch<T>`、`RadixEntry<T>`、`RadixTree<T>` |
| `lib/radix/router.cp` | `// Source: cp/lib/radix/router.cp (...)` | `RouteParam`、`RouteMatch<T>`、`RadixRouter<T>` |
| `lib/radix/index.cp` | `// Source: cp/lib/radix/index.cp (...)` | `cap:radix` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与结果结构

- **`RadixError`**：
  - `KeyNotFound`：键不存在。
  - `DuplicateKey`：`insert` 或 `addRoute` 时键/路由已存在。
  - `OutOfMemory`：节点、边前缀或结果切片分配失败。
- **`RadixMatch<T>`**：`{ key: []const u8, value: T }`（`longestPrefix` 返回结果）。
- **`RadixEntry<T>`**：`{ key: []const u8, value: T }`（`scanPrefix` 返回条目）。
- **`RouteParam`**：`{ name: []const u8, value: []const u8 }`。
- **`RouteMatch<T>`**：`{ value: T, params: []RouteParam }`，提供 `getParam(name): ([]const u8)?` 与 `deinit()`。

### 2. `RadixTree<T>` 核心方法

- `RadixTree<T>.init(allocator: std.mem.Allocator): RadixTree<T>`：初始化空压缩基数树（延迟分配，零初始开销）。
- `deinit(*_)` / `clear(*_)`：释放所有节点、压缩边前缀及存储键内存。
- `insert(*_, key: []const u8, value: T): void ! RadixError`：插入新键值对；若键已存在则返回 `RadixError.DuplicateKey`。
- `upsert(*_, key: []const u8, value: T): void ! RadixError`：插入或覆盖更新已有键的值。
- `get(key: []const u8): T?` / `contains(key: []const u8): bool`：$O(k)$ 精确查找。
- `delete(*_, key: []const u8): T?`：删除键并返回被删除的值；若产生孤立非终端单子节点则自动与子节点合并压缩。
- `longestPrefix(key: []const u8): RadixMatch<T>?`：查找树中作为 `key` 前缀的最长已存储键。
- `scanPrefix(allocator: std.mem.Allocator, prefix: []const u8): []RadixEntry<T> ! RadixError`：按字典序返回所有以 `prefix` 开头的键值对（调用方使用 `allocator.free(entries)` 释放返回切片）。
- `count(): usize` / `isEmpty(): bool` / `nodeCount(): usize` / `verifyInvariants(): bool`。

### 3. `RadixRouter<T>` 核心方法

- `RadixRouter<T>.init(allocator: std.mem.Allocator): RadixRouter<T>` / `deinit(*_)` / `clear(*_)`。
- `addRoute(*_, pattern: []const u8, value: T): void ! RadixError`：注册静态、`:param` 或 `*wildcard` 路由模式。
- `matchRoute(allocator: std.mem.Allocator, path: []const u8): RouteMatch<T>? ! RadixError`：匹配路径并提取参数（非空时调用方需调用 `match.deinit()`）。
- `count(): usize` / `isEmpty(): bool`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { RadixTree, RadixRouter } from "cap:ds";
import cap from "cap";

pub fn demoRadix(): ! {
    const alloc = std.heap.page_allocator;

    let tree = RadixTree<u32>.init(alloc);
    defer tree.deinit();

    tree.insert("192.168.", 10)?;
    tree.insert("192.168.1.", 20)?;
    const m = tree.longestPrefix("192.168.1.42").?; // 匹配 "192.168.1.", value = 20

    const entries = tree.scanPrefix(alloc, "192.168.")?;
    defer alloc.free(entries);

    let router = cap.radix.RadixRouter<u32>.init(alloc);
    defer router.deinit();
    router.addRoute("/users/:id/posts", 101)?;
    if (rm := router.matchRoute(alloc, "/users/42/posts")?) {
        defer rm.deinit();
        const uid = rm.getParam("id").?; // "42"
    }
}
```
