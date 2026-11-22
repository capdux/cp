# CP 标准库：高性能网络传输、套接字抽象、CIDR/SSRF 防护、代理隧道与连接池手册 (`cap:net`)

> 参考来源：`/data/dev/bun/docs/runtime/networking/tcp.mdx`、`/data/dev/bun/docs/runtime/networking/udp.mdx`、`/data/dev/bun/src/uws/uws.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:net` 是 CP 语言官方内置的工业生产级网络传输标准库，涵盖 IPv4/IPv6 地址与 CIDR/SSRF 边界校验、Linux 原生套接字选项与 `poll` 就绪轮询、RAII 风格 `SafeSocket` 生命周期管理、带 16KB 软塞（Corking）缓冲的 `TcpStream`、多选项监听配置的 `TcpServer`、无连接 `UdpSocket` 数据报传输、完整 RFC 1928/1929 SOCKS5 与 HTTP `CONNECT` 代理隧道编解码器，以及工业级 TCP 连接池 `ConnectionPool`。核心传输层吸收自 Bun 原生 uSockets 与 Socket 子系统，新增基础设施层 100% 纯 CP 零外部依赖实现。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层网络应用 (Application)                    │
├───────────────────┬─────────────────────┬───────────────────────────────┤
│ 传输与连接池封装   │ 地址/CIDR/代理编解码 │ 套接字选项与旧版兼容层        │
│ TcpServer /       │ Ipv4Addr / Ipv6Addr │ SocketOptions / PollEvent     │
│ TcpStream (Cork)  │ SocketAddress       │ pollSingleFd / pollManyFds    │
│ UdpSocket / Packet│ Ipv4Cidr / Ipv6Cidr │ SafeSocket (RAII)             │
│ ConnectionPool    │ IpClass / Allowlist │ Server / Socket / listen()    │
│                   │ Socks5Codec /       │                               │
│                   │ HttpConnectCodec    │                               │
├───────────────────┴─────────────────────┴───────────────────────────────┤
│   uSockets C-FFI (_usockets.cp) & Zig std.os.linux 原生系统调用层        │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/net/` 下全部 12 个源文件第 1 行均严格标记来源谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/net/_usockets.cp` | `// Source: /data/dev/bun/src/uws_sys/ & /data/dev/bun/src/uws/uws.cp` | 底层 uSockets `@c` FFI 声明、`@opaque` 句柄类型、监听位掩码常量及 `SocketCloseCode` / `SocketKind` / `SendStatus` 枚举 |
| `lib/net/address.cp` | `// Source: /data/dev/bun/src/runtime/socket/SocketAddress.cp` | `AddressFamily`、`Ipv4Addr`、`Ipv6Addr`、`SocketAddress` 零分配解析、私有/回环/组播判定与 IPv4-Mapped IPv6 转换 |
| `lib/net/cidr.cp` | `// Source: cp/lib/net/cidr.cp (RFC 4632 / RFC 4291 / RFC 1918)` | `IpAddr`、`IpKind`、`IpAddrCodec`、`Ipv4Cidr`、`Ipv6Cidr`、`IpClass` 与 `CidrAllowlist`（CIDR 解析/包含/重叠与 SSRF 出站防护） |
| `lib/net/sockopts.cp` | `// Source: cp/lib/net/sockopts.cp (POSIX Socket Options & Linux Poll)` | 基于 `std.os.linux` 的 `SocketOptions`、`setNoDelay`/`setKeepAlive`/`setReuseAddr`/`setReusePort`/`setRecvBufSize`/`setSendBufSize`/`setRecvTimeoutMs`/`setSendTimeoutMs`/`setNonBlocking`/`getSocketError`、`PollEvent` 与 `pollSingleFd`/`pollManyFds` |
| `lib/net/socks5.cp` | `// Source: cp/lib/net/socks5.cp (RFC 1928 / RFC 1929)` | 完整 RFC 1928/1929 `Socks5AuthMethod`、`Socks5ReplyCode`、`Socks5Codec`（Greeting/MethodSelection/UserPassAuth/IPv4+Domain+IPv6 Connect 编解码）与 `HttpConnectCodec` |
| `lib/net/pool.cp` | `// Source: cp/lib/net/pool.cp (Production TCP Connection Pool)` | `ConnPoolConfig`、`PoolEntry`、`PoolStats` 与 `ConnectionPool`（`host:port` 分桶复用、TTL 空闲超时、最大生存期淘汰与容量上限驱逐） |
| `lib/net/proxy.cp` | `// Source: /data/dev/bun/src/http/ProxyTunnel.cp` | 向后兼容 `HttpConnectTunnel`（RFC 7231 `CONNECT` 报文构造与响应解析）及重导出 `Socks5AddrType`、`Socks5Codec` |
| `lib/net/socket.cp` | `// Source: /data/dev/bun/src/uws_sys/socket.cp` | `SafeSocket` 状态机、读写统计 `SocketStats`、`>240s` 自动升级分钟级长超时、半关闭与 RAII `release()`，以及兼容型 `Socket` |
| `lib/net/tcp_stream.cp` | `// Source: /data/dev/bun/src/uws/uws.cp` | 16KB `StreamBuffer`（`STREAM_CORK_BUFFER_SIZE = 16384`）与支持 `cork()` / `uncork()` / `flush()` 批聚合发送的 `TcpStream` |
| `lib/net/tcp_server.cp` | `// Source: /data/dev/bun/src/runtime/socket/Listener.cp` | `ListenConfig`（`toLibusOptions()` 位掩码生成）、`TcpServer` 连接接入计数与生命周期，以及兼容型 `Server` / `listen()` / `connect()` |
| `lib/net/udp.cp` | `// Source: /data/dev/bun/src/uws_sys/udp.cp` | `UdpPacket`、`UdpSocket` 绑定、对端关联（`connectPeer`）、广播/单播 TTL/组播 TTL 与回环选项控制 |
| `lib/net/ipaddr.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 5952 IPv4 / IPv6 Barrel)` | RFC 5952 IPv4/IPv6 地址编解码：`RfcIpv4Addr`、`RfcIpv6Addr`、`IpVersion`、`RfcIpAddr` |
| `lib/net/mac.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (IEEE 802 MAC-48 Barrel)` | IEEE 802 MAC-48 硬件地址解析与格式化：`MacAddr` |
| `lib/net/sockaddr.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (Socket Address Barrel)` | `host:port` / `[ipv6]:port` 零分配解析：`HostKind`、`SocketAddr` |
| `lib/net/proxyv1.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (HAProxy PROXY Protocol v1 Parser & Formatter)` | HAProxy PROXY Protocol v1 文本编解码：`ProxyProtoFamily`、`ProxyV1Header` |
| `lib/net/proxyv2.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (HAProxy PROXY Protocol v2 Binary Codec)` | HAProxy PROXY Protocol v2 二进制编解码与 TLV 迭代：`PROXY_V2_SIGNATURE`、`hasProxyV2Signature`、`ProxyV2Command`、`ProxyV2Family`、`ProxyV2Transport`、`ProxyV2Tlv`、`ProxyV2TlvIter`、`ProxyV2Header` |
| `lib/net/hosts.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (/etc/hosts Parser Barrel)` | `/etc/hosts` 正向/反向零分配查询：`HostsLine`、`HostsIter`、`lookupHost`、`lookupHostV4`、`lookupHostV6`、`reverseLookup` |
| `lib/net/noproxy.cp` | `// Source: Absorbed from /data/dev/bun/src/http/ProxyTunnel.cp` | `NO_PROXY` 规则匹配与 CIDR 绕过：`NoProxyIpv4Addr`、`NoProxyIpv4Cidr`、`shouldBypassProxy` |
| `lib/net/index.cp` | `// Source: /data/dev/bun/src/uws/uws.cp & /data/dev/bun/src/runtime/socket/` | `cap:net` 统一公开导出入口 |

