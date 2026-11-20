# CP 标准库：RFC 6455 WebSocket 帧协议、握手与分片重组手册 (`cap:ws`)

## 一、 架构总览与设计准则

`cap:ws` 是 CP 语言官方内置的完整 RFC 6455 WebSocket 协议标准库，吸收自 Bun 原生 WebSocket 子系统（`/data/dev/bun/src/http/websocket.cp` 与 `/data/dev/bun/src/http_jsc/websocket_client/`），提供 SIMD 4 字节 XOR 掩码、RSV1-3 与 7/16/64-bit 扩展长度帧编解码、真实 SHA-1 + Base64 握手计算与 HTTP Upgrade 校验、Close 载荷编解码以及完整的分片重组与自动 Pong 应答状态机。

```text
┌───────────────────────────────────────────────────────────────────────────┐
│                         CP WebSocket 全栈处理管线                          │
├────────────────────┬───────────────────────┬──────────────────────────────┤
│ 握手与升级校验      │ 帧头与 SIMD 掩码层     │ 分片重组状态机                │
│ WebSocketHandshake │ FrameHeader (RSV1-3)  │ WebSocketMessageAssembler    │
│  computeAcceptKey  │ FrameBuilder          │  feedFrame / feedBytes       │
│  generateClientKey │  buildFragment/Close  │  Continue 分片拼接            │
│  build/validate    │  buildPongReply       │  Utf8 文本校验 + Auto-Pong   │
│  (SHA-1 + Base64)  │ Mask.apply (16B SIMD) │  ClosePayload (1000..4999)   │
└────────────────────┴───────────────────────┴──────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ws/opcode.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `Opcode`（含 `fromU4` 安全校验）与 `CloseCode`（含 `isValidWireCode` 范围校验） |
| `lib/ws/mask.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `Mask.apply` / `applyWithOffset` 16 字节 `@Vector(16, u8)` SIMD 循环异或掩码 |
| `lib/ws/frame.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `FrameHeader`（FIN/RSV1-3/Opcode/7-16-64bit/控制帧合法性校验）、`ClosePayload` 与 `FrameBuilder` |
| `lib/ws/handshake.cp` | `// Source: /data/dev/bun/src/http/websocket.cp` | `WebSocketHandshake`（RFC 6455 SHA-1+Base64 `Sec-WebSocket-Accept` 计算与 Upgrade 请求/响应校验） |
| `lib/ws/assembler.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `WebSocketMessageAssembler` 分片重组、控制帧交错处理、UTF-8 校验与自动 Pong 生成 |
| `lib/ws/index.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `cap:ws` 统一导出与便捷命名空间 `ws` |

---

## 二、 核心 API 详解

### 1. `WebSocketHandshake` 握手与升级协议

- 真实计算 `Sec-WebSocket-Accept = Base64(SHA1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))`（复用 `cap:crypto` `sha1Raw` 与 `cap:encoding` `Base64`）。

| API 签名 | 说明 |
| :--- | :--- |
| `WebSocketHandshake.computeAcceptKey(key, out_buf): ([]const u8)!` | 计算 28 字节 `Sec-WebSocket-Accept` |
| `WebSocketHandshake.generateClientKey(out_buf): ([]const u8)!` | 生成 16 字节随机 Base64 `Sec-WebSocket-Key` |
| `WebSocketHandshake.buildClientUpgradeRequest(out, host, path, key, proto)` | 生成 HTTP/1.1 Upgrade GET 请求报文 |
| `WebSocketHandshake.buildServerUpgradeResponse(out, client_key, proto)` | 生成 HTTP/1.1 `101 Switching Protocols` 响应报文 |
| `WebSocketHandshake.validateClientUpgradeRequest(raw_http)` | 校验请求头 `Upgrade`/`Connection`/`Version: 13`/`Key` |
| `WebSocketHandshake.validateServerUpgradeResponse(raw_http, client_key)` | 校验 `101` 状态及服务端 `Sec-WebSocket-Accept` |

### 2. `FrameHeader`、`ClosePayload` 与 `FrameBuilder`

- `FrameHeader` 新增 `rsv1`、`rsv2`、`rsv3` 位支持，并在 `decode`/`encode` 严格校验 RFC 6455 约束：
  - 控制帧（`Close`/`Ping`/`Pong`）禁止分片（`fin == false` 报错 `FragmentedControlFrame`）且长度必须 `<= 125`（报错 `ControlFrameTooLarge`）；
  - 拒绝保留操作码（`InvalidOpcode`）与 64 位最高位非零长度（`InvalidPayloadLength`）。
- `ClosePayload.parse(payload)` / `encode(out, code, reason)`：校验合法 RFC 6455 线缆关闭码（`1000..1003`、`1007..1011`、`3000..4999`）及关闭原因合法 UTF-8。
- `FrameBuilder` 新增：`buildFragment(out, fin, opcode, payload, mask)`、`buildContinue(...)`、`buildPongReply(out, ping_payload, mask)`、`buildCloseCode(out, code, reason, mask)`。

### 3. `WebSocketMessageAssembler` 分片重组状态机

| API 签名 | 说明 |
| :--- | :--- |
| `WebSocketMessageAssembler.init(allocator, max_message_size)` | 初始化分片重组状态机（防止超大报文耗尽内存） |
| `asm.feedFrame(hdr, payload): (WebSocketMessage?)!` | 喂入已解码帧头与负载，自动去掩码并拼接分片；文本帧完成时校验 UTF-8 |
| `asm.feedBytes(wire): AssembleResult!` | 从原始字节流解析帧头并喂入状态机（支持交错出现的 `Ping`/`Pong`/`Close` 控制帧） |
| `asm.hasPendingPong(): bool` / `asm.takePongFrame(out, mask)` | 当收到 `Ping` 时自动准备等载荷 `Pong` 应答帧 |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { FrameBuilder, WebSocketHandshake, WebSocketMessageAssembler } from "cap:ws";

pub fn demoWebSocketFull(): !void {
    const allocator = std.heap.page_allocator;

    // 1. RFC 6455 握手 Accept 计算
    let acc_buf: [32]u8 = undefined;
    const accept = try WebSocketHandshake.computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==", &acc_buf);
    // accept == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
    * = accept;

    // 2. 分片消息重组 + 自动 Pong
    let assembler = WebSocketMessageAssembler.init(allocator, 1024 * 1024);
    defer assembler.deinit();

    let f1: [64]u8 = undefined;
    let f2: [64]u8 = undefined;
    let mask: [4]u8 = [0x12, 0x34, 0x56, 0x78];
    const n1 = try FrameBuilder.buildFragment(&f1, false, .Text, "Hello ", mask);
    const n2 = try FrameBuilder.buildContinue(&f2, true, "CP!", mask);

    * = try assembler.feedBytes(f1[0..n1]); // null (等待 Continue)
    const done = try assembler.feedBytes(f2[0..n2]);
    // done.message.?.data == "Hello CP!"
    * = done;
}
```
