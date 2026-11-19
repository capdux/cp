# CP 标准库：高性能网络传输、套接字抽象与代理隧道手册 (`cap:net`)

## 一、 架构总览与设计准则

`cap:net` 是 CP 语言官方内置的高性能网络传输标准库，涵盖 IPv4/IPv6 地址解析、RAII 风格 `SafeSocket` 生命周期管理、带 16KB 软塞（Corking）缓冲的 `TcpStream`、多选项监听配置的 `TcpServer`、无连接 `UdpSocket` 数据报传输，以及 RFC 7231 HTTP `CONNECT` / RFC 1928 SOCKS5 代理握手编解码器。核心架构吸收自 Bun 原生 uSockets 与 Socket 子系统（`/data/dev/bun/src/uws/`、`src/uws_sys/`、`src/runtime/socket/` 与 `src/http/ProxyTunnel.cp`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层网络应用 (Application)                    │
├───────────────────┬─────────────────────┬───────────────────────────────┤
│ 传输层高阶封装     │  地址与协议编解码    │  旧版 Actor/Async 兼容层       │
│ TcpServer /       │  Ipv4Addr / Ipv6Addr│  Server / Socket              │
│ TcpStream (Cork)  │  SocketAddress      │  listen(port) / connect(port) │
│ UdpSocket / Packet│  HttpConnectTunnel  │  async read() / write()       │
│ SafeSocket (RAII) │  Socks5Codec        │                               │
├───────────────────┴─────────────────────┴───────────────────────────────┤
│            uSockets C-FFI 绑定层 (_usockets.cp / @c & @opaque)          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/net/` 下全部 8 个源文件第 1 行均严格标记上游 Bun 源码谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/net/_usockets.cp` | `// Source: /data/dev/bun/src/uws_sys/ & /data/dev/bun/src/uws/uws.cp` | 底层 uSockets `@c` FFI 声明、`@opaque` 句柄类型、监听位掩码常量及 `SocketCloseCode` / `SocketKind` / `SendStatus` 枚举 |
| `lib/net/address.cp` | `// Source: /data/dev/bun/src/runtime/socket/SocketAddress.cp` | `AddressFamily`、`Ipv4Addr`、`Ipv6Addr`、`SocketAddress` 零分配解析、私有/回环/组播判定与 IPv4-Mapped IPv6 转换 |
| `lib/net/socket.cp` | `// Source: /data/dev/bun/src/uws_sys/socket.cp` | `SafeSocket` 状态机、读写统计 `SocketStats`、`>240s` 自动升级分钟级长超时、半关闭与 RAII `release()`，以及兼容型 `Socket` |
| `lib/net/tcp_stream.cp` | `// Source: /data/dev/bun/src/uws/uws.cp` | 16KB `StreamBuffer`（`STREAM_CORK_BUFFER_SIZE = 16384`）与支持 `cork()` / `uncork()` / `flush()` 批聚合发送的 `TcpStream` |
| `lib/net/tcp_server.cp` | `// Source: /data/dev/bun/src/runtime/socket/Listener.cp` | `ListenConfig`（`toLibusOptions()` 位掩码生成）、`TcpServer` 连接接入计数与生命周期，以及兼容型 `Server` / `listen()` / `connect()` |
| `lib/net/udp.cp` | `// Source: /data/dev/bun/src/uws_sys/udp.cp` | `UdpPacket`、`UdpSocket` 绑定、对端关联（`connectPeer`）、广播/单播 TTL/组播 TTL 与回环选项控制 |
| `lib/net/proxy.cp` | `// Source: /data/dev/bun/src/http/ProxyTunnel.cp` | `HttpConnectTunnel`（RFC 7231 `CONNECT` 报文构造与响应状态码解析）与 `Socks5Codec`（RFC 1928 握手与 IPv4/域名目标连接编码） |
| `lib/net/index.cp` | `// Source: /data/dev/bun/src/uws/uws.cp & /data/dev/bun/src/runtime/socket/` | `cap:net` 统一公开导出入口 |

---

## 二、 核心 API 与关键机制

### 1. 地址模型（`Ipv4Addr`、`Ipv6Addr`、`SocketAddress`）

- **`Ipv4Addr`**：提供 `init(a, b, c, d)`、`loopback()`、`any()`、`broadcast()`、`parse(text)`、`toU32()` / `fromU32(val)`，以及语义判定方法 `isLoopback()`、`isUnspecified()`、`isPrivate()`（RFC 1918 10/8、172.16/12、192.168/16）、`isMulticast()`（224.0.0.0/4）。
- **`Ipv6Addr`**：提供 `loopback()`（`::1`）、`any()`（`::`）、`fromIpv4Mapped(v4)`（`::ffff:a.b.c.d`），以及 `isLoopback()`、`isUnspecified()`、`isIpv4Mapped()`。
- **`SocketAddress`**：统一封装 IPv4 / IPv6 端点（含 `port`、`flowlabel`、`scope_id`），提供 `v4`、`v6`、`loopbackV4`、`loopbackV6`、`parseV4` / `parseIpv4`、`isIPv6()` 与 `socklen()`（IPv4 返回 `16`，IPv6 返回 `28`）。

### 2. `SafeSocket` 与超时自动分级机制

- 完整管理 `SocketState`（`Connecting` / `Connected` / `Closing` / `Closed` / `Detached`）与 `SocketStats`（读写字节数与调用次数）。
- **uSockets 240 秒阈值自动升级**：`setTimeout(seconds)` 在 `seconds <= 240` 时配置 4 秒粒度的短超时 `timeout_sec`；超过 `240` 秒时自动清零短超时并晋升为分钟级长超时 `long_timeout_min = seconds / 60`，消除长连接高频定时器轮询开销。
- 支持半关闭 `shutdown()`（关闭写端）与 `shutdownRead()`（关闭读端）、流控 `pause()` / `resumeSocket()`，以及 `release()` 统一兜底关闭并释放堆对象。