---

## 二、 核心 API 与关键机制

### 1. 地址模型与 CIDR / SSRF 防护（`address.cp` & `cidr.cp`）

- **`Ipv4Addr` / `Ipv6Addr` / `SocketAddress`**：提供 `init`、`loopback`、`any`、`broadcast`、`parse`、`toU32`/`fromU32`、`fromIpv4Mapped`，以及 `socklen()`（IPv4 返回 `16`，IPv6 返回 `28`）。
- **`IpAddrCodec`**：提供纯 CP 无堆分配 IPv6 解析（完整支持 `::` 零压缩与 `::ffff:a.b.c.d` 内嵌 IPv4）、RFC 5952 规范化 IPv6 格式化（`formatV6`）、IPv4 格式化（`formatV4`）与 `u128`/字节数组互转。
- **`Ipv4Cidr` / `Ipv6Cidr`**：支持 `parse("192.168.1.42/24")` / `parse("fe80::/10")`、自动规范化网络地址、`networkAddress()`、`broadcastAddress()`（IPv4）、`prefixLen()`、`contains(ip)`、`overlaps(other)` 与 `format(out)`。
- **`IpClass` 与 `CidrAllowlist`**：
  - `IpClass` 精准判定 `isLoopback`（`127/8`, `::1`）、`isPrivate`（RFC 1918 `10/8`, `172.16/12`, `192.168/16`, RFC 4193 `fc00::/7`）、`isLinkLocal`（`169.254/16`, `fe80::/10`）、`isCgnat`（RFC 6598 `100.64/10`）、`isMulticast`（`224/4`, `ff00::/8`）与 `isGlobalUnicast`；
  - `CidrAllowlist` 支持 `addRule("10.10.0.0/16")` / `addRule("fc00::/7")` / 单 IP 自动补 `/32` 或 `/128`，并提供 `isAllowedOutbound(ip)` 作为开箱即用的 SSRF / 内网边界守卫。

### 2. 真实系统套接字选项与轻量轮询（`sockopts.cp`）

