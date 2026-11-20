// Source: cp/doc/lib/wal.md
# `cap:wal` — Castagnoli CRC32C 预写日志（WAL）帧编解码、崩溃恢复与日志压缩引擎

纯 CP、零 C 依赖（无 `std.posix` 与 `@import("bun")`）的内存预写日志（Write-Ahead Log）帧引擎，提供固定 24 字节小端序帧头、Castagnoli CRC32C 完整性校验、严格恢复与掉电半写尾部截断恢复，以及基于 LSN 的日志压缩能力。

## 导入方式

```cp
import {
    WAL_MAGIC,
    WAL_HEADER_SIZE,
    WAL_DEFAULT_MAX_RECORD_BYTES,
    WalError,
    WalRecordType,
    WalRecord,
    WalRecoveryResult,
    WalLog,
    crc32c,
} from "cap:wal";

// 或动态命名空间挂载：
import cap from "cap";
// cap.wal.WalLog / cap.wal.WalRecord / cap.wal.WalError / cap.wal.crc32c
```

## 帧格式契约

每条 WAL 记录采用固定 24 字节小端序帧头 + 变长负载：

```text
[magic:4 ("WAL1")][lsn:u64 LE][payload_len:u32 LE][record_type:u8][reserved:3][crc32c:u32 LE][payload:N]
```

- `WAL_MAGIC = ['W', 'A', 'L', '1']`
- `WAL_HEADER_SIZE = 24`
- `crc32c`（Castagnoli `0x82F63B78` 多项式）覆盖 `magic + lsn + payload_len + record_type + reserved + payload`（即 `header[0..20]` 与 `payload[0..N]`），头部或负载任意单比特翻转均可检出。

## 错误与类型定义

### `WalError`
- `CorruptedFrame`：魔数非 `"WAL1"` 或非法 `record_type`
- `ChecksumMismatch`：CRC32C 校验和不匹配（头部或负载被篡改）
- `TruncatedTail`：帧头不足 24 字节或负载不足 `payload_len` 字节（半写截断）
- `SequenceOutOfOrder`：LSN 为 0 或未严格单调递增（`lsn <= prev_lsn`）
- `PayloadTooLarge`：负载超过 `max_record_bytes` 或 `u32` 上限
- `OutOfMemory`：内存分配失败

### `WalRecordType`
- `Data = 1`：普通数据写入记录
- `Checkpoint = 2`：检查点记录
- `Tombstone = 3`：删除墓碑记录

### `WalRecord`
- `lsn: u64`：全局单调递增日志序列号
- `record_type: WalRecordType`：记录类型
- `payload: []const u8`：记录二进制负载

### `WalRecoveryResult`
- `records: []WalRecord`：已恢复的有效记录列表
- `valid_bytes: usize`：有效已落盘字节偏移量
- `truncated_tail_bytes: usize`：尾部被安全截断的字节数（严格模式下为 `0`）
- `last_lsn: u64`：最后一条有效记录的 LSN（空日志为 `0`）
- `deinit(*_, alloc: std.mem.Allocator)`：释放所有记录负载及切片内存

## 核心引擎 (`WalLog`)

- `WalLog.init(alloc: std.mem.Allocator, max_record_bytes: usize): WalLog`
- `deinit(*_)`
- `append(*_, payload: []const u8): u64 ! WalError`：自动递增 LSN 并追加 `Data` 记录
- `appendTyped(*_, record_type: WalRecordType, payload: []const u8): u64 ! WalError`：自动递增 LSN 并追加指定类型记录
- `appendWithLsn(*_, lsn: u64, record_type: WalRecordType, payload: []const u8): u64 ! WalError`：显式指定 LSN 追加（若 `lsn == 0 || lsn <= last_lsn` 返回 `SequenceOutOfOrder`）
- `appendBatch(*_, payloads: []const []const u8): u64 ! WalError`：原子预检并批量追加多条 `Data` 记录
- `bytes(): []const u8`：返回当前序列化 WAL 字节流只读视图
- `lastLsn(): u64` / `recordCount(): usize` / `byteLen(): usize`
- `compactBefore(*_, alloc: std.mem.Allocator, min_lsn: u64): ! WalError`：清除 `lsn < min_lsn` 的旧帧并重写紧凑日志（保留 `lastLsn` 高水位以确保后续写入 LSN 持续递增）
- `WalLog.recoverStrict(alloc: std.mem.Allocator, raw_bytes: []const u8): WalRecoveryResult ! WalError`：严格恢复，遇到任何损坏或半写截断立即报错
- `WalLog.recoverWithTailTruncation(alloc: std.mem.Allocator, raw_bytes: []const u8): WalRecoveryResult ! WalError`：容错崩溃恢复，遇到尾部半写截断或末尾帧损坏时安全截断并记录 `valid_bytes` 与 `truncated_tail_bytes`
- `WalLog.restoreTruncated(alloc: std.mem.Allocator, max_record_bytes: usize, raw_bytes: []const u8): WalLog ! WalError`：从受损字节流安全截断恢复为可继续追加写入的 `WalLog` 实例
