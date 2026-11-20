# `cap:http2_frame` (`cap.http2_frame`) — RFC 9113 HTTP/2 二进制帧与多路复用状态机

## 1. 概述

`lib/http2_frame/` 是纯 CP 实现的工业级 RFC 9113 HTTP/2 二进制帧编解码、流式拆包与多路复用流状态机标准库，对外提供 `cap:http2_frame` 与 `cap.http2_frame` 双协议导入支持。

核心能力：
- **RFC 9113 9 字节帧头与全 10 种帧类型**：支持 `DATA`、`HEADERS`、`PRIORITY`、`RST_STREAM`、`SETTINGS`、`PUSH_PROMISE`、`PING`、`GOAWAY`、`WINDOW_UPDATE`、`CONTINUATION` 及未知扩展帧透传，内置 24 字节 `H2_PREFACE` 连接前言校验。
- **增量半包黏包流式分帧器 (`H2StreamDecoder`)**：支持任意 `1..7` 字节切片跨 chunk 喂入，校验 `max_frame_size`（默认 16384，最高 16 MiB）与各类控制帧长度/流 ID 约束。
- **多路复用流与流控窗口管理器 (`H2ConnectionWindow`)**：完整跟踪 `Idle -> Open -> HalfClosedLocal / HalfClosedRemote -> Closed` 状态转移与连接/流级发送接收窗口扣减、补充及溢出检测（`FlowControlError`）。

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `H2FrameType` | RFC 9113 全 10 种帧类型枚举 (`data` .. `continuation`) |
| `H2ErrorCode` | RFC 9113 §7 标准错误码 (`no_error` .. `http_1_1_required`) |
| `H2Error` | 编解码与状态机错误集 (`FrameTooLarge`, `InvalidFrameLength`, `InvalidStreamId`, `FlowControlError` 等) |
| `H2FrameHeader` | 9 字节帧头 (`length: u32`, `frame_type: u8`, `flags: u8`, `stream_id: u32`) |
| `H2Frame` | 联合体帧载荷 (`data`, `headers`, `rst_stream`, `settings`, `ping`, `goaway`, `window_update` 等) |
| `H2StreamDecoder` | 增量流式分帧器，支持 `feed` 与 `nextFrame` |
| `H2DecodedFrame` | 流式解帧产物，调用 `deinit(alloc)` 释放托管载荷 |
| `H2StreamState` | 流状态枚举 (`idle`, `open`, `half_closed_local`, `half_closed_remote`, `closed`) |
| `H2ConnectionWindow` | 连接级与流级多路复用状态机及流控窗口管理器 |

## 3. API 速查

- **连接前言与单帧编解码**：
  - `validatePreface(buf: []const u8): H2Error!void`
  - `H2FrameHeader.parse(bytes: []const u8): H2Error!H2FrameHeader`
  - `H2FrameHeader.encode(out: []u8): H2Error!void`
  - `encodeFrame(alloc: std.mem.Allocator, frame: H2Frame): H2Error![]u8`
  - `decodeFrame(alloc: std.mem.Allocator, header: H2FrameHeader, payload: []const u8): H2Error!H2Frame`
  - `freeDecodedFrame(alloc: std.mem.Allocator, frame: H2Frame)`
- **增量流式分帧器 (`H2StreamDecoder`)**：
  - `H2StreamDecoder.init(alloc: std.mem.Allocator, max_frame_size: u32): H2Error!H2StreamDecoder`
  - `decoder.deinit()` / `decoder.setMaxFrameSize(max_size: u32): H2Error!void`
  - `decoder.feed(chunk: []const u8): H2Error!void`
  - `decoder.nextFrame(): H2Error!H2DecodedFrame?`
- **多路复用流与流控 (`H2ConnectionWindow`)**：
  - `H2ConnectionWindow.init(alloc: std.mem.Allocator, initial_window: i64): H2Error!H2ConnectionWindow`
  - `win.deinit()` / `win.openStream(stream_id: u32): H2Error!void`
  - `win.transitionOnSendHeaders(stream_id: u32, end_stream: bool): H2Error!void`
  - `win.transitionOnRecvHeaders(stream_id: u32, end_stream: bool): H2Error!void`
  - `win.transitionOnSendEndStream(stream_id: u32): H2Error!void`
  - `win.transitionOnRecvEndStream(stream_id: u32): H2Error!void`
  - `win.resetStream(stream_id: u32): H2Error!void`
  - `win.consumeSendWindow(stream_id: u32, bytes: u32): H2Error!void`
  - `win.consumeRecvWindow(stream_id: u32, bytes: u32): H2Error!void`
  - `win.applyWindowUpdate(stream_id: u32, increment: u32): H2Error!void`

## 4. 示例

```typescript
const std = @import("std");
import {
    H2_DEFAULT_MAX_FRAME_SIZE,
    H2Frame,
    H2PingFrame,
    H2StreamDecoder,
    encodeFrame,
} from "cap:http2_frame";

pub fn main(): !void {
    const alloc = std.heap.page_allocator;
    const raw = try encodeFrame(alloc, H2Frame{
        .ping = H2PingFrame{ .ack = false, .opaque_data = .{ 1, 2, 3, 4, 5, 6, 7, 8 } },
    });
    defer alloc.free(raw);

    let dec = try H2StreamDecoder.init(alloc, H2_DEFAULT_MAX_FRAME_SIZE);
    defer dec.deinit();
    try dec.feed(raw);
    if (decoded := try dec.nextFrame()) {
        let d = decoded;
        defer d.deinit(alloc);
    }
}
```