- 全量使用 Zig 0.17 `std.os.linux`（零 `std.posix`、零外部依赖）：
  - `setNoDelay` / `getNoDelay`（`TCP_NODELAY`）
  - `setKeepAlive` / `getKeepAlive`（`SO_KEEPALIVE`）
  - `setReuseAddr` / `getReuseAddr`（`SO_REUSEADDR`）
  - `setReusePort` / `getReusePort`（`SO_REUSEPORT`）
  - `setRecvBufSize` / `getRecvBufSize`（`SO_RCVBUF`）、`setSendBufSize` / `getSendBufSize`（`SO_SNDBUF`）
  - `setRecvTimeoutMs` / `setSendTimeoutMs`（`SO_RCVTIMEO` / `SO_SNDTIMEO`）
  - `setNonBlocking` / `isNonBlocking`（`fcntl` `O_NONBLOCK`）
  - `getSocketError`（`getsockopt` `SO_ERROR`）
  - `SocketOptions` 批量配置结构体（`opts.apply(fd)`）与辅助 `createTcpSocket`、`createSocketPair`、`closeSocketFd`、`writeFd`、`readFd`。
- **`PollEvent` & `pollSingleFd` / `pollManyFds`**：基于 `std.os.linux.poll` / `std.os.linux.pollfd` 实现轻量级 `EINTR` 自重试就绪轮询，支持 `isReadable()`、`isWritable()`、`hasError()`、`isHup()`。

### 3. 完整 SOCKS5 与 HTTP CONNECT 线缆编解码（`socks5.cp` & `proxy.cp`）

- **`Socks5Codec`**：
  - 握手协商：`encodeGreeting(out, methods)`（支持 `NoAuth` `0x00`、`GssApi` `0x01`、`UserPass` `0x02`）与 `decodeMethodSelection(buf)`；
  - RFC 1929 用户名密码子协商：`encodeUserPassAuth(out, user, pass)` 与 `decodeUserPassReply(buf)`；
  - `CONNECT` 请求与响应：`encodeConnectRequest(out, host, port)` 自动识别 IPv4 (`0x01`)、Domain (`0x03`)、IPv6 (`0x04`)；`decodeConnectReply(buf)` 完整解析 `Socks5ReplyCode`、绑定地址（IPv4/Domain/IPv6）及端口；并 100% 保留旧版 `encodeGreetingNoAuth` / `verifyGreetingReply` / `encodeConnectIpv4` / `encodeConnectDomain` / `verifyConnectReply`。
- **`HttpConnectCodec` & `HttpConnectTunnel`**：
  - `HttpConnectCodec.encodeRequest(out, host, port, proxy_auth_header)` 支持 IPv6 自动加方括号 `[::1]:port` 与可选 `Proxy-Authorization` 请求头；`parseResponseStatus(buf)` 解析状态码。

### 4. 生产级 TCP 连接池（`pool.cp`）

- **`ConnPoolConfig` & `ConnectionPool`**：
  - 支持配置 `max_idle_per_host`、`max_total_idle`、`idle_timeout_ms`、`max_lifetime_ms` 与 `close_on_evict` / 自定义关闭回调 `setCloseCallback`；
  - 按 `host:port` 分桶，采用 LIFO（最近归还优先）策略 `acquire(host, port, now_ms)` 复用连接，并自动跳过和关闭已超时/超寿的陈旧 FD；
  - `release` / `releaseFd` / `releaseWithCreated` 自动执行过期清理、单主机上限淘汰与全局容量上限驱逐；
  - `evictExpired(now_ms)`、`hostIdleCount(host, port)`、`stats(): PoolStats`（`hits` / `misses` / `evictions` / `idle_count`）与 `deinit()`。

### 5. `SafeSocket`、`TcpStream` 16KB Corking、`TcpServer` 与 `UdpSocket`

- **`SafeSocket`**：管理 `SocketState`、`SocketStats` 及 uSockets 240 秒自动升级分钟级长超时策略。
- **`TcpStream` & `TcpServer`**：16KB `StreamBuffer` 聚合发送、`ListenConfig.toLibusOptions()` 位掩码映射。
- **`UdpSocket`**：绑定、对端关联、广播及单播/组播 TTL 控制。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    Ipv4Cidr,
    Ipv6Cidr,
    IpAddr,
    IpClass,
    CidrAllowlist,
    PollEvent,
    setNoDelay,
    setNonBlocking,
    pollSingleFd,
    createSocketPair,
    closeSocketFd,
    writeFd,
    readFd,
    Socks5AuthMethod,
    Socks5Codec,
    HttpConnectCodec,
    ConnPoolConfig,
    ConnectionPool,
} from "cap:net";

