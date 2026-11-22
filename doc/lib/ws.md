# CP 标准库：RFC 6455 / RFC 7692 WebSocket 协议栈、扩展协商与会话状态机手册 (`cap:ws`)

> 参考来源：`/data/dev/bun/docs/runtime/http/websockets.mdx`, `/data/dev/bun/src/http/websocket.cp`, `/data/dev/bun/src/http_jsc/websocket_client/` | 吸收日期：2026-11-22

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
| `lib/ws/permessageDeflate.cp` | `// Source: cp/lib/ws/permessageDeflate.cp (RFC 7692 permessage-deflate)` | `PerMessageDeflateConfig`、`PerMessageDeflate` 协商与 `0x00 0x00 0xFF 0xFF` 编解码 |
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

### 2. RFC 7692 `permessage-deflate` (`lib/ws/permessageDeflate.cp`)

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

### WebSocket 主题发布/订阅代理（`lib/ws/pubsub.cp`，吸收自 `/data/dev/bun/src/runtime/server/WebSocketServerContext.cp`）

- 核心导出：`WS_PUBSUB_MAX_TOPICS` (`64`)、`WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC` (`64`)、`WsPubSubError`、`WsPubSubConfig`（`publish_to_self`）、`WsPublishResult`、`WsPubSubBroker`（`init` / `topicCount` / `subscriberCount` / `isSubscribed` / `subscribe` / `unsubscribe` / `unsubscribeAll` / `collectRecipients` / `listClientTopics`）。

```typescript
import { WsPubSubBroker, WsPubSubConfig } from "cap:ws";

let broker = WsPubSubBroker.init(WsPubSubConfig{ publish_to_self: false });
* = broker.subscribe(1, "the-group-chat")?;
* = broker.subscribe(2, "the-group-chat")?;
let targets: [8]u64 = [0; 8];
// ws.publish(sender=1): 仅投递给 client 2，跳过发送者自身
const r = broker.collectRecipients("the-group-chat", 1, targets[0..]);
// r.delivered_count == 1, targets[0] == 2, r.skipped_self == true
* = r;
```

### WebSocket 客户端 URL 解析与 HTTP/1.1 升级状态机（`lib/ws/clientUp.cp`，吸收自 `/data/dev/bun/src/http/websocket_http_client.cp` & `/data/dev/bun/docs/runtime/http/websockets.mdx`）

- 核心导出：`WS_CLIENT_MAX_CUSTOM_HEADERS` (`16`)、`WS_CLIENT_MAX_PROTOCOLS` (`8`)、`WsClientUpgradeError`、`WsUrlScheme`（`Ws`/`Wss`）、`WsUpgradePhase`（`Idle`/`AwaitingResponse`/`Upgraded`/`Failed`）、`WsClientUrl`（`parse`/`isTls`/`isDefaultPort`/`formatHostHeader`/`formatCanonicalUrl`）、`WsClientHeader`、`WsClientUpgradeConfig`（`addHeader`/`addProtocol`/`containsProtocol`）、`WsClientUpgradeMachine`（`init`/`buildUpgradeRequest`/`verifyUpgradeResponse`/`negotiatedProtocol`）。

```cp
import { WsClientUrl, WsClientUpgradeConfig, WsClientUpgradeMachine } from "cap:ws";

const url = WsClientUrl.parse("wss://realtime.example.com/v1/stream?room=42")?;
let cfg = WsClientUpgradeConfig{ enable_permessage_deflate: true };
cfg.addProtocol("chat.v1")?;
let machine = WsClientUpgradeMachine.init(url);
let req_buf: [512]u8 = [0; 512];
const n = machine.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", cfg, req_buf[0..])?;
* = n;
```

---

## 上游 Bun WebSocket 行为契约与安全边界注意事项

1. **RFC 6455 强制掩码与方向校验（`WsSession`）**：
   - 客户端发往服务端的帧**必须**携带 4 字节随机掩码（`is_server: true` 时收到未掩码帧立即触发协议错误 `1002`），服务端发往客户端的帧不得掩码；`Mask.apply` 利用 16 字节 SIMD 向量化异或加速掩码编解码。
2. **控制帧交错与载荷上限**：
   - RFC 6455 规定 `Ping` / `Pong` / `Close` 控制帧长度不得超过 `125` 字节且**禁止分片**（`FIN` 必须为 `1`），但允许在数据帧分片流中间交错插入；`WebSocketMessageAssembler` 自动处理控制帧交错并在开启 `auto_pong: true` 时即时生成匹配载荷的 `Pong` 帧。
3. **关闭码合法性与状态机（`CloseCodeValidator`）**：
   - `1005`（No Status）、`1006`（Abnormal Closure）与 `1015`（TLS Handshake）为保留内部状态码，**禁止在线路 `Close` 帧中发送**；`CloseCodeValidator` 严格拦截非法关闭码并校验关闭原因的 UTF-8 合法性。
