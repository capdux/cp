# `cap:bimap` — 双向一对一映射容器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 1. 概述

`cap:bimap` 提供工业级双向一对一映射（Bidirectional 1-to-1 Map）容器，支持通过左侧键（Left）或右侧值（Right）在 O(1) 平均时间复杂度内双向查找、删除与唯一性校验。模块同时支持 `import { ... } from "cap:ds"` 静态导入与 `cap.bimap` 动态挂载。

## 2. 核心类型

- **`BiMapError`**：唯一性冲突错误集，包含 `DuplicateLeft` 与 `DuplicateRight`。
- **`BiMap(comptime L: type, comptime R: type)`**：基于双 `std.AutoHashMap` 实现的泛型双向映射，适用于整型、枚举、指针等值类型键。
- **`StringBiMap`**：面向 `[]const u8 <-> []const u8` 的专用双向字符串映射，内部自动执行 `dupe` 深拷贝并在删除、清空与 `deinit` 时自动释放全部堆内存，保证零泄漏。

## 3. API 速查

| 方法 | 说明 |
| :--- | :--- |
| `init(alloc)` | 使用指定分配器初始化双向映射 |
| `deinit(&self)` | 释放底层哈希表（`StringBiMap` 同步释放深拷贝字符串） |
| `insert(&self, left, right)` | 插入一对映射；若 `left` 或 `right` 已存在则返回 `DuplicateLeft` / `DuplicateRight` |
| `forceInsert(&self, left, right)` | 强制插入并自动清理与 `left` 或 `right` 关联的旧双向映射 |
| `getRight(self, left)` | 根据左侧键 O(1) 查找右侧值（返回 `R?` / `([]const u8)?`） |
| `getLeft(self, right)` | 根据右侧值 O(1) 反向查找左侧键（返回 `L?` / `([]const u8)?`） |
| `removeByLeft(&self, left)` | 按左侧键删除双向映射（`BiMap` 返回 `R?`，`StringBiMap` 返回 `bool`） |
| `removeByRight(&self, right)` | 按右侧值删除双向映射（`BiMap` 返回 `L?`，`StringBiMap` 返回 `bool`） |
| `containsLeft(self, left)` | 判断左侧键是否存在 |
| `containsRight(self, right)` | 判断右侧值是否存在 |
| `len(self)` | 返回当前映射对数量 |
| `clear(&self)` | 清空全部映射并保留容量（`StringBiMap` 同步释放字符串内存） |

## 4. 示例

```cp
import { BiMap, StringBiMap } from "cap:ds";
import cap from "cap";

let bm = BiMap(u32, u64).init(alloc);
defer bm.deinit();
try bm.insert(1, 100);
const r = bm.getRight(1); // 100
const l = bm.getLeft(100); // 1

let sbm = cap.bimap.StringBiMap.init(alloc);
defer sbm.deinit();
try sbm.insert("id_1", "alice");
```