pub fn demoProductionNet() {
    // 1. CIDR 与 SSRF 安全校验
    const c4 = Ipv4Cidr.parse("192.168.1.42/24").?;
    // c4.networkAddress() == 192.168.1.0, c4.broadcastAddress() == 192.168.1.255
    * = c4.contains(IpAddr.parse("192.168.1.10").?); // 或传 Ipv4Addr
    let allow = CidrAllowlist.init();
    defer allow.deinit();
    * = allow.addRule("10.10.0.0/16");
    const safe_public = allow.isAllowedOutboundStr("8.8.8.8"); // true
    const blocked_meta = allow.isAllowedOutboundStr("169.254.169.254"); // false
    * = safe_public;
    * = blocked_meta;

    // 2. Socketpair + sockopts + pollSingleFd
    const pair = createSocketPair(true).?;
    defer closeSocketFd(pair[0]);
    defer closeSocketFd(pair[1]);
    * = setNonBlocking(pair[0], true);
    * = writeFd(pair[0], "hello");
    const ev = pollSingleFd(pair[1], PollEvent.IN, 100);
    if (ev.isReadable()) {
        let rbuf: [16]u8 = [0; 16];
        * = readFd(pair[1], rbuf[0..16]);
    }

    // 3. SOCKS5 (RFC 1928/1929) & HTTP CONNECT
    let wire: [128]u8 = [0; 128];
    let methods: [2]Socks5AuthMethod = [Socks5AuthMethod.NoAuth, Socks5AuthMethod.UserPass];
    * = Socks5Codec.encodeGreeting(wire[0..128], methods[0..2]);
    * = Socks5Codec.encodeUserPassAuth(wire[0..128], "user", "pass");
    * = Socks5Codec.encodeConnectRequest(wire[0..128], "2001:db8::1", 443);
    * = HttpConnectCodec.encodeRequest(wire[0..128], "api.cp-lang.org", 443, "Basic dXNlcjpwYXNz");

    // 4. ConnectionPool 复用与淘汰
    let pool = ConnectionPool.init(ConnPoolConfig {
        max_idle_per_host: 4,
        max_total_idle: 32,
        idle_timeout_ms: 30000,
        max_lifetime_ms: 300000,
        close_on_evict: false,
    });
    defer pool.deinit();
    * = pool.releaseFd("api.cp-lang.org", 443, 12, 1000);
    const reused = pool.acquire("api.cp-lang.org", 443, 1500);
    * = reused;
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`Bun.listen` / `src/uws`) | Node.js (`node:net` / `node:dgram`) | CP 标准库 (`cap:net`) |
| :--- | :--- | :--- | :--- |
| **地址与 CIDR / SSRF 校验** | `SocketAddress` (`src/runtime/socket/`) | `net.BlockList` / `net.isIP()` | `Ipv4Addr` / `Ipv6Addr` / `Ipv4Cidr` / `Ipv6Cidr` / `IpClass` / `CidrAllowlist` |
| **套接字选项与 Poll** | `uSockets` / `std.posix` | `socket.setNoDelay()` / `setKeepAlive()` | `SocketOptions` / `setNoDelay` / `setNonBlocking` / `getSocketError` / `pollSingleFd` |
| **TCP 服务端监听 & Cork** | `Bun.listen` / `us_socket_stream_buffer_t` | `net.createServer().listen()` / `cork()` | `TcpServer.bind` / `TcpStream.cork()` / `.uncork()` (16KB) |
| **UDP 数据报套接字** | `Bun.udpSocket({ port, socket })` | `dgram.createSocket('udp4')` | `UdpSocket.bind(addr)` / `sendTo` / `setMulticastTTL` |
| **HTTP CONNECT / SOCKS5 代理** | `src/http/ProxyTunnel.cp` | 第三方库（`https-proxy-agent` / `socks`） | `HttpConnectCodec` / `HttpConnectTunnel` / `Socks5Codec` (RFC 1928/1929) |
| **TCP 连接池** | 内置 `HTTPClient` 连接池 | `http.Agent({ keepAlive, maxSockets })` | `ConnectionPool` / `ConnPoolConfig` / `PoolStats` |

---

## 五、 上游高性能设计、安全防护与边界注意事项

1. **16KB 软塞（Corking）批聚合与共享回调模型**：
   - 继承自 Bun / uSockets 的高性能设计：`TcpStream` 内置 `STREAM_CORK_BUFFER_SIZE = 16384`（16KB）软塞缓冲，将多次小包写入在 `cork()` / `uncork()` 间合并为单次系统调用，显著降低内核态切换与 TCP 小包开销；
   - `pollSingleFd` / `pollManyFds` 自动处理 `EINTR` 信号中断重试，避免高并发场景下虚假超时或提前返回。
2. **出站 SSRF 与 IPv4-Mapped IPv6 绕过防护（`CidrAllowlist` & `IpClass`）**：
   - `CidrAllowlist.isAllowedOutbound` / `isAllowedOutboundStr` 默认拦截回环（`127.0.0.0/8`, `::1`）、RFC 1918 私网、链路本地（含云元数据 `169.254.169.254`）、RFC 6598 CGNAT（`100.64.0.0/10`）与组播地址；
   - `IpAddrCodec` 与 `Ipv6Addr.fromIpv4Mapped` 完整识别 `::ffff:a.b.c.d` 内嵌 IPv4 地址，防止利用 IPv6 映射地址绕过 IPv4 黑白名单。
