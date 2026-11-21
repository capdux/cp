# `cap:rdb` — 零分配 Redis RDB 二进制快照解析库

> **模块路径**：`lib/rdb/`（导入方式：`import { ... } from "cap:rdb";`）  
> **特性**：100% 纯 CP、零堆分配、完整支持 RDB 文件头、4 种变长整数编码、int8/16/32 紧凑整数串、LZF 边界切片与全套核心 Opcode 流式解析。

---

## 一、 核心常量

- **魔数与头部**：`RDB_MAGIC = "REDIS"`、`RDB_HEADER_SIZE = 9`
- **控制 Opcode**：`OP_MODULE_AUX (0xF7)`、`OP_IDLE (0xF8)`、`OP_FREQ (0xF9)`、`OP_AUX (0xFA)`、`OP_RESIZEDB (0xFB)`、`OP_EXPIRETIME_MS (0xFC)`、`OP_EXPIRETIME (0xFD)`、`OP_SELECTDB (0xFE)`、`OP_EOF (0xFF)`
- **值类型常量**：`RDB_TYPE_STRING (0)` .. `RDB_TYPE_LIST_QUICKLIST_2 (18)`

---

## 二、 核心类型与函数

- `isRdb(data: []const u8): bool` / `parseRdbHeader(data: []const u8): RdbHeader?`
- `readRdbLength(data: []const u8, pos: usize): RdbLength?`  
  支持 `00` 6-bit、`01` 14-bit BE、`10` 0x80 32-bit BE / 0x81 64-bit BE、`11` 特殊编码（`int8`/`int16`/`int32`/`lzf`）。
- `readRdbString(data: []const u8, pos: usize): RdbStringVal?`  
  支持普通字节串、有符号 `int8`/`int16`/`int32` 紧凑整数与 LZF 压缩串（返回零拷贝压缩切片 `raw` 与 `uncompressed_len`）。
- `RdbReader.init(data)` & `next(*_): RdbEvent?`  
  流式产出 `aux`、`select_db`、`resize_db`、`string_kv`（自动绑定前置秒/毫秒过期时间戳至 `expire_ms: u64?`）、`eof`（读取 8B CRC64 校验尾）。

---

## 三、 使用示例

```typescript
import { RdbReader, RdbEventKind } from "cap:rdb";

fn scanRdb(bytes: []const u8) {
    let reader = RdbReader.init(bytes);
    while (ev := reader.next()) {
        if (ev.kind == RdbEventKind.string_kv) {
            * = ev.key.raw.len;
            * = ev.value.raw.len;
        }
    }
}
```