4. **`permessage-deflate` 压缩与防解压膨胀（RFC 7692，吸收自 `/data/dev/bun/docs/guides/websocket/compression.mdx`）**：
   - 开启 `PerMessageDeflate` 时，既可全局启用全消息压缩，也可针对单条消息按需开启压缩（对应 Bun `ws.send(msg, true)`）；压缩消息须正确附加/剥离 `0x00 0x00 0xFF 0xFF` 同步尾（`PMD_SYNC_TRAILER`）；服务端务必在 `WsSessionConfig` 中设置合理的 `max_frame_bytes` 与 `max_message_bytes`，防止高压缩比载荷耗尽内存。
5. **原生 Pub-Sub 主题生命周期与 `publish_to_self` 契约（吸收自 `/data/dev/bun/docs/guides/websocket/pubsub.mdx`）**：
   - 默认 `publish_to_self == false` 时，单连接广播（`ws.publish`，传入 `sender_id`）自动排除发送者自身，而服务端广播（`server.publish`，传入 `sender_id = null`）投递给主题内全部订阅者；
   - 连接断开（`close` 回调）时务必调用 `unsubscribeAll(client_id)` 清理订阅并自动回收空主题槽位，防止长周期服务出现孤儿订阅泄漏。
6. **客户端 URL 合规性与升级握手安全防线（吸收自 `/data/dev/bun/src/http/websocket_http_client.cp` & `/data/dev/bun/docs/runtime/http/websockets.mdx`）**：
   - 遵循 RFC 6455 Section 3：`WsClientUrl.parse` 严格拒绝包含 `#fragment` 或控制字符/CRLF 的 URL，支持 IPv6 字面量 `[::1]:port` 并按默认端口（`ws` 80 / `wss` 443）规范化 `Host` 头；
   - `WsClientUpgradeConfig.addHeader` 禁止注入 `\r`/`\n` 并拒绝覆盖核心握手头（`Host`/`Upgrade`/`Connection`/`Sec-WebSocket-Key`/`Sec-WebSocket-Version`）；`WsClientUpgradeMachine.verifyUpgradeResponse` 严格校验 `101 Switching Protocols`、`Sec-WebSocket-Accept` 28 字节 SHA-1 Base64 摘要以及协商子协议白名单，任意不匹配立即置状态为 `Failed`。
7. **DevServer HMR WebSocket 协议帧与连接会话管理（`lib/ws/hmrSocket.cp`，吸收自 `/data/dev/bun/src/bake/DevServer/HmrSocket.cp` & `/data/dev/bun/docs/bundler/hot-reloading.mdx`）**：
   - **导出符号**：`HMR_VERSION_FRAME_LEN` (`17`)、`HMR_INIT_FRAME_LEN` (`9`)、`HMR_MAX_REFERENCED_SOURCEMAPS` (`32`)、`HMR_TOPIC_MAX_COUNT` (`8`)、`HmrSocketError`、`HmrIncomingOpcode`、`HmrOutgoingOpcode`、`HmrTopicBits`、`HmrTopicDiff`、`HmrInitPayload`、`HmrCloseSummary`、`isHmrLocalhostIp`、`encodeHmrVersionFrame`、`decodeHmrVersionFrame`、`encodeHmrInitFrame`、`decodeHmrInitFrame`、`HmrSocketSession`；
   - **安全与零泄漏最佳实践**：
     - **环回地址鉴权（`isHmrLocalhostIp` / `requireLocalhost`）**：仅来自 `127.0.0.1` 或 `::1` 的 HMR WebSocket 连接允许执行敏感 DevServer 状态检视或变更操作；
     - **二进制精简握手**：建连即发送 17 字节版本帧（`'V'` + 16 字节配置哈希），客户端上报 9 字节初始化帧（`'i'` + 8 位十六进制 `generation_id`）并将对应 SourceMap 强引用登记到 `HmrSocketSession`；
     - **增量主题位掩码差分与 `closeAndDrain`**：`applySubscribePayload` 精确计算新增/取消订阅差分位，断连时 `closeAndDrain` 一次性排出全部 SourceMap 引用键、旧路由 ID 与已订阅主题，确保长周期开发服务器零引用泄漏。

```cp
import { HmrSocketSession, encodeHmrInitFrame, decodeHmrInitFrame } from "cap:ws";

let sess = HmrSocketSession.init("127.0.0.1", false);
let frame_buf: [16]u8 = [0; 16];
const wire = encodeHmrInitFrame(0x0000002a, frame_buf[0..])?;
const parsed = decodeHmrInitFrame(wire)?;
* = sess.trackSourceMap(parsed.source_map_key)?;
const diff = sess.applySubscribePayload("he")?; // added: hot_update + errors
let drain: [32]u64 = [0; 32];
const summary = sess.closeAndDrain(drain[0..]);
* = diff; * = summary;
```
