# CP 标准库：崩溃安全嵌入式键值存储引擎 (`cap:kv`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 一、 架构总览与设计准则

`cap:kv` 是 CP 语言官方内置的零 C 依赖持久化嵌入式键值存储引擎，采用经典 **Bitcask** Append-Only WAL 架构，专为 AI Agent 本地状态持久化、工具调用缓存与会话检查点（Checkpointing）打造。**零 `std.posix`、零 `@import("bun")`**，底层文件 I/O 全部基于 `std.c` / `std.os.linux` 原生系统调用，支持 `import { ... } from "cap:db"` 与 `import cap from "cap"; cap.kv.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                CP 崩溃安全嵌入式键值存储引擎 (cap:kv)                     │
├─────────────────────────────┬───────────────────────────────────────────┤
│ CRC32 WAL 帧编解码 (wal.cp) │ Bitcask 引擎与原子压缩 (store.cp)          │
│ [CPKV][CRC32][op][klen][vlen]│ StringHashMap 内存索引 + Append-Only WAL   │
│ EOF / TornTail / Corrupt 区分│ 崩溃半写 ftruncate 自愈 + rename 原子压缩  │
└─────────────────────────────┴───────────────────────────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/kv/wal.cp` | `// Source: cp/lib/kv/wal.cp (CRC32-Protected Append-Only WAL Record Codec)` | `KvError`、`WalOp`、`WalRecord`、`WalDecodeResult`、`WalCodec`（IEEE 802.3 CRC32 帧编解码与半写/损坏精确区分） |
| `lib/kv/store.cp` | `// Source: cp/lib/kv/store.cp (Bitcask-Style Durable KV Store with Atomic Compaction)` | `KvStoreOptions`、`KvStats`、`KvStore`（WAL 回放、崩溃截断修复、字典序前缀扫描、`fsync` 落盘与原子 `compact`） |
| `lib/kv/index.cp` | `// Source: cp/lib/kv/index.cp (Unified Exports for cap:kv)` | `cap:kv` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. WAL 帧格式与编解码器（`wal.cp`）

二进制记录帧布局（Little-Endian）：
`[magic: 4B "CPKV"][crc32: u32 LE][op: u8][key_len: u32 LE][val_len: u32 LE][key][val]`

- **`KvError`**：`IoError`、`CorruptRecord`、`InvalidKey`、`OutOfMemory`。
- **`WalOp`**：`Put = 1`、`Delete = 2`。
- **`WalRecord`**：`{ op: WalOp, key: string, value: string }`，提供 `WalRecord.put(key, value)` 与 `WalRecord.del(key)` 构造器。
- **`WalDecodeResult`**：带载荷联合枚举：
  - `.Record(WalFrame)`：成功解码，`WalFrame` 包含 `record` 与 `next_offset`；
  - `.Eof`：正常到达流末尾（`offset >= bytes.len`）；
  - `.TornTail`：尾部存在未写完的半帧截断（部分 Magic/Header 或有效长度超出剩余字节）；
  - `.CorruptRecord`：非法 Magic、非法操作码或 IEEE 802.3 CRC32 校验失败。
- **`WalCodec`**：
  - `WalCodec.crc32(data): u32`
  - `WalCodec.encode(alloc, rec): ([]u8)!`
  - `WalCodec.decodeNext(bytes, offset): WalDecodeResult`

### 2. 持久化存储引擎（`store.cp`）

- **`KvStoreOptions`**：`{ sync_on_write: bool = false, recover_torn_tail: bool = true }`。
- **`KvStats`**：`{ live_keys: usize, total_records: u64, dead_records: u64, wal_bytes: u64, compactions: u64 }`。
- **`KvStore`**：
  - `KvStore.open(alloc, path, opts): KvStore!`：打开或创建 WAL 文件并顺序回放重建内存键值表；若尾部检测到 `TornTail` 且 `recover_torn_tail == true`，自动执行 `ftruncate` + `fsync` 修复截断尾部；若存在中间损坏（`CorruptRecord`）则拒绝打开并报错。
  - `put(*_, key, value): !` / `get(key): string?` / `delete(*_, key): bool!` / `has(key): bool` / `len(): usize`。
  - `keys(alloc): ([]string)!` / `prefixScan(alloc, prefix): ([]string)!`：返回按字典序排序的键列表（使用 `KvStore.freeKeys(alloc, list)` 释放）。
  - `sync(*_): !`：显式调用 `fsync` 强制落盘。
  - `compact(*_): !`：将当前全部存活键值写入 `<path>.compact.tmp` -> `fsync` -> 原子 `renameat2` 覆盖原 WAL -> 重新打开并清零 `dead_records`。
  - `stats(): KvStats` / `close(*_)` / `deinit(*_)`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { KvStore, KvStoreOptions } from "cap:db";
import cap from "cap";

pub fn demoKvCheckpoint()! {
    const alloc = std.heap.page_allocator;
    const db_path = "/tmp/agent_checkpoint.wal";

    let store = KvStore.open(alloc, db_path, KvStoreOptions{
        sync_on_write: true,
        recover_torn_tail: true,
    })?;
    defer store.deinit();

    store.put("agent/step", "42")?;
    store.put("agent/goal", "verify-wal")?;
    store.put("agent/step", "43")?; // 覆盖产生 1 条死记录

    const matches = store.prefixScan(alloc, "agent/")?;
    defer KvStore.freeKeys(alloc, matches);

    // 原子压缩回收历史覆盖/删除死记录空间
    store.compact()?;
    store.close();

    // 支持 cap.kv 动态懒挂载
    let reopened = cap.kv.KvStore.open(alloc, db_path, cap.kv.KvStoreOptions{})?;
    defer reopened.deinit();
    const val = reopened.get("agent/step");
    * = val;
}
```
