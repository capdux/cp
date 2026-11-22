# CP 标准库：高性能布谷鸟过滤器手册 (`cap:cuckoo`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 一、 架构总览与设计准则

`cap:cuckoo` 是 CP 语言官方内置的纯 CP **布谷鸟过滤器（Cuckoo Filter）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为缓存防穿透、去重黑名单、限流白名单与分布式路由打造。相比标准 Bloom Filter，在相同假阳性率下拥有更高的空间利用率，并**原生支持 O(1) 安全删除（`delete`）**。既可通过 `import { ... } from "cap:ds"` 导入，也支持 `import cap from "cap"; cap.cuckoo.*` 动态懒挂载调用。

### 核心算法参数与不变量
- **桶与槽位布局**：每桶 `4` 槽位（`BUCKET_SIZE = 4`），每槽位存储 `u16`（16-bit）指纹；保留 `0` 作为空槽位哨兵，有效指纹范围 `1..=65535`。
- **严格对合可逆备选桶**：桶数量 `bucket_count` 自动向上取整为 2 的幂，备选桶计算公式为 `altIndex = (i ^ hash(fp)) & (bucket_count - 1)`，确保 `(altIndex(i, fp) ^ hash(fp)) & mask == i` 严格成立。
- **有界随机踢出与无损回滚**：当两个候选桶均满时，最多执行 `MAX_KICKS = 500` 次随机踢出重定位；若 500 次仍未找到空位，按逆序精确回滚所有槽位并抛出 `CuckooError.FilterFull`，保证**零假阴性（Zero False Negatives）**。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/cuckoo/cuckoo.cp` | `// Source: cp/lib/cuckoo/cuckoo.cp (...)` | `CuckooError`、`CuckooStats`、`CuckooFilter` |
| `lib/cuckoo/index.cp` | `// Source: cp/lib/cuckoo/index.cp (...)` | `cap:cuckoo` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `CuckooError`
- `FilterFull`：过滤器已达高负载极限，经过 `MAX_KICKS = 500` 次重定位仍无法插入（内部状态已完整回滚）。
- `InvalidCapacity`：初始化传入 `min_capacity == 0` 或容量超出安全范围。
- `OutOfMemory`：底层桶槽位切片分配失败。

### 2. 统计快照 `CuckooStats`
```typescript
pub struct CuckooStats {
    bucket_count: usize,
    slot_count: usize,
    item_count: usize,
    load_factor: f64,
    total_kicks: u64,
}
```

### 3. `CuckooFilter`
- **生命周期**：
  - `CuckooFilter.init(alloc: std.mem.Allocator, min_capacity: usize): CuckooFilter ! CuckooError`
  - `deinit(*_)`
- **成员操作**：
  - `insert(*_, item: []const u8): void ! CuckooError`：插入元素；若遇满载重定位失败则无损回滚并返回 `FilterFull`。
  - `insertUnique(*_, item: []const u8): bool ! CuckooError`：若 `contains(item)` 为 `true` 则跳过并返回 `false`，否则插入并返回 `true`。
  - `contains(item: []const u8): bool`：查询元素是否可能存在（保证零假阴性，16-bit 指纹理论假阳性率约 `0.012%`）。
  - `delete(*_, item: []const u8): bool`：在主桶或备选桶中移除 1 个匹配指纹并递减计数，不影响同指纹的其他元素。
  - `clear(*_)`：清空所有槽位并重置计数与踢出统计，保留已分配内存。
- **状态查询**：
  - `len(): usize`：当前已插入元素计数。
  - `loadFactor(): f64`：当前槽位占用率（`item_count / slot_count`）。
  - `stats(): CuckooStats`：返回容量、占用率与累计踢出次数快照。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { CuckooFilter, CuckooError } from "cap:ds";
import cap from "cap";

pub fn demoCuckoo(): ! {
    const alloc = std.heap.page_allocator;

    let cf = CuckooFilter.init(alloc, 1024)?;
    defer cf.deinit();

    cf.insert("user:1001")?;
    cf.insert("user:1002")?;
    const exists = cf.contains("user:1001"); // true
    const removed = cf.delete("user:1001");   // true
    * = exists;
    * = removed;

    // 通过 cap.cuckoo 动态挂载调用
    let dyn_cf = cap.cuckoo.CuckooFilter.init(alloc, 64)?;
    defer dyn_cf.deinit();
    dyn_cf.insert("session:abc")?;
}
```
