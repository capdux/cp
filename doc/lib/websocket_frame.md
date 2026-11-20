# `cap:websocket_frame` — RFC 6455 WebSocket 帧协议解析、掩码与分片重组引擎

> **模块路径**: `lib/websocket_frame/`  
> **导入协议**: `import { WsOpcode, WsCloseCode, WsError, computeAcceptKey, encodeFrame, decodeFrame, WsStreamAssembler } from "cap:websocket_frame";`  
> **动态挂载**: `import cap from "cap";` -> `cap.websocket_frame.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:websocket_frame` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix` 或 `@import("bun")`）的工业级 RFC 6455 WebSocket 帧协议解析、编码、掩码与流式分片重组引擎：

1. **RFC 6455 握手与帧建模 (`types.cp`)**：
   - `WsOpcode`：`Continuation = 0x0`、`Text = 0x1`、`Binary = 0x2`、`Close = 0x8`、`Ping = 0x9`、`Pong = 0xA`，提供 `isControl()`、`isData()`、`fromU4()`。
   - `WsCloseCode`：标准关闭状态码（`1000..1011`），提供 RFC 6455 §7.4.1 线缆合法性校验 `isValidWireCode()` 与 `fromU16()`。
   - `WsError`：`Incomplete`、`ReservedBitsSet`、`InvalidOpcode`、`ControlFrameTooLarge`、`ControlFrameFragmented`、`UnmaskedClientFrame`、`PayloadTooLarge`、`InvalidUtf8`、`UnexpectedContinuation`、`FragmentedInProgress`、`InvalidClosePayload`、`OutOfMemory`。
   - `computeAcceptKey(key_base64)`：零堆分配计算 `Base64(SHA1(key_base64 + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))`，返回定长 `[28]u8`。
2. **帧编码与解码 (`codec.cp`)**：
   - `encodeFrame(alloc, opcode, payload, fin, mask_key_opt)`：自动编码 7-bit、`126` (16-bit) 与 `127` (64-bit) 长度扩展字段及可选 4 字节 XOR 掩码；强制校验控制帧 `FIN=1` 且 `len <= 125`。
   - `encodeCloseFrame(alloc, code, reason, mask_key_opt)`：校验关闭码与 UTF-8 原因字符串并编码 Close 帧。
   - `applyMask(buf, mask_key)`：4 字节循环 XOR 原地掩码/解掩码。
   - `decodeFrame(alloc, buf, require_masked, max_payload)`：严格校验 RSV 位、Opcode、控制帧约束、客户端掩码强制、最大载荷限制、Close 载荷与单帧 Text UTF-8 合法性，返回 `DecodedFrame { frame, bytes_consumed }`。
3. **增量流式分片重组 (`assembler.cp`)**：
   - `WsStreamAssembler`：支持任意 `1..N` 字节半包/黏包跨 chunk `feed(chunk)`，支持 Text/Binary 多分片（`FIN=0` + `Continuation` + `FIN=1`）自动重组，支持 RFC 6455 §5.4 分片中途穿插 Ping/Pong/Close 控制帧，并在完整 Text 消息和 Close 载荷上强制 UTF-8 校验，`deinit()` 零泄漏释放。

---

## 二、 核心 API 速查表

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `computeAcceptKey(key_base64)` | 计算 28 字节 `Sec-WebSocket-Accept`（零堆分配） |
| `applyMask(buf, mask_key)` | 原地 4 字节循环 XOR 掩码/解掩码 |
| `encodeFrame(alloc, opcode, payload, fin, mask_key_opt)` | 编码 WebSocket 帧（自动处理 7/16/64-bit 长度与掩码） |
| `encodeCloseFrame(alloc, code, reason, mask_key_opt)` | 编码带状态码与 UTF-8 原因的 Close 帧 |
| `decodeFrame(alloc, buf, require_masked, max_payload)` | 解码单帧并返回 `DecodedFrame` 或 `WsError` |
| `WsFrame.deinit(alloc)` | 释放 `WsFrame.payload` |
| `WsStreamAssembler.init(alloc, require_masked, max_payload)` | 初始化流式分片重组器 |
| `WsStreamAssembler.deinit()` | 释放内部缓冲区与未消费消息队列 |
| `WsStreamAssembler.feed(chunk)` | 喂入任意字节分片，解码所有完整帧与分片消息 |
| `WsStreamAssembler.nextMessage()` | 弹出下一条已就绪 `WsMessage?`（调用方负责 `msg.deinit(alloc)`） |
| `WsStreamAssembler.pendingCount()` | 返回队列中待消费的消息数量 |
| `WsMessage.deinit(alloc)` | 释放 `WsMessage.payload` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    computeAcceptKey,
    encodeFrame,
    WsStreamAssembler,
} from "cap:websocket_frame";

fn demo(alloc: std.mem.Allocator): !void {
    const accept = computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==");
    // accept == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="

    const mask_key: [4]u8 = [0x37, 0xFA, 0x21, 0x3D];
    const wire = encodeFrame(alloc, .Text, "Hello WebSocket!", true, mask_key)?;
    defer alloc.free(wire);

    let assembler = WsStreamAssembler.init(alloc, true, 64 * 1024);
    defer assembler.deinit();

    assembler.feed(wire)?;
    if (msg := assembler.nextMessage()) {
        let m = msg;
        defer m.deinit(alloc);
        // m.opcode == .Text, m.payload == "Hello WebSocket!"
    }
}
```
