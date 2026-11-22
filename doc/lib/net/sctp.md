# `cap:sctp` — RFC 4960 SCTP 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配的 RFC 4960 Stream Control Transmission Protocol（SCTP）报文公共头、Castagnoli CRC32C 校验和与 4 字节对齐 Chunk 编解码器。

## 导入

```cp
import {
    SCTP_HEADER_SIZE,
    SCTP_CHUNK_HEADER_SIZE,
    CHUNK_DATA,
    CHUNK_INIT,
    CHUNK_INIT_ACK,
    CHUNK_SACK,
    CHUNK_HEARTBEAT,
    CHUNK_HEARTBEAT_ACK,
    CHUNK_ABORT,
    CHUNK_SHUTDOWN,
    CHUNK_SHUTDOWN_ACK,
    CHUNK_ERROR,
    CHUNK_COOKIE_ECHO,
    CHUNK_COOKIE_ACK,
    CHUNK_SHUTDOWN_COMPLETE,
    DATA_FLAG_END,
    DATA_FLAG_BEGIN,
    DATA_FLAG_UNORDERED,
    DATA_FLAG_IMMEDIATE,
    crc32c,
    sctpCrc32c,
    verifySctpChecksum,
    SctpHeader,
    SctpChunk,
    SctpChunkIter,
    SctpPacket,
    parseSctpPacket,
    SctpDataChunk,
    parseSctpData,
    SctpInitChunk,
    parseSctpInit,
    SctpSackChunk,
    parseSctpSack,
    SctpWriter,
} from "cap:net";
```

## 常量

| 常量 | 类型 | 值 | 说明 |
| :--- | :--- | :--- | :--- |
| `SCTP_HEADER_SIZE` | `usize` | `12` | SCTP 公共头长度 |
| `SCTP_CHUNK_HEADER_SIZE` | `usize` | `4` | SCTP Chunk 头长度 |
| `CHUNK_DATA` .. `CHUNK_SHUTDOWN_COMPLETE` | `u8` | `0..11, 14` | RFC 4960 核心 Chunk Type |
| `DATA_FLAG_END` / `BEGIN` / `UNORDERED` / `IMMEDIATE` | `u8` | `0x01`/`0x02`/`0x04`/`0x08` | DATA Chunk 标志位 |
| `SCTP_CRC32C_POLY` | `u32` | `0x82F63B78` | Castagnoli CRC32C 反射多项式 |

## 核心 API

### 校验和（Castagnoli CRC32C，Little-Endian 存储）
- `crc32c(data: []const u8): u32` — 计算任意字节切片的 Castagnoli CRC32C。
- `sctpCrc32c(packet: []const u8): u32` — 按 RFC 4960 计算完整 SCTP 报文校验和（自动将偏移 `8..12` 视为 `0x00000000`）。
- `verifySctpChecksum(packet: []const u8): bool` — 校验报文偏移 `8..12`（Little-Endian）处的 CRC32C 校验和。

### 报文与 Chunk 零拷贝解析
- `parseSctpPacket(data: []const u8): SctpPacket?` — 解析 12B 公共头（BE `src_port`/`dst_port`/`verification_tag`，LE `checksum`）与 `chunks_slice`。
  - `pkt.chunks(): SctpChunkIter` — 返回 4 字节对齐 Chunk 迭代器（末块允许无尾部填充直达 EOF）。
  - `pkt.verifyChecksum(full_packet: []const u8): bool` — 校验完整报文 CRC32C。
- `SctpChunkIter`：`next(*_): SctpChunk?`、`hasError(): bool`、`remaining(): usize`。
- `parseSctpData(chunk: SctpChunk): SctpDataChunk?` — 解析 DATA Chunk（`tsn`, `stream_id`, `stream_seq`, `ppid`, `user_data`，含 `isUnordered()`, `isBegin()`, `isEnd()`, `isImmediate()`）。
- `parseSctpInit(chunk: SctpChunk): SctpInitChunk?` — 解析 INIT / INIT ACK Chunk（`initiate_tag`, `a_rwnd`, `outbound_streams`, `inbound_streams`, `initial_tsn`, `params_slice`）。
- `parseSctpSack(chunk: SctpChunk): SctpSackChunk?` — 解析 SACK Chunk（`cumulative_tsn_ack`, `a_rwnd`, `num_gap_ack_blocks`, `num_dup_tsns`, `gap_blocks_slice`, `dup_tsns_slice`）。

### 零分配编码器 (`SctpWriter`)
- `SctpWriter.init(buf: []u8, src_port: u16, dst_port: u16, vtag: u32): SctpWriter`
- `writeChunk(*_, chunk_type: u8, flags: u8, value: []const u8): bool` — 自动按 4 字节边界补零对齐。
- `writeDataChunk(*_, flags: u8, tsn: u32, stream_id: u16, stream_seq: u16, ppid: u32, user_data: []const u8): bool` — 直接编码 DATA Chunk 并 4 字节补零对齐。
- `finish(*_): ([]const u8)?` — 计算并回填 4B Little-Endian CRC32C，返回完整报文切片。

## 示例

```cp
import std from "std";
import {
    DATA_FLAG_BEGIN,
    DATA_FLAG_END,
    SctpWriter,
    parseSctpPacket,
    parseSctpData,
    verifySctpChecksum,
} from "cap:net";

fn encodeAndDecode() {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = SctpWriter.init(buf[0..], 5000, 5001, 0x1122_3344);
    * = w.writeDataChunk(DATA_FLAG_BEGIN | DATA_FLAG_END, 100, 1, 0, 51, "hello");
    if (wire := w.finish()) {
        if (verifySctpChecksum(wire)) {
            if (pkt := parseSctpPacket(wire)) {
                let it = pkt.chunks();
                if (chunk := it.next()) {
                    if (dc := parseSctpData(chunk)) {
                        * = dc.user_data;
                    }
                }
            }
        }
    }
}
```
