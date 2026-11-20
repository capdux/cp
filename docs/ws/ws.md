# CP 标准库：RFC 6455 / RFC 7692 WebSocket 协议栈、扩展协商与会话状态机手册 (`cap:ws`)

## 一、 架构总览与设计准则

`cap:ws` 是 CP 语言官方内置的工业生产级 RFC 6455 / RFC 7692 WebSocket 协议引擎，在保留既有高性能帧编解码、SIMD 掩码与握手校验的基础上，新增子协议与扩展头协商、RFC 6455 §7.4.1 关闭码严格校验、RFC 7692 `permessage-deflate` 消息压缩/解压以及完整的 `WsSession` 会话生命周期状态机。

```text
┌───────────────────────────────────────────────────────────────────────────────────────┐
│                            CP WebSocket 工业生产级协议引擎 (`cap:ws`)                  │
├───────────────────────┬──────────────────────────┬────────────────────────────────────┤
│ 握手与协商层           │ 帧头、掩码与压缩层        │ 会话与分片重组状态机                │
│ WebSocketHandshake    │ FrameHeader (FIN/RSV1-3) │ WsSession (Connecting/Open/Closing │
│ SubprotocolNegotiator │ FrameBuilder             │   /Closed)                         │
│ WsExtensionHeader     │ Mask.apply (16B SIMD)    │ WebSocketMessageAssembler          │
│ CloseCodeValidator    │ PerMessageDeflate        │  分片重组 + 控制帧交错 + Auto-Pong │
│  (RFC 6455 §7.4.1)    │  (RFC 7692 00 00 FF FF)  │  掩码强制 + 超限帧/消息防护         │
└───────────────────────┴──────────────────────────┴────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游/源码注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ws/opcode.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `Opcode`（含 `fromU4` 安全校验）与 `CloseCode` |
| `lib/ws/mask.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `Mask.apply` / `applyWithOffset` 16 字节 `@Vector(16, u8)` SIMD 循环异或掩码 |
| `lib/ws/frame.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `FrameHeader`、`ClosePayload` 与 `FrameBuilder` |
| `lib/ws/handshake.cp` | `// Source: /data/dev/bun/src/http/websocket.cp` | `WebSocketHandshake`（SHA-1 + Base64 `Sec-WebSocket-Accept` 与 Upgrade 报文校验） |
| `lib/ws/assembler.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `WebSocketMessageAssembler` 分片重组、控制帧交错、UTF-8 校验与 Auto-Pong |
| `lib/ws/subprotocol.cp` | `// Source: cp/lib/ws/subprotocol.cp (RFC 6455 Subprotocol, Extensions & Close Validation)` | `SubprotocolNegotiator`、`WsExtensionHeader` 解析/序列化、`CloseCodeValidator` |
| `lib/ws/permessage_deflate.cp` | `// Source: cp/lib/ws/permessage_deflate.cp (RFC 7692 permessage-deflate)` | `PerMessageDeflateConfig`、`PerMessageDeflate` 协商与 `0x00 0x00 0xFF 0xFF` 编解码 |
| `lib/ws/session.cp` | `// Source: cp/lib/ws/session.cp (RFC 6455 WebSocket Session State Machine)` | `WsSessionState`、`WsSessionConfig`、`WsSession` 完整协议状态机与安全防护 |
| `lib/ws/index.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `cap:ws` 统一导出与便捷命名空间 `ws` |

---

## 二、 核心 API 详解

### 1. 子协议协商、扩展头解析与关闭码校验 (`lib/ws/subprotocol.cp`)

| 类型 / API | 说明 |
| :--- | :--- |
| `SubprotocolNegotiator.parseOffered(header)` | 解析逗号分隔的 `Sec-WebSocket-Protocol`，校验 HTTP Token 字符并拒绝空项与重复项 |
| `SubprotocolNegotiator.select(offered, supported)` | 遵循 RFC 6455 §11.3.4 大小写精确匹配，按客户端偏好顺序选择首个受支持子协议 |
| `SubprotocolNegotiator.selectFromHeader(header, supported)` | 一步解析并完成子协议选择 |
| `WsExtensionHeader.parse(single_ext)` / `parseAll(header)` | 解析 `permessage-deflate; client_max_window_bits=15; server_no_context_takeover` 等扩展与带引号参数 |
| `ext.serialize(out)` | 序列化扩展名与参数为标准 `Sec-WebSocket-Extensions` 报文头切片 |
| `CloseCodeValidator.isValidWireCloseCode(code)` | 严格遵循 RFC 6455 §7.4.1：拒绝 `0-999`、`1004`、`1005`、`1006`、`1015`、`1016-2999` 及 `>=5000`，允许 `1000-1003`、`1007-1014`、`3000-4999` |
| `CloseCodeValidator.isValidUtf8(bytes)` | 校验 Close 原因文本是否为合法 UTF-8 |

### 2. RFC 7692 `permessage-deflate` (`lib/ws/permessage_deflate.cp`)

| 类型 / API | 说明 |
| :--- | :--- |
| `PerMessageDeflateConfig` | 配置 `server_no_context_takeover`、`client_no_context_takeover`、`server_max_window_bits`、`client_max_window_bits`、`compression_threshold`、`compression_level`、`max_decompressed_bytes` |
| `PerMessageDeflate.negotiate(client_ext_header)` | 解析客户端扩展要约，校验参数合法性并生成服务端协商结果与 `responseHeader()` |
| `PerMessageDeflate.negotiateWithConfig(header, prefs)` | 按显式服务端偏好策略进行多要约回退协商 |
| `PerMessageDeflate.compressPayload(payload, level, wbits)` | 基于 `cap:pack` 原始 Deflate (`Z_SYNC_FLUSH`) 压缩并按 RFC 7692 §7.2.1 剥离尾部 `0x00 0x00 0xFF 0xFF` |
| `PerMessageDeflate.decompressPayload(compressed, max_bytes, wbits)` | 按 RFC 7692 §7.2.2 补回尾部 `0x00 0x00 0xFF 0xFF` 后解压，并在超限时返回 `error.MessageTooBig` |
| `PerMessageDeflate.init(config)` / `compress` / `decompress` | 支持上下文接管（Context Takeover）与 `no_context_takeover` 自动复位的高性能流式会话编解码器 |

### 3. `WsSession` 协议状态机 (`lib/ws/session.cp`)

- **状态流转**：`Connecting` -> `markOpen()` -> `Open` -> `initiateClose()` / 收到对端 `Close` -> `Closing` -> 双方 `Close` 完成 -> `Closed`。
- **安全与资源耗尽防护**：
  - 服务端强制校验客户端入站帧 `masked == true`（否则 `error.UnmaskedClientFrame`）；客户端强制校验服务端入站帧 `masked == false`（否则 `error.MaskedServerFrame`）。
  - 在完整负载到达前即校验 `FrameHeader.payload_len <= max_frame_bytes` 与累计消息长度 `<= max_message_bytes`。
  - 校验 `RSV2/RSV3 == 0`，且仅当启用 `permessage_deflate` 时允许数据起始帧携带 `RSV1`。
- **控制帧与心跳**：支持分片消息中途交错 `Ping`/`Pong`/`Close` 控制帧；收到 `Ping` 且 `auto_pong == true` 时可通过 `takePendingPongFrame()` 生成等载荷 `Pong` 帧；支持 `shouldSendHeartbeat(now_ms)` 检测心跳超时。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    SubprotocolNegotiator,
    PerMessageDeflate,
    WsSession,
    WsSessionConfig,
    FrameBuilder,
} from "cap:ws";

pub fn demoProductionWs(): !void {
    const allocator = std.heap.page_allocator;

    // 1. 子协议与 permessage-deflate 协商
    const supported: [2][]const u8 = ["chat", "superchat"];
    const proto = try SubprotocolNegotiator.selectFromHeader("Chat, superchat", &supported);
    // proto == "superchat" (RFC 6455 大小写精确匹配)
    * = proto;

    let neg = try PerMessageDeflate.negotiate("permessage-deflate; client_max_window_bits=12; server_no_context_takeover");
    * = neg.responseHeader();

    // 2. WsSession 状态机处理掩码帧、Auto-Pong 与 Close 握手
    let session = WsSession.initOpen(allocator, WsSessionConfig{
        is_server: true,
        max_message_bytes: 1024 * 1024,
        max_frame_bytes: 65536,
        auto_pong: true,
    });
    defer session.deinit();

    let frame: [128]u8 = undefined;
    const mask: [4]u8 = [0x12, 0x34, 0x56, 0x78];
    const n = try FrameBuilder.buildText(&frame, "Hello WsSession!", mask);
    const res = try session.feedBytes(frame[0..n]);
    * = res;
}
```