3. **长连接超时分级与连接池双 TTL 淘汰**：
   - `SafeSocket` 遵循 uSockets 规范：当超时设置超过 240 秒时自动升级为分钟级长超时刻度；
   - `ConnectionPool` 按 `host:port` 采用 LIFO（最近归还优先）策略最大化热连接命中率，并在 `acquire`/`release` 时自动淘汰超过 `idle_timeout_ms` 或 `max_lifetime_ms` 的陈旧套接字，避免复用半关闭失效连接。

---

## 六、 收敛集成的网络层、路由、流量遥测与应用层协议栈

以下纯 CP 零外部依赖协议模块已统一聚合导出至 `cap:net`（`lib/net/index.cp`），零符号冲突：

| 分类 | 文件路径 | 协议 / 标准 | 说明 |
| :--- | :--- | :--- | :--- |
| L2/L3/L4 与地址/安全 | `lib/net/arp.cp` | RFC 826 / RFC 5227 | 零堆分配 ARP 报文编解码与免费 ARP/探针检测 |
| L2/L3/L4 与地址/安全 | `lib/net/bgp.cp` | RFC 4271 | 零堆分配 BGP-4 协议帧与路径属性解析器 |
| L2/L3/L4 与地址/安全 | `lib/net/dhcp.cp` | RFC 2131 / RFC 2132 | 零堆分配 DHCPv4 线缆报文与 Option 编解码 |
| L2/L3/L4 与地址/安全 | `lib/net/icmp.cp` | RFC 792 / RFC 4443 | 零堆分配 ICMPv4 / ICMPv6 报文与校验和编解码 |
| L2/L3/L4 与地址/安全 | `lib/net/ipsec.cp` | RFC 4302 / 4303 / 7296 | 零堆分配 IPsec AH / ESP 与 IKEv2 报文解析 |
| 流量遥测与负载均衡 | `lib/net/ipfix.cp` | RFC 7011 | 零堆分配 IPFIX v10 流量遥测线缆解析器 |
| 流量遥测与负载均衡 | `lib/net/netflow.cp` | Cisco NetFlow v5 / RFC 3954 v9 | 零堆分配 NetFlow v5/v9 流量记录解析器 |
| 流量遥测与负载均衡 | `lib/net/haproxy.cp` | HAProxy Config | 零堆分配 HAProxy 配置文件分节与指令解析器 |
| 消息与物联网协议 | `lib/net/amqp.cp` | AMQP 0-9-1 | 二进制帧编解码、FieldTable 与增量流解码器 |
| 消息与物联网协议 | `lib/net/nats.cp` | NATS Wire Protocol | NATS 命令编解码、HPUB/HMSG 头与增量流解析器 |
| 消息与物联网协议 | `lib/net/coap.cp` | RFC 7252 | 零堆分配 CoAP 受限应用协议线缆编解码 |
| 工业控制协议 | `lib/net/bacnet.cp` | ASHRAE 135 | 零堆分配 BACnet/IP BVLC / NPDU / APDU / TLV 编解码 |
| 经典应用与凭证协议 | `lib/net/ftp.cp` | RFC 959 / 2428 / 3659 | 零堆分配 FTP 控制报文、EPSV/PASV 与 MLSD 编解码 |
| 经典应用与凭证协议 | `lib/net/gopher.cp` | RFC 1436 | 零堆分配 Gopher 协议与目录菜单编解码 |
| 经典应用与凭证协议 | `lib/net/imap.cp` | RFC 3501 / RFC 9051 | IMAP4rev1/2 命令编解码、Literal 帧与响应解析 |
| 经典应用与凭证协议 | `lib/net/irc.cp` | RFC 1459 / 2812 / IRCv3 | 零堆分配 IRC 与 IRCv3 Message Tags 线缆编解码 |
| 经典应用与凭证协议 | `lib/net/ldap.cp` | RFC 4514 / RFC 4515 | LDAP 搜索过滤器解析/求值与 DN 编解码 |
| 经典应用与凭证协议 | `lib/net/netrc.cp` | `.netrc` (Absorbed from Bun `npm_client.zig`) | `.netrc` 凭证迭代与主机查找（`NetrcIter` / `lookupNetrc`） |
| 参考归档（不导出） | `lib/net/cidr_.cp` | CIDR / LPM 路由（仅参考） | 与权威 `lib/net/cidr.cp` 功能重叠，保留供对比分析，不在 `cap:net` 重复导出 |
| 邮件与经典协议（第 2 波） | `lib/net/smtp.cp` | RFC 5321 / 2047 | SMTP 命令编解码、点填充、响应解析与 MIME `MailBuilder` |
| 邮件与经典协议（第 2 波） | `lib/net/pop3.cp` | RFC 1939 | 零堆分配 POP3 邮件协议命令与响应编解码 |
| 邮件与经典协议（第 2 波） | `lib/net/whois.cp` | RFC 3912 | 零堆分配 WHOIS 查询与响应字段解析 |
| 邮件与经典协议（第 2 波） | `lib/net/tftp.cp` | RFC 1350 / 2347-2349 | 零堆分配 TFTP 线缆编解码与选项协商 |
| 时间/路由/传输（第 2 波） | `lib/net/ntp.cp` | RFC 5905 | 零堆分配 NTPv4 / SNTP 线缆编解码与时钟偏移计算 |
| 时间/路由/传输（第 2 波） | `lib/net/ospf.cp` | RFC 2328 | 零堆分配 OSPFv2 路由协议报文解析器 |
| 时间/路由/传输（第 2 波） | `lib/net/sctp.cp` | RFC 4960 | 零堆分配 SCTP 报文与 Chunk 解析器 |
| 安全隧道（第 2 波） | `lib/net/wg_replay.cp` / `lib/net/wg_noise.cp` | WireGuard Noise_IKpsk2 | TAI64N 时间戳、滑动窗口重放保护与 BLAKE2s/Noise 握手原语 |
| 实时通信与流媒体（第 2 波） | `lib/net/stun.cp` / `lib/net/turn.cp` | RFC 5389 / RFC 5766 | STUN 绑定/完整性/指纹与 TURN 中继协议编解码 |
| 实时通信与流媒体（第 2 波） | `lib/net/sip.cp` / `lib/net/rtsp.cp` | RFC 3261 / RFC 2326 | SIP 请求响应构建/解析与 RTSP 传输头/交错帧编解码（`RTSP_DEFAULT_MAX_PAYLOAD_BYTES` 别名） |
| 实时通信与流媒体（第 2 波） | `lib/net/rtp.cp` / `lib/net/rtcp.cp` | RFC 3550 | RTP 封包/抖动缓冲与 RTCP 复合报文解析（`RtcpByePacket` 别名） |
| 消息与网管/AAA（第 2 波） | `lib/net/stomp.cp` | STOMP 1.2 | STOMP 帧编解码与增量流解析器（`encodeStompFrame` 别名） |
| 消息与网管/AAA（第 2 波） | `lib/net/snmp.cp` | RFC 1157 / 3416 | 零堆分配 SNMP v1/v2c PDU 与 VarBind 解析器 |
| 消息与网管/AAA（第 2 波） | `lib/net/radius.cp` / `lib/net/diameter.cp` | RFC 2865 / RFC 6733 | 零堆分配 RADIUS 与 Diameter AAA 认证计费协议编解码 |
| 参考归档（不导出） | `lib/net/socks_.cp` / `lib/net/socks5_.cp` | SOCKS4/4a/5（仅参考） | 与权威 `lib/net/socks5.cp` 功能重叠，保留供对比分析，不在 `cap:net` 重复导出 |
| 隧道与封装（第 3 波） | `lib/net/gre.cp` / `lib/net/geneve.cp` / `lib/net/l2tp.cp` | RFC 2784/8926/2661/3931 | GRE、Geneve 与 L2TPv2/v3 隧道编解码（`L2TP_AVP_RESULT_CODE` 别名） |
| 隧道与封装（第 3 波） | `lib/net/mpls.cp` / `lib/net/pppoe.cp` | RFC 3032/4385/5586 & RFC 2516 | MPLS 标签栈/PW-CW/G-ACh 与 PPPoE 发现/会话编解码 |
| 路由与高可用（第 3 波） | `lib/net/isis.cp` / `lib/net/rip.cp` / `lib/net/igmp.cp` / `lib/net/vrrp.cp` | ISO 10589 / RFC 2453 / 3376 / 5798 | IS-IS、RIPv1/v2/RIPng、IGMP v1-v3 与 VRRPv2/v3 编解码 |
| 名称与远程存储（第 3 波） | `lib/net/nbns.cp` / `lib/net/smb.cp` / `lib/net/nfs.cp` / `lib/net/iscsi.cp` | RFC 1001 / MS-SMB2/3 / ONC RPC-NFSv3 / RFC 7143 | NetBIOS、SMB2/3、NFSv3（`NFS_AUTH_NONE` 别名）与 iSCSI 线缆解析 |
| 现代传输与实时通信（第 3 波） | `lib/net/quic.cp` / `lib/net/sdp.cp` | RFC 9000 QUIC & RFC 8866 WebRTC SDP | QUIC 变长整数/帧编解码（`encodeQuicFrame` 别名）与 WebRTC SDP 解析构建 |
| 内容与目录协议（第 3 波） | `lib/net/gemini.cp` / `lib/net/nntp.cp` / `lib/net/ldif.cp` | Gemini / RFC 3977 / RFC 2849 | Gemini/Gemtext、NNTP 与 LDIF 线缆与目录解析 |

