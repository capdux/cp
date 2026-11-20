# CP 标准库：RFC 2326 RTSP 协议编解码与 `$` 交织流拆帧 (`cap:rtsp`)

## 一、 架构总览与设计准则

`cap:rtsp` 是 CP 语言官方内置的 **RFC 2326 RTSP/1.0（Real Time Streaming Protocol）** 纯 CP 协议编解码、Transport 头解析与 TCP 交织流拆帧标准库。严格遵循 RFC 2326 文本报文格式与 §10.12 `$` 二进制交织 RTP/RTCP 帧规范，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:rtsp"` 静态导入与 `import cap from "cap"; cap.rtsp.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│             CP RTSP/1.0 协议引擎与交织流拆帧器 (cap:rtsp)                │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ 请求/响应流畅构造器    │ Transport 头编解码器     │ 混合增量流式拆帧器     │
│ RtspBuilder           │ parseTransportHeader    │ RtspStreamParser      │
│ 自动递增 CSeq          │ formatTransportHeader   │ 文本消息 + SDP 载荷    │
│ CR/LF 头注入防护       │ interleaved/client_port │ '$' 二进制交织 RTP/RTCP│
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/rtsp/rtsp.cp` | `// Source: cp/lib/rtsp/rtsp.cp (Pure CP RFC 2326 RTSP Protocol Codec, Transport Header & Interleaved '$' Framing)` | `RtspError`、`RtspMethod`、`RtspMessage`、`TransportSpec`、`RtspStreamParser`、`RtspBuilder` |
| `lib/rtsp/index.cp` | `// Source: cp/lib/rtsp/index.cp (Public Unified Export for cap:rtsp & cap.rtsp)` | `cap:rtsp` 与 `cap.rtsp` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `RtspError` 与方法枚举 `RtspMethod`

```typescript
pub err RtspError {
    InvalidVersion,          // 非 RTSP/1.0 协议版本（如 RTSP/2.5）
    InvalidStartLine,        // 请求行或状态行格式非法 / 未知方法
    MalformedHeader,         // 头部缺少冒号或名称含非法字符
    InvalidContentLength,    // 非数字或冲突的 Content-Length
    InvalidTransport,        // 非法 Transport 头格式或端口/通道越界
    InvalidInterleavedFrame, // 非法 '$' 交织帧
    PayloadTooLarge,         // 载荷或交织帧长度超出 max_payload_bytes 上限
    HeaderInjection,         // 检测到 CR/LF 头注入字符
    OutOfMemory,             // 内存分配失败
}
```

- `RtspMethod` 完整覆盖 RFC 2326 11 种方法：`Options`, `Describe`, `Announce`, `Setup`, `Play`, `Pause`, `Teardown`, `GetParameter`, `SetParameter`, `Record`, `Redirect`，提供 `toString()` 与 `RtspMethod.parse(s)`。

### 2. 消息模型 `RtspMessage`

- `RtspHeader { name: []const u8, value: []const u8 }`
- `RtspStartLine`：标签联合 `request: RtspRequestLine` | `response: RtspResponseLine`
- `RtspMessage` 方法：
  - `msg.deinit(allocator)`：释放全部堆分配字符串、头部数组与消息体
  - `msg.header(name): ([]const u8)?`：大小写不敏感查找首条匹配头部
  - `msg.cseq(): u32?`：解析 `CSeq` 序号
  - `msg.sessionId(): ([]const u8)?`：提取 `Session` ID（自动剥离 `;timeout=...` 参数）
  - `msg.contentBase(): ([]const u8)?`：获取 `Content-Base` 头部
  - `msg.format(allocator): []u8 ! RtspError`：序列化为 `\r\n` 线协议报文，自动补全 `Content-Length` 并拦截 CR/LF 注入

### 3. `TransportSpec` 与 `Transport` 头编解码

- `RtspPortPair { rtp: u16, rtcp: u16? }`
- `TransportSpec` 支持 `protocol`、`unicast`、`multicast`、`interleaved`（`0-1`）、`client_port`（`5000-5001`）、`server_port`、`ssrc`、`mode`（`"PLAY"`）、`destination`、`source`、`ttl`、`layers`，提供 `spec.deinit(allocator)`。
- `parseTransportHeader(allocator, val): TransportSpec ! RtspError`
- `formatTransportHeader(allocator, spec): []u8 ! RtspError`

### 4. 混合增量流式拆帧器 `RtspStreamParser`

- `RtspStreamItem`：标签联合 `message: RtspMessage` | `interleaved: RtspInterleavedFrame`（`{ channel: u8, data: []const u8 }`），提供 `item.deinit(allocator)`。
- `RtspStreamParser.init(allocator, max_payload_bytes)` / `parser.deinit()`
- `parser.feed(chunk): void ! RtspError`：喂入任意分片网络字节流
- `parser.nextFrame(): RtspStreamItem? ! RtspError`：跨任意半包/黏包边界拆出完整 `RTSP/1.0` 文本消息（含 `Content-Length` 载荷如 SDP）或 RFC 2326 §10.12 `['$'][channel:u8][length:u16 BE][payload]` 二进制交织帧。
- `encodeInterleavedFrame(allocator, channel, data): []u8 ! RtspError`：编码 `$` 交织二进制帧。

### 5. 流畅构造器 `RtspBuilder`

- `RtspBuilder.init(allocator)` / `builder.deinit()`
- `builder.request(method, uri)` / `builder.response(status_code, reason_phrase)`
- `builder.header(name, value)` / `builder.cseq(seq)` / `builder.session(id)` / `builder.transport(spec)` / `builder.setBody(body)`
- `builder.build(): RtspMessage ! RtspError` / `builder.buildBytes(): []u8 ! RtspError`：自动递增 `CSeq` 与维护 `Content-Length`，严格拦截 CR/LF 头注入返回 `RtspError.HeaderInjection`。

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    RtspMethod,
    RtspBuilder,
    RtspStreamParser,
    encodeInterleavedFrame,
    parseTransportHeader,
} from "cap:rtsp";

let builder = RtspBuilder.init(alloc);
defer builder.deinit();

* = builder.request(.Describe, "rtsp://127.0.0.1:554/live/cam0") catch unreachable;
* = builder.header("Accept", "application/sdp") catch unreachable;
const wire = builder.buildBytes() catch unreachable;
defer alloc.free(wire);
```
