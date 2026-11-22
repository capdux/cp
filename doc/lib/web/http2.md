// Source: cp/doc/lib/http2.md
# `cap:http2` — RFC 9113 HTTP/2 二进制分帧、流状态与流控引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

纯 CP、零 C 依赖的 RFC 9113 HTTP/2 二进制帧编解码、流式增量拆帧、流状态机与双向流量控制窗口库。

## 导入方式

```cp
import {
    CONNECTION_PREFACE,
    FrameType,
    FrameFlags,
    Http2ErrorCode,
    SettingId,
    SettingParam,
    Http2Frame,
    FrameStreamDecoder,
    FlowWindow,
    Http2Stream,
    StreamState,
    Http2Error,
    encodeFrame,
    encodeSettings,
    decodeSettings,
    encodePing,
    decodePing,
    encodeGoaway,
    decodeGoaway,
    encodeWindowUpdate,
    decodeWindowUpdate,
    encodeRstStream,
    decodeRstStream,
    encodeDataFrame,
    decodeDataPayload,
} from "cap:web";

// 或动态命名空间挂载：
import cap from "cap";
// cap.http2.FrameStreamDecoder / cap.http2.FlowWindow / cap.http2.encodeSettings
```

## 核心常量与枚举

- `CONNECTION_PREFACE: []const u8`：24 字节客户端连接序言 `"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"`
- `FRAME_HEADER_SIZE: usize = 9`
- `DEFAULT_MAX_FRAME_SIZE: u32 = 16384`、`MAX_ALLOWED_FRAME_SIZE: u32 = 16777215`
- `DEFAULT_INITIAL_WINDOW_SIZE: u32 = 65535`、`MAX_WINDOW_SIZE: u32 = 0x7FFFFFFF`
- `Http2Error`：`FrameSizeError`, `ProtocolError`, `FlowControlError`, `StreamClosed`, `InvalidSettingsLength`, `InvalidWindowIncrement`, `OutOfMemory`
- `FrameType: u8`：`Data (0x0)`, `Headers (0x1)`, `Priority (0x2)`, `RstStream (0x3)`, `Settings (0x4)`, `PushPromise (0x5)`, `Ping (0x6)`, `Goaway (0x7)`, `WindowUpdate (0x8)`, `Continuation (0x9)`
- `FrameFlags`：`END_STREAM (0x1)`, `ACK (0x1)`, `END_HEADERS (0x4)`, `PADDED (0x8)`, `PRIORITY (0x20)`
- `Http2ErrorCode: u32`：`NoError (0)`, `ProtocolError (1)`, `InternalError (2)`, `FlowControlError (3)`, ..., `EnhanceYourCalm (11)`
- `SettingId`：`HEADER_TABLE_SIZE (1)`, `ENABLE_PUSH (2)`, `MAX_CONCURRENT_STREAMS (3)`, `INITIAL_WINDOW_SIZE (4)`, `MAX_FRAME_SIZE (5)`, `MAX_HEADER_LIST_SIZE (6)`

## 帧编解码辅助 (`lib/http2/http2.cp`)

- `SettingParam { id: u16, value: u32 }`
- `Http2Frame { frame_type: FrameType, flags: u8, stream_id: u31, payload: []const u8 }`：解码返回的帧可通过 `frame.deinit(allocator)` 释放 `payload`
- `encodeFrame(allocator, frame_type, flags, stream_id, payload)`：构造 9 字节大端序帧头并清除保留位 `R`
- `encodeSettings(allocator, params, ack)` / `decodeSettings(allocator, frame)`：支持 6 字节键值对数组与 `ACK` 空负载校验
- `encodePing(allocator, opaque_data, ack)` / `decodePing(frame)`：8 字节 PING 负载编解码
- `encodeGoaway(allocator, last_stream_id, error_code, debug_data)` / `decodeGoaway(frame)`
- `encodeWindowUpdate(allocator, stream_id, increment)` / `decodeWindowUpdate(frame)`：校验增量 `1..0x7FFFFFFF`，`0` 或越界返回 `InvalidWindowIncrement`
- `encodeRstStream(allocator, stream_id, error_code)` / `decodeRstStream(frame)`
- `encodeDataFrame(allocator, stream_id, data, end_stream, pad_length)` / `decodeDataPayload(frame)`：自动处理 `PADDED` 填充与剥离

## 增量流式拆帧器 (`FrameStreamDecoder`)

- `init(allocator, max_frame_size)` / `deinit(*_)` / `setMaxFrameSize(*_, max_frame_size)`
- `feed(*_, chunk: []const u8): ! Http2Error`：支持任意跨分块半包/黏包喂入
- `nextFrame(*_): (Http2Frame?) ! Http2Error`：数据不足返回 `null`，超 `max_frame_size` 返回 `FrameSizeError`，非法 `SETTINGS` 长度返回 `InvalidSettingsLength`，非 0 `stream_id` 的 `SETTINGS`/`PING`/`GOAWAY` 返回 `ProtocolError`

## 流量控制与流状态 (`FlowWindow` & `Http2Stream`)

- `FlowWindow`：
  - `init(initial_size: u32)` / `available(): i64`
  - `consume(*_, bytes: u32): ! Http2Error`：超额返回 `FlowControlError`
  - `replenish(*_, increment: u32): ! Http2Error`：`0` 或 `> 0x7FFFFFFF` 返回 `InvalidWindowIncrement`，总窗口超过 `0x7FFFFFFF` 返回 `FlowControlError`
  - `updateInitialSize(*_, new_initial: u32, old_initial: u32): ! Http2Error`
- `Http2Stream`：`Idle` / `ReservedLocal` / `ReservedRemote` / `Open` / `HalfClosedLocal` / `HalfClosedRemote` / `Closed` 状态转换（`sendHeaders`, `recvHeaders`, `reset`）