---

## 七、 零堆分配流量整形、WDRR 公平调度与 AIMD 拥塞控制（`traffic_shaper.cp`）

面向高并发 API 网关、多租户 Agent 通信与 LLM 客户端自适应背压的三大零堆分配确定性原语：

- **双速三色令牌桶整形器（`TrTcmConfig` / `TrafficColor` / `TrTcmPolicer`）**：严格遵循 RFC 2698（CIR/CBS 承诺速率 + PIR/PBS 峰值速率），亚毫秒余数精确补桶（零浮点误差），O(1) 将报文/请求标记为 `Green`（承诺内放行）、`Yellow`（超承诺但在峰值内降级标记）或 `Red`（超峰值丢弃/限流）。
- **加权赤字轮询公平调度器（`DrrFlowSlot` / `WeightedDrrScheduler`）**：按流权重（`quantum`）公平分配带宽并内建 O(N) 多轮快进防饥饿，彻底杜绝大包/高负载流饿死小包流。
- **AIMD 自适应并发窗口控制器（`AimdConfig` / `AimdLimiter`）**：纯整数加性增（`onSuccess`）/ 乘性减（`onCongestion`，如 429/超时按 `backoff_num / backoff_den` 骤降），严格钳位至 `[min_limit, max_limit]`。

### QuickStart 示例

