# CP 标准库：纯 CP B+ 树有序键值与范围扫描引擎 (`cap:bplus`)

## 一、 架构总览与设计准则

`cap:bplus` 是 CP 语言官方内置的纯 CP **B+ 树（B+ Tree）有序键值与叶子链表范围扫描引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。采用索引池（`ArrayList(BPlusNode)` + `free_slots` 回收槽）管理树节点，所有数据键值对集中存储于双向链表串联的叶子节点中，内部节点仅存储路标键与子节点索引，实现 $O(\log N)$ 点查/插入/删除与极速顺序范围扫描（`rangeScan`、`prefixScan`、`lowerBound`）。

既可通过 `import { ... } from "cap:bplus"` 导入，也支持 `import cap from "cap"; cap.bplus.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bplus/bplus.cp` | `// Source: cp/lib/bplus/bplus.cp (...)` | `BPlusError`、`BPlusEntry`、`BPlusTree` |
| `lib/bplus/index.cp` | `// Source: cp/lib/bplus/index.cp (...)` | `cap:bplus` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与条目类型

- **`BPlusError`**：
  - `InvalidOrder`：树阶数 `order < 3`。
  - `InvalidRange`：`rangeScan` 传入 `start_key > end_key`。
  - `OutOfMemory`：底层节点池、字符串拷贝或扫描输出切片分配失败。
- **`BPlusEntry`**：`{ key: string, value: string }`（只读键值视图，生命周期与树内对应键未删除前一致）。

### 2. `BPlusTree` 核心方法

- **生命周期**：
  - `BPlusTree.init(alloc: std.mem.Allocator, order: usize): BPlusTree ! BPlusError`：初始化空 B+ 树，要求 `order >= 3`（否则返回 `BPlusError.InvalidOrder`）。
  - `deinit(*_)`：释放全部节点、内部拷贝的键值字符串及节点池内存。
- **写入与删除**：
  - `put(*_, key: string, value: string): bool ! BPlusError`：插入或覆盖更新键值对。若键已存在则覆盖值并返回 `false`；新建返回 `true`。叶子满（`> order - 1`）时自动向父节点递归/迭代分裂，必要时生成新根增加树高。
  - `remove(*_, key: string): bool`：删除指定键。触发下溢时自动尝试向左右兄弟节点借位或与兄弟合并，并向上传播；若根节点仅剩单个子节点则自动收缩降低树高。返回该键是否存在并被删除。
- **点查与定位**：
  - `get(key: string): ([]const u8)?`：$O(\log N)$ 精确点查。
  - `lowerBound(key: string): BPlusEntry?`：$O(\log N)$ 查找第一个满足 `entry.key >= key` 的条目。
  - `minEntry(): BPlusEntry?` / `maxEntry(): BPlusEntry?`：$O(1)$ 通过 `head_leaf` / `tail_leaf` 返回最小/最大条目。
- **范围与前缀扫描**：
  - `rangeScan(alloc, start_key: string, end_key: string, include_end: bool): []BPlusEntry ! BPlusError`：二分定位 `start_key` 所在叶子后沿 `next_leaf` 顺序扫出 `[start_key, end_key]`（`include_end == true`）或 `[start_key, end_key)`（`include_end == false`）的严格升序切片。若 `start_key > end_key` 返回 `BPlusError.InvalidRange`。调用方使用 `alloc.free(entries)` 释放返回切片。
  - `prefixScan(alloc, prefix: string): []BPlusEntry ! BPlusError`：利用 `prefix` 二分定位起点并沿叶子链表顺序扫描，遇到非 `prefix` 前缀键立即提前终止。调用方使用 `alloc.free(entries)` 释放返回切片。
- **状态与不变量自检**：
  - `len(): usize`：返回当前键值对总数。
  - `height(): usize`：返回当前树高（空树为 `0`）。
  - `verifyInvariants(): bool`：全面校验所有叶子同层等深、节点键数合法性、内部路标键严格分区约束及叶子双向链表严格单调递增与计数一致性。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { BPlusTree } from "cap:bplus";
import cap from "cap";

pub fn demoBPlus(): ! {
    const alloc = std.heap.page_allocator;

    let tree = BPlusTree.init(alloc, 4)?;
    defer tree.deinit();

    * = tree.put("user:01", "alice")?;
    * = tree.put("user:02", "bob")?;
    * = tree.put("user:03", "carol")?;
    * = tree.put("order:01", "o1")?;

    const users = tree.prefixScan(alloc, "user:")?;
    defer alloc.free(users);

    const range = tree.rangeScan(alloc, "user:01", "user:03", false)?;
    defer alloc.free(range); // 包含 user:01, user:02

    let dyn_tree = cap.bplus.BPlusTree.init(alloc, 6)?;
    defer dyn_tree.deinit();
    * = dyn_tree.put("k1", "v1")?;
}
```