### 3. `TcpStream` 16KB Corking 与 `TcpServer`

- **`StreamBuffer` & `TcpStream`**：开启 `cork()` 后，多次小块 `write()` 会零系统调用地聚合进 16KB（`STREAM_CORK_BUFFER_SIZE`）发送缓冲；调用 `uncork()` 或缓冲区满时自动批量 `flush()`，大幅降低内核态切换次数。
- **`ListenConfig` & `TcpServer`**：通过 `ListenConfig.defaultForPort(port)` 配置 `reuse_port`、`reuse_addr`、`exclusive_port`、`allow_half_open`、`ipv6_only` 等，调用 `toLibusOptions()` 直接生成 uSockets 位掩码；`TcpServer.bind(addr, cfg)` 管理 `active_connections` 与 `total_accepted` 统计。

### 4. `UdpSocket` 与代理隧道编解码（`HttpConnectTunnel`、`Socks5Codec`）

- **`UdpSocket`**：支持 `bind(addr)`、`connectPeer(peer)` / `disconnectPeer()`、`setBroadcast`、`setUnicastTTL`、`setMulticastTTL`、`setMulticastLoopback`、`sendTo` / `send`。
- **`HttpConnectTunnel`**：零堆分配构造 `CONNECT host:port HTTP/1.1` 请求（`buildConnectRequest`），并解析代理响应状态码（`parseConnectResponse` / `isTunnelEstablished`）。
- **`Socks5Codec`**：支持无认证握手包编码（`encodeGreetingNoAuth` -> `[0x05, 0x01, 0x00]`）、服务端握手校验（`verifyGreetingReply`）、IPv4/域名 `CONNECT` 请求编码（`encodeConnectIpv4` / `encodeConnectDomain`）及连接响应校验（`verifyConnectReply`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    Ipv4Addr,
    Ipv6Addr,
    SocketAddress,
    SafeSocket,
    SocketKind,
    ListenConfig,
    TcpServer,
    UdpSocket,
    HttpConnectTunnel,
    Socks5Codec,
} from "cap:net";

pub fn demoNetSubsystem() {
    // 1. IPv4 / IPv6 地址解析与转换
    const ip = Ipv4Addr.parse("10.0.1.42").?;
    // ip.isPrivate() == true, ip.toU32() == 0x0A00012A
    const v6_mapped = Ipv6Addr.fromIpv4Mapped(ip);
    // v6_mapped.isIpv4Mapped() == true

    // 2. TcpServer + TcpStream Corking 聚合发送（CP 编译器自动注入 RAII release）
    const bind_addr = SocketAddress.loopbackV4(8080);
    const cfg = ListenConfig.defaultForPort(8080);
    let server = TcpServer.bind(bind_addr, cfg);

    const peer = SocketAddress.parseV4("192.168.1.20", 54321).?;
    if (stream := server.acceptConnection(peer)) {
        stream.setTimeout(360); // >240s 自动晋升为 6 分钟长超时
        stream.cork();
        * = stream.write("HTTP/1.1 200 OK\r\n\r\n");
        * = stream.write("Hello cap:net");
        const flushed = stream.uncork(); // 一次批量刷出 32 字节
        * = flushed;
        stream.release();
    }

    // 3. HTTP CONNECT 与 SOCKS5 代理握手编解码
    let req_buf: [256]u8 = [0 as u8; 256];
    const req_len = HttpConnectTunnel.buildConnectRequest(req_buf[0..256], "api.cp-lang.org", 443);
    const ok = HttpConnectTunnel.isTunnelEstablished("HTTP/1.1 200 Connection Established\r\n\r\n");
    * = req_len;
    * = ok;

    let s5_buf: [64]u8 = [0 as u8; 64];
    const g_len = Socks5Codec.encodeGreetingNoAuth(s5_buf[0..64]); // [0x05, 0x01, 0x00]
    const c_len = Socks5Codec.encodeConnectDomain(s5_buf[0..64], "registry.cp-lang.org", 443);
    * = g_len;
    * = c_len;
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`Bun.listen` / `Bun.udpSocket` / `src/uws`) | Node.js (`node:net` / `node:dgram`) | CP 标准库 (`cap:net`) |
| :--- | :--- | :--- | :--- |
| **地址解析与判定** | `SocketAddress` (`src/runtime/socket/`) | `net.isIPv4()` / `net.isIPv6()` | `Ipv4Addr.parse` / `Ipv6Addr` / `SocketAddress` |
| **TCP 服务端监听** | `Bun.listen({ hostname, port, socket })` | `net.createServer().listen()` | `TcpServer.bind(addr, ListenConfig)` / `listen(port)` |
| **TCP 批聚合发送 (Cork)** | uSockets `us_socket_stream_buffer_t` | `socket.cork()` / `socket.uncork()` | `TcpStream.cork()` / `.uncork()` / `StreamBuffer` (16KB) |
| **套接字选项与长超时** | `SocketHandler.setTimeout` (`>240s` 升级) | `socket.setNoDelay()` / `setKeepAlive()` / `setTimeout()` | `SafeSocket.setNoDelay` / `setKeepAlive` / `setTimeout` |
| **UDP 数据报套接字** | `Bun.udpSocket({ port, socket })` | `dgram.createSocket('udp4')` | `UdpSocket.bind(addr)` / `sendTo` / `setMulticastTTL` |
| **HTTP CONNECT / SOCKS5 代理** | `src/http/ProxyTunnel.cp` | 第三方库（`https-proxy-agent` / `socks`） | `HttpConnectTunnel` / `Socks5Codec` |