```cp
import {
    TrafficColor,
    TrTcmConfig,
    TrTcmPolicer,
    DrrFlowSlot,
    WeightedDrrScheduler,
    AimdConfig,
    AimdLimiter,
} from "cap:net";

pub fn demoTrafficShaperQuickStart() {
    // 1. RFC 2698 trTCM 三色标记：CIR=1KB/s, CBS=1KB, PIR=2KB/s, PBS=2KB
    let policer = TrTcmPolicer.init(TrTcmConfig{
        cir_bytes_per_sec: 1000,
        cbs_bytes: 1000,
        pir_bytes_per_sec: 2000,
        pbs_bytes: 2000,
    }, 1000) ?? return;
    const c1 = policer.police(1000, 1000); // TrafficColor.Green
    const c2 = policer.police(800, 1000);  // TrafficColor.Yellow
    const c3 = policer.police(500, 1000);  // TrafficColor.Red
    * = c1; * = c2; * = c3;

    // 2. WDRR 加权公平调度：流 1 (quantum=600) vs 流 2 (quantum=200)
    let slots: [4]DrrFlowSlot = [DrrFlowSlot{}; 4];
    let slots_s: []DrrFlowSlot = slots[0..];
    let sched = WeightedDrrScheduler.init(slots_s) ?? return;
    * = sched.registerFlow(1, 600);
    * = sched.registerFlow(2, 200);
    * = sched.enqueue(1, 200);
    * = sched.enqueue(2, 200);
    * = sched.scheduleNext();

    // 3. AIMD 自适应并发窗口：初始 8，成功 +1，遇 429/超时减半 (1/2)
    let aimd = AimdLimiter.init(AimdConfig{
        initial_limit: 8,
        min_limit: 2,
        max_limit: 32,
        additive_increase: 1,
        backoff_num: 1,
        backoff_den: 2,
    }) ?? return;
    if (aimd.tryAcquire()) {
        * = aimd.onSuccess();    // limit -> 9
    }
    if (aimd.tryAcquire()) {
        * = aimd.onCongestion(); // limit -> 4
    }
}
```

---

## 八、 一致性哈希环、Bounded-Load 防热点与多副本路由器（`consistent_ring.cp`）

面向分布式缓存、AI Agent 集群分片与微服务负载均衡的 100% 纯 CP、零堆分配、O(log V) 二分查找一致性哈希环：

- **`hashRingKey(key: string): u64`**：FNV-1a 64-bit + SplitMix64 雪崩最终化的确定性路由键哈希。
- **`RingPhysicalNode` / `RingVirtualSlot` / `RingStats`**：物理节点（支持 `weight`、`healthy`、`active_load`、`max_load`）、有序虚拟槽位与环遥测快照。
- **`ConsistentHashRing`**：
  - `init(nodes_buf, vslots_buf, vnodes_per_unit): ConsistentHashRing?`：在调用方固定缓冲区零堆初始化。
  - `addNode(*_, node_id, weight, max_load): bool`：按 `weight * vnodes_per_unit` 确定性生成 SplitMix64 虚拟节点令牌并维持升序，拒绝重复 `node_id`、零权重及超容。
  - `removeNode(*_, node_id)` / `setHealthy(*_, node_id, healthy)` / `setLoad(*_, node_id, active_load)` / `getNode(node_id)` / `stats()`。
  - `routeKey(key: string): u32?`：O(log V) 二分顺时针定位首个健康且未过载（`max_load == 0 || active_load < max_load`）的物理节点，内建 Google Bounded-Load 防热点雪崩。
  - `routeReplicas(key: string, replica_count: usize, out_node_ids: *[]u32): usize`：顺时针选取互不重复的物理节点副本列表，天然跳过同一物理机的不同虚拟节点、不健康及过载节点。

### QuickStart 示例

