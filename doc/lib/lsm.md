# CP 标准库：纯 CP LSM-Tree 核心存储引擎 (`cap:lsm`)

## 一、 架构总览与设计准则

`cap:lsm` 是 CP 语言官方内置的纯 CP **LSM-Tree（Log-Structured Merge-Tree）高写入吞吐嵌入式存储引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。核心由以下分层组件构成：

1. **可写有序 `MemTable`**：基于二分查找维护的有序内存表，支持 `O(log N)` 点查与覆盖写；写入与删除（写入 `Tombstone` 墓碑标记）均分配单调递增序列号 `seq: u64`；当 `memtable_bytes >= memtable_max_bytes` 时自动触发 `flush()` 刷盘生成不可变 L0 `SsTable`。
2. **不可变 `SsTable`**：内存序列化二进制镜像，包含有序 Data Block（每条记录携带 `seq` 与 `is_tombstone`）、每 16 条采样一次的稀疏 Index Block（支持 `minKey()`/`maxKey()` 范围过滤与二分块定位），以及尾部 `"CPS1"` 魔数与 CRC32 校验和防篡改损坏。
3. **多层点查与 `rangeScan`**：`get(key)` 严格按「活跃 MemTable -> 最新到最旧 L0 SST -> L1 SST」顺序查找，命中 `Tombstone` 立即返回 `null` 绝不穿透读取旧版本；`rangeScan` 对 MemTable 与所有重叠 SST 执行多路最新序列胜出归并，过滤墓碑后返回严格升序切片。
4. **多路归并压缩 (`compact`)**：将所有 L0 与 L1 `SsTable` 执行多路归并，仅保留每个键最高 `seq` 的最新版本，并彻底清除已落底的 `Tombstone` 墓碑，合并为单一干净有序的 L1 `SsTable`，显著降低空间放大与读放大。

既可通过 `import { ... } from "cap:lsm"` 导入，也支持 `import cap from "cap"; cap.lsm.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/lsm/lsm.cp` | `// Source: cp/lib/lsm/lsm.cp (...)` | `LsmError`、`LsmEntry`、`LsmStats`、`SstRecord`、`SstLookup`、`SsTable`、`LsmEngine` |
| `lib/lsm/index.cp` | `// Source: cp/lib/lsm/index.cp (...)` | `cap:lsm` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型

- **`LsmError`**：
  - `CorruptSst`：SSTable 魔数不为 `"CPS1"`、CRC32 校验失败或内部块偏移越界/无序。
  - `InvalidRange`：`rangeScan` 传入 `start_key > end_key`。
  - `KeyTooLarge`：键长度超过 `MAX_KEY_LEN`（65535 字节）。
  - `OutOfMemory`：底层缓冲区、MemTable 或扫描切片分配失败。
- **`LsmEntry`**：`{ key: string, value: string }`（只读键值视图）。
- **`LsmStats`**：`{ memtable_entries: usize, memtable_bytes: usize, sst_count: usize, total_sst_entries: usize, compactions_run: usize }`。
- **`SstRecord`**：`{ key: string, value: string, seq: u64, is_tombstone: bool }`。
- **`SstLookup`**：`enum { Value([]const u8), Tombstone, NotFound }`。

### 2. `SsTable` 不可变有序表

- `SsTable.encodeFromEntries(alloc, entries: []const SstRecord, level: u8, id: u64): SsTable ! LsmError`：将按 `key` 严格升序排列的记录序列化为含 Data Block、稀疏 Index Block、CRC32 与 `"CPS1"` 尾标的 `SsTable`。
- `SsTable.decodeAndVerify(alloc, raw_bytes: []const u8): SsTable ! LsmError`：校验魔数、CRC32 及所有记录与索引边界并重建 `SsTable`，任一字节篡改或截断均返回 `LsmError.CorruptSst`。
- `deinit(*_)`：释放序列化字节镜像与稀疏索引。
- `get(key: string): SstLookup`：先进行 `minKey()`/`maxKey()` 范围过滤，再二分稀疏索引定位目标块并顺序扫描返回 `Value` / `Tombstone` / `NotFound`。
- `minKey(): string` / `maxKey(): string` / `len(): usize` / `rawBytes(): []const u8`。

### 3. `LsmEngine` 存储引擎

- `LsmEngine.init(alloc: std.mem.Allocator, memtable_max_bytes: usize): LsmEngine` / `deinit(*_)`
- `put(*_, key: string, value: string): void ! LsmError`：写入或覆盖键值并递增 `seq`；达到阈值自动 `flush()`。
- `delete(*_, key: string): void ! LsmError`：写入 `Tombstone` 墓碑标记并递增 `seq`；达到阈值自动 `flush()`。
- `get(key: string): ([]const u8)?`：跨 MemTable、L0（新到旧）、L1 精确点查；遇到 `Tombstone` 立即返回 `null`。
- `flush(*_): void ! LsmError`：将当前非空 MemTable 刷盘为新 L0 `SsTable`。
- `rangeScan(alloc, start_key: string, end_key: string): []LsmEntry ! LsmError`：多路归并 `[start_key, end_key]` 闭区间，按最新 `seq` 胜出并过滤墓碑。调用方使用 `alloc.free(entries)` 释放返回切片。
- `compact(*_): void ! LsmError`：全量多路归并压缩所有 L0/L1 SSTable，清除被覆盖旧版本与已落底墓碑，生成单一有序 L1 SSTable。
- `stats(): LsmStats`：返回当前 MemTable 条目数/字节数、SSTable 数量、SST 总条目数及压缩次数。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { LsmEngine } from "cap:lsm";
import cap from "cap";

pub fn demoLsm(): ! {
    const alloc = std.heap.page_allocator;

    let engine = LsmEngine.init(alloc, 1024);
    defer engine.deinit();

    engine.put("k:01", "v1")?;
    engine.put("k:02", "v2")?;
    engine.put("k:01", "v1_new")?;
    engine.delete("k:02")?;
    engine.flush()?;

    const v1 = engine.get("k:01"); // "v1_new"
    const v2 = engine.get("k:02"); // null (Tombstone)
    * = v1;
    * = v2;

    const entries = engine.rangeScan(alloc, "k:00", "k:99")?;
    defer alloc.free(entries);

    engine.compact()?;

    let dyn_engine = cap.lsm.LsmEngine.init(alloc, 512);
    defer dyn_engine.deinit();
    dyn_engine.put("dyn", "ok")?;
}
```
