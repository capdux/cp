# CP 标准库：RFC 9000 QUIC 变长整数、包头与帧编解码 (`cap:quic`)

## 一、 架构总览与设计准则

`cap:quic` 是 CP 语言官方内置的纯 CP **RFC 9000 QUIC 协议底层线缆编解码引擎**，涵盖 §16 变长整数（Varint）、§17 长/短包头（Long/Short Header）以及 §19 核心帧（`PADDING`、`PING`、`ACK`、`RESET_STREAM`、`CRYPTO`、`STREAM`、`MAX_DATA`、`CONNECTION_CLOSE`）的无状态编解码。全模块 **零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，解码路径中的载荷与连接 ID 切片均零拷贝引用输入缓冲区。同时支持 `import { ... } from "cap:quic"` 具名导入与 `import cap from "cap"; cap.quic.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/quic/quic.cp` | `// Source: cp/lib/quic/quic.cp (...)` | `QuicError`、RFC 9000 §16 Varint、§17 `LongHeader`/`ShortHeader`、§19 `QuicFrame` 编解码与内存释放 |
| `lib/quic/index.cp` | `// Source: cp/lib/quic/index.cp (Public Unified Export for cap:quic & cap.quic)` | `cap:quic` & `cap.quic` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合 (`QuicError`)

- `UnexpectedEof`：输入字节流在包头或 varint 读取完成前耗尽。
- `VarintOutOfRange`：编码值超过 RFC 9000 §16 上限 `2^62 - 1`（`0x3FFFFFFFFFFFFFFF`）。
- `InvalidHeader`：缺少 QUIC 固定比特 `0x40`、Header Form 比特不匹配或包号长度超出 `1..4`。
- `InvalidFrameType`：遇到未支持的帧类型标识。
- `InvalidConnectionId`：DCID 或 SCID 长度超过 RFC 9000 上限 `20` 字节。
- `TruncatedFrame`：帧内部 varint 或声明的载荷长度超出可用载荷边界。
- `TooManyRanges`：`ACK` 帧的附加区间数超过安全上限 `MAX_ACK_RANGES`（1024）。
- `OutOfMemory`：内存分配失败。

### 2. RFC 9000 §16 变长整数

- `varintLen(v: u64): usize ! QuicError`：返回编码所需字节数（1 / 2 / 4 / 8），超限返回 `VarintOutOfRange`。
- `writeVarint(list: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u64): !QuicError`：将变长整数大端序写入 `ArrayList(u8)`。
- `encodeVarint(allocator: std.mem.Allocator, v: u64): ([]u8) ! QuicError`：编码为新分配的 `[]u8`（调用方需 `allocator.free(...)`）。
- `decodeVarint(bytes: []const u8): VarintDecoded ! QuicError`：返回 `{ value: u64, len: usize }`。

### 3. QUIC 包头编解码 (`LongHeader` / `ShortHeader`)

- **`LongPacketType`**：`Initial = 0`、`ZeroRtt = 1`、`Handshake = 2`、`Retry = 3`。
- **`LongHeader`**：`{ packet_type, version, dcid, scid, token, length, packet_number_len }`。
  - `encodeLongHeader(allocator, hdr): ([]u8) ! QuicError`
  - `decodeLongHeader(bytes): LongHeader ! QuicError`（自动校验 DCID/SCID `<= 20` 字节及固定比特 `0x40`）。
- **`ShortHeader`**：`{ spin_bit, key_phase, dcid, packet_number_len }`。
  - `encodeShortHeader(allocator, hdr): ([]u8) ! QuicError`
  - `decodeShortHeader(bytes, dcid_len): ShortHeader ! QuicError`

### 4. QUIC 核心帧编解码 (`QuicFrame`)

- **`QuicFrame`**（带标签联合体）：
  - `padding: usize`（连续 `0x00` 自动聚合）
  - `ping: void`
  - `ack: AckFrame`（`{ largest_acked, ack_delay, first_ack_range, ranges: []AckRange }`）
  - `crypto: CryptoFrame`（`{ offset, data }`）
  - `stream: StreamFrame`（`{ stream_id, offset, length, fin, data }`）
  - `reset_stream: ResetStreamFrame`（`{ stream_id, error_code, final_size }`）
  - `max_data: u64`
  - `connection_close: ConnectionCloseFrame`（`{ is_app_error, error_code, frame_type, reason_phrase }`）
- `encodeFrame(allocator, frame: QuicFrame): ([]u8) ! QuicError`
- `decodeFrames(allocator, payload: []const u8): ([]QuicFrame) ! QuicError`
- `freeFrames(allocator, frames: []QuicFrame)`：释放帧切片及所有 `AckFrame.ranges`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { encodeVarint, decodeVarint, QuicFrame, encodeFrame, decodeFrames, freeFrames } from "cap:quic";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;

    const vwire = encodeVarint(alloc, 151288809941952652) catch unreachable;
    defer alloc.free(vwire);
    const vdec = decodeVarint(vwire) catch unreachable;
    _ = vdec;

    const fwire = cap.quic.encodeFrame(alloc, QuicFrame {
        stream: {
            stream_id: 4,
            offset: 128,
            fin: true,
            data: "hello quic",
        },
    }) catch unreachable;
    defer alloc.free(fwire);

    const frames = decodeFrames(alloc, fwire) catch unreachable;
    defer freeFrames(alloc, frames);
}
```