```cp
import {
    RingPhysicalNode,
    RingVirtualSlot,
    ConsistentHashRing,
} from "cap:net";

pub fn demoConsistentHashRingQuickStart() {
    let nodes: [8]RingPhysicalNode = [RingPhysicalNode{}; 8];
    let vslots: [256]RingVirtualSlot = [RingVirtualSlot{}; 256];
    let ring = ConsistentHashRing.init(nodes[0..], vslots[0..], 16) ?? return;

    // 注册 3 个带权重和 Bounded-Load 上限的物理节点
    * = ring.addNode(1, 2, 100); // 32 vnodes, max_load=100
    * = ring.addNode(2, 1, 100); // 16 vnodes, max_load=100
    * = ring.addNode(3, 1, 100); // 16 vnodes, max_load=100

    // O(log V) 单节点路由
    if (primary := ring.routeKey("agent-session:42")) {
        * = primary;
    }

    // 顺时针去重 3 副本路由（自动跳过同一物理机的多个虚拟节点）
    let replicas: [3]u32 = [0; 3];
    const n = ring.routeReplicas("agent-session:42", 3, replicas[0..]);
    * = n;

    // 热点过载触发 Bounded-Load 自动顺延至下一个可用物理节点
    * = ring.setLoad(1, 100);
    * = ring.routeKey("agent-session:42");
}
```

---

## 九、 二进制多路复用流帧编解码与流控状态机（Yamux 风格，`stream_mux.cp`）

面向高性能 RPC、跨节点 Agent 通信与单 TCP 连接多路复用的 100% 纯 CP、零堆分配 12 字节大端帧编解码与信用窗口流控引擎：

- **12 字节大端固定帧头与零拷贝编解码**：
  - 常量：`MUX_PROTO_VERSION`（`0`）、`MUX_HEADER_LEN`（`12`）、`MUX_DEFAULT_INITIAL_WINDOW`（`256 KiB`）、`MUX_GOAWAY_NORMAL` / `MUX_GOAWAY_PROTOCOL_ERROR` / `MUX_GOAWAY_INTERNAL_ERROR`。
  - 类型：`MuxFrameType`（`Data | WindowUpdate | Ping | GoAway`）、`MuxFlags`（`NONE` / `SYN` / `ACK` / `FIN` / `RST`）、`MuxFrameHeader`、`MuxDecodedFrame`、`MuxCodecError`。
  - 函数：`encodeMuxFrameHeader` / `decodeMuxFrameHeader` / `encodeMuxFrame` / `decodeMuxFrame`（严格校验版本号、标志位掩码、会话级/流级 `stream_id` 合法性及载荷截断）。
- **多路复用会话与双向信用流控（`MuxSession`）**：
  - `init(streams_buf, is_client, max_concurrent_streams, initial_window): MuxSession?`：客户端自动分配奇数流 ID（`1, 3, 5...`）、服务端分配偶数流 ID（`2, 4, 6...`）。
  - `openStream` / `acceptRemoteStream`：并发流上限校验与对端奇偶性校验。
  - `canSend` / `consumeSendWindow` / `onRecvData` / `onWindowUpdate` / `grantRecvWindow`：精确双向 `send_window`/`recv_window` 信用扣减、超窗拦截与 `WindowUpdate` 补窗（防 `u32` 溢出）。
  - `closeLocal` / `resetStream` / `sendGoAway` / `onGoAway`：完整支持全双工半关闭（`LocalHalfClosed` / `RemoteHalfClosed`）、`RST` 立即关闭与优雅停机 `GoAway`（拒绝新开流但放行在途流正常收尾）。

### QuickStart 示例

```cp
import {
    MUX_PROTO_VERSION,
    MuxFrameType,
    MuxFlags,
    MuxFrameHeader,
    encodeMuxFrame,
    decodeMuxFrame,
    MuxStreamSlot,
    MuxSession,
} from "cap:net";

pub fn demoStreamMuxQuickStart()! {
    let slots: [8]MuxStreamSlot = [MuxStreamSlot{}; 8];
    let client = MuxSession.init(slots[0..], true, 8, 4096) ?? return;

    // 1. 客户端自动分配奇数流 ID = 1 并扣减发送窗口
    const sid = client.openStream() ?? return;
    const body: []const u8 = "hello-rpc";
    if (client.consumeSendWindow(sid, body.len as u32)) {
        let wire: [64]u8 = [0; 64];
        const hdr = MuxFrameHeader{
            version: MUX_PROTO_VERSION,
            frame_type: MuxFrameType.Data,
            flags: MuxFlags.SYN,
            stream_id: sid,
            length: body.len as u32,
        };
        const n = encodeMuxFrame(hdr, body, wire[0..])?;
        const decoded = decodeMuxFrame(wire[0..n])?;
        * = decoded.payload; // 零拷贝切片 "hello-rpc"
    }

    // 2. 收到对端 WindowUpdate 补窗后恢复发送信用
    * = client.onWindowUpdate(sid, 1024, MuxFlags.NONE);
}
```
