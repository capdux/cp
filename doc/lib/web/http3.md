// Source: cp/doc/lib/http3.md
# `cap:http3` — RFC 9114 HTTP/3 二进制分帧、流类型与 RFC 9204 QPACK 编解码引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

纯 CP、零 C 依赖的 RFC 9114 HTTP/3 帧与设置编解码器、单向流类型校验器，以及 RFC 9204 QPACK 静态表（99 项）头部压缩/解压库。

## 导入方式

```cp
import {
    QPACK_MAX_TABLE_CAPACITY,
    MAX_FIELD_SECTION_SIZE,
    QPACK_BLOCKED_STREAMS,
    Http3Error,
    H3StreamType,
    H3FrameType,
    H3Setting,
    H3PushPromiseFrame,
    H3UnknownFrame,
    H3Frame,
    H3Header,
    parseStreamType,
    encodeFrame,
    decodeFrames,
    decodeFramesWithMaxSize,
    freeFrames,
    encodeQpackHeaders,
    decodeQpackHeaders,
} from "cap:web";

// 或动态命名空间挂载：
import cap from "cap";
// cap.http3.encodeFrame / cap.http3.decodeFrames / cap.http3.encodeQpackHeaders
```

## 核心常量、错误与枚举

- `DEFAULT_MAX_FRAME_SIZE: usize = 16777216`（16 MiB 默认最大帧载荷）
- `QPACK_MAX_TABLE_CAPACITY: u64 = 0x01`、`MAX_FIELD_SECTION_SIZE: u64 = 0x06`、`QPACK_BLOCKED_STREAMS: u64 = 0x07`
- `Http3Error`：`UnexpectedEof`, `InvalidFrame`, `FrameUnexpected`, `DuplicateSetting`, `ReservedSetting`, `InvalidStreamType`, `QpackDecompressionFailed`, `FrameTooLarge`, `OutOfMemory`
- `H3StreamType: u64`：`Control (0x00)`, `Push (0x01)`, `QpackEncoder (0x02)`, `QpackDecoder (0x03)`
- `H3FrameType: u64`：`Data (0x00)`, `Headers (0x01)`, `CancelPush (0x03)`, `Settings (0x04)`, `PushPromise (0x05)`, `Goaway (0x07)`, `MaxPushId (0x0d)`
- `parseStreamType(stream_type_id: u64): H3StreamType ! Http3Error`：校验单向流类型 ID，未知 ID 返回 `InvalidStreamType`

## 帧编解码 (`H3Frame` & `H3Setting`)

- `H3Setting { id: u64, value: u64 }`
- `H3PushPromiseFrame { push_id: u64, header_block: []const u8 }`
- `H3UnknownFrame { frame_type: u64, payload: []const u8 }`
- `H3Frame` tagged union：`.data`, `.headers`, `.cancel_push`, `.settings`, `.push_promise`, `.goaway`, `.max_push_id`, `.unknown`
- `encodeFrame(allocator, frame): ([]u8) ! Http3Error`：
  - 按 RFC 9000 Varint 格式编码 `Type`、`Length` 与负载；
  - 对 `.settings` 严格校验 HTTP/2 保留设置（`0x02`/`0x03`/`0x04`/`0x05` -> `ReservedSetting`）与重复设置 ID（`DuplicateSetting`）；
  - 对 `.unknown` 拦截 HTTP/2 保留帧类型（`0x02`/`0x06`/`0x08`/`0x09` -> `FrameUnexpected`）。
- `decodeFrames(allocator, bytes): ([]H3Frame) ! Http3Error` / `decodeFramesWithMaxSize(allocator, bytes, max_frame_size): ([]H3Frame) ! Http3Error`：
  - 支持多帧连续解码与 GREASE/未知帧透明保留为 `.unknown`；
  - 超出 `max_frame_size` 返回 `FrameTooLarge`，截断返回 `UnexpectedEof`，HTTP/2 保留帧返回 `FrameUnexpected`；
  - `data`/`headers`/`push_promise.header_block`/`unknown.payload` 零拷贝借用输入切片，需调用 `freeFrames(allocator, frames)` 释放帧数组及内部 `.settings` 切片。

## RFC 9204 QPACK 静态表编解码

- `H3Header { name: []const u8, value: []const u8 }`
- `QPACK_STATIC_TABLE: [99]H3Header`：完整 RFC 9204 Appendix A 99 项静态表
- `encodeQpackHeaders(allocator, headers): ([]u8) ! Http3Error`：
  - 自动写入 2 字节前导（`Required Insert Count = 0`, `Delta Base = 0`）；
  - 优先命中静态表完全匹配（6-bit 前缀索引），其次命中静态表名称索引（4-bit 前缀 + 7-bit 字符串值），否则使用字面量名称（3-bit 前缀 + 7-bit 字符串值）。
- `decodeQpackHeaders(allocator, block): ([]H3Header) ! Http3Error`：
  - 校验前导 `Required Insert Count == 0` 与 `Delta Base == 0`，越界索引（`>= 99`）、动态表引用或截断均返回 `QpackDecompressionFailed`；
  - 返回的 `name`/`value` 借用静态表常量或 `block` 切片，调用方通过 `allocator.free(headers)` 释放切片。
