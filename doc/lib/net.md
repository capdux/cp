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
| `lib/net/tcpStream.cp` | `// Source: /data/dev/bun/src/uws/uws.cp` | 16KB `StreamBuffer`（`STREAM_CORK_BUFFER_SIZE = 16384`）与支持 `cork()` / `uncork()` / `flush()` 批聚合发送的 `TcpStream` |
| `lib/net/tcpServer.cp` | `// Source: /data/dev/bun/src/runtime/socket/Listener.cp` | `ListenConfig`（`toLibusOptions()` 位掩码生成）、`TcpServer` 连接接入计数与生命周期，以及兼容型 `Server` / `listen()` / `connect()` |
| `lib/net/udp.cp` | `// Source: /data/dev/bun/src/uws_sys/udp.cp` | `UdpPacket`、`UdpSocket` 绑定、对端关联（`connectPeer`）、广播/单播 TTL/组播 TTL 与回环选项控制 |
| `lib/net/ipaddr.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 5952 IPv4 / IPv6 Barrel)` | RFC 5952 IPv4/IPv6 地址编解码：`RfcIpv4Addr`、`RfcIpv6Addr`、`IpVersion`、`RfcIpAddr` |
| `lib/net/mac.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (IEEE 802 MAC-48 Barrel)` | IEEE 802 MAC-48 硬件地址解析与格式化：`MacAddr` |
| `lib/net/sockaddr.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (Socket Address Barrel)` | `host:port` / `[ipv6]:port` 零分配解析：`HostKind`、`SocketAddr` |
| `lib/net/proxyv1.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (HAProxy PROXY Protocol v1 Parser & Formatter)` | HAProxy PROXY Protocol v1 文本编解码：`ProxyProtoFamily`、`ProxyV1Header` |
| `lib/net/proxyv2.cp` | `// Source: Absorbed from /data/dev/bun/src/http/http.cp (HAProxy PROXY Protocol v2 Binary Codec)` | HAProxy PROXY Protocol v2 二进制编解码与 TLV 迭代：`PROXY_V2_SIGNATURE`、`hasProxyV2Signature`、`ProxyV2Command`、`ProxyV2Family`、`ProxyV2Transport`、`ProxyV2Tlv`、`ProxyV2TlvIter`、`ProxyV2Header` |
| `lib/net/hosts.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (/etc/hosts Parser Barrel)` | `/etc/hosts` 正向/反向零分配查询：`HostsLine`、`HostsIter`、`lookupHost`、`lookupHostV4`、`lookupHostV6`、`reverseLookup` |
| `lib/net/noproxy.cp` | `// Source: Absorbed from /data/dev/bun/src/http/ProxyTunnel.cp` | `NO_PROXY` 规则匹配与 CIDR 绕过：`NoProxyIpv4Addr`、`NoProxyIpv4Cidr`、`shouldBypassProxy` |
| `lib/net/blockList.cp` | `// Source: Absorbed from /data/dev/bun/src/runtime/node/net/BlockList.cp` | Node/Bun `net.BlockList` 纯 CP 零分配 IP 访问控制表：`BlockListFamily`、`BlockListRuleKind`、`BlockListError`、`BlockListAddr`、`BlockListRule`、`BlockList`（单地址/范围/CIDR 子网规则、IPv4-mapped IPv6 双向匹配、规则格式化导出） |
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
| 安全隧道（第 2 波） | `lib/net/wgReplay.cp` / `lib/net/wgNoise.cp` | WireGuard Noise_IKpsk2 | TAI64N 时间戳、滑动窗口重放保护与 BLAKE2s/Noise 握手原语 |
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

## 七、 零堆分配流量整形、WDRR 公平调度与 AIMD 拥塞控制（`shaper.cp`）

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

## 八、 一致性哈希环、Bounded-Load 防热点与多副本路由器（`hashRing.cp`）

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

## 九、 二进制多路复用流帧编解码与流控状态机（Yamux 风格，`streamMux.cp`）

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

---

## 十、 零分配 P2C + Peak-EWMA 自适应负载均衡器与异常摘除引擎（`p2c.cp`）

面向微服务网关、RPC 集群与多节点 LLM 推理后端的 Linkerd / Envoy 级 Power-of-Two-Choices (P2C) + Peak-EWMA 自适应负载均衡与被动异常摘除（Outlier Ejection）引擎，100% 纯 CP 零动态堆分配（固定 64 节点内联数组 `MAX_P2C_BACKENDS = 64`）：

- **核心类型**：`BackendHealthState`（`healthy | ejected | half_open`）、`P2cBalancerConfig`（`ewma_alpha_f32`, `max_consecutive_errors`, `ejection_cooldown_ms`, `max_ejection_percent`, `inflight_penalty_us`）、`BackendSlot`、`P2cPickDecision`（含 `used_fallback_panic_mode`）、`P2cBalancerStats`、`P2cLoadBalancer`。
- **核心算法**：
  - `pick_backend(*_, now_ms, rng_seed): P2cPickDecision`：自动将冷却到期 `ejected` 节点转为 `half_open` 探活；通过 SplitMix64 抽样两个不同可用节点并按 `effective_load = ewma_latency_us * (1 + inflight) / weight + inflight * inflight_penalty_us`（`@fma` 计算）择低路由并递增 `inflight`；若所有节点不可用则自动进入 **Panic Mode** 降级兜底路由。
  - `report_outcome(*_, slot_idx, ok, latency_us, now_ms): bool`：递减 `inflight`，采用 Peak-EWMA（延迟尖峰立即跃升、恢复时按 `@fma` 指数衰减）平滑延迟；连续失败达 `max_consecutive_errors` 且未超 `max_ejection_percent` 防雪崩上限时自动摘除；`half_open` 探活成功立即恢复 `healthy`。

### QuickStart 示例

```cp
import {
    P2cBalancerConfig,
    P2cLoadBalancer,
} from "cap:net";

pub fn demoP2cLoadBalancer() {
    let lb = P2cLoadBalancer.init(P2cBalancerConfig{
        ewma_alpha_f32: 0.25,
        max_consecutive_errors: 3,
        ejection_cooldown_ms: 10000,
        max_ejection_percent: 50,
        inflight_penalty_us: 5000.0,
    }) ?? return;

    const n1 = lb.register_backend("gpu-node-1", 2) ?? return;
    const n2 = lb.register_backend("gpu-node-2", 1) ?? return;

    const pick = lb.pick_backend(1000, 42);
    if (pick.found) {
        * = lb.report_outcome(pick.slot_idx, true, 12000, 1012);
    }
    * = n1;
    * = n2;
    * = lb.stats();
}
```

---

## 十一、 零分配 RFC 9002 拥塞控制与 SRTT/PTO/Pacing 传输调度引擎（`congestion.cp`）

面向 QUIC / HTTP/3、自定义 UDP 传输与高并发 AI Agent RPC 的 100% 纯 CP 零动态堆分配 RFC 9002 NewReno 拥塞控制、Jacobson/Karels RTT 估算与字节级 Pacing 速率调度引擎：

- **核心类型**：
  - `CongestionPhase`（`slow_start | congestion_avoidance | recovery`）
  - `CongestionConfig`（`max_datagram_size` [默认 `1200`]、`initial_cwnd_bytes` [默认 `12000`]、`min_cwnd_bytes` [默认 `2400`]、`initial_rtt_us` [默认 `333000`]、`loss_reduction_bps` [默认 `5000` 即 0.5x]、`pacing_gain_bps` [默认 `12500` 即 1.25x]，提供 `defaults()` 与 `isValid()`）
  - `RttEstimate`（`has_sample`, `latest_rtt_us`, `min_rtt_us`, `smoothed_rtt_us`, `rttvar_us`，提供 RFC 9002 §6.2.1 `pto_us(max_ack_delay_us): u64`）
  - `CongestionStats`（完整拥塞窗口、阶段、在途字节、可用窗口、Pacing 速率、SRTT/min_rtt 与丢包/持续拥塞计数）
- **核心引擎（`Rfc9002CongestionController`）**：
  - `init(cfg): Rfc9002CongestionController?`
  - `on_rtt_sample(*_, latest_rtt_us, ack_delay_us, max_ack_delay_us)`：严格遵循 RFC 9002 §5.3 更新 `min_rtt`、`adjusted_rtt`（仅当 `latest_rtt >= min_rtt + min(ack_delay, max_ack_delay)` 时扣减）、`smoothed_rtt`（7/8 + 1/8）与 `rttvar`（3/4 + 1/4）。
  - `on_packet_sent(*_, bytes)` / `can_send(bytes): bool` / `available_window(): u64`
  - `on_packets_acked(*_, acked_bytes, sent_time_us, is_app_limited)`：防下溢扣减 `bytes_in_flight`；`is_app_limited` 或 `sent_time_us <= recovery_start_us` 抑制扩窗；否则 Slow Start 指数扩窗、Congestion Avoidance 按 `max_datagram_size * acked_bytes / cwnd` 线性扩窗。
  - `on_congestion_event(*_, lost_bytes, largest_lost_sent_us, now_us)`：单 RTT 恢复窗口防重复腰斩守卫，首次触发进入 `recovery` 并按 `loss_reduction_bps` 缩减 `ssthresh`/`cwnd`（不低于 `min_cwnd_bytes`）。
  - `on_persistent_congestion(*_)`（RFC 9002 §7.6 坍缩 `cwnd` 至 `min_cwnd_bytes` 并清零恢复状态）、`pacing_rate_bytes_per_sec(): u64`（`u128` 防溢出计算）、`stats(): CongestionStats`。

### QuickStart 示例

```cp
import {
    CongestionConfig,
    Rfc9002CongestionController,
} from "cap:net";

pub fn demoRfc9002CongestionCtrl() {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;

    // 1. 首包 RTT 采样 (40ms) -> srtt=40ms, rttvar=20ms, pto=40+80+25=145ms
    cc.on_rtt_sample(40000, 0, 25000);
    const pto = cc.rtt.pto_us(25000);

    // 2. 发送 2400B 并收到确认 -> Slow Start 指数扩窗
    if (cc.can_send(2400)) {
        cc.on_packet_sent(2400);
        cc.on_packets_acked(2400, 1000, false); // cwnd: 12000 -> 14400
    }

    // 3. 丢包进入 Recovery（cwnd 减半为 7200），同 RTT 再次丢包不重复腰斩
    cc.on_congestion_event(1200, 2000, 5000);
    const rate = cc.pacing_rate_bytes_per_sec();
    * = pto;
    * = rate;
    * = cc.stats();
}
```

---

## RFC 8305 Happy Eyeballs v2 双栈 IPv6/IPv4 连接竞速状态机 (`happyEye.cp`)

面向现代双栈 TCP/QUIC 客户端的 100% 纯 CP 零动态堆分配 RFC 8305 Happy Eyeballs v2 竞速与回退调度状态机（固定容量 `HE_MAX_CANDIDATES = 32`）：

- **常量与枚举**：
  - `HE_MAX_CANDIDATES = 32`、`HE_DEFAULT_RESOLUTION_DELAY_MS = 50`、`HE_DEFAULT_ATTEMPT_DELAY_MS = 250`、`HE_DEFAULT_FIRST_FAMILY_COUNT = 1`、`HE_INVALID_ATTEMPT_ID = 0xFFFF`
  - `HeAddressFamily`（`ipv6 | ipv4`）、`HeAttemptState`（`pending | in_flight | succeeded | failed | cancel_pending | canceled`）、`HeActionKind`（`start_attempt | wait_until | connected | all_failed`）
- **数据模型**：`HeCandidate`（`fromIpv4` / `fromIpv6` POD 端点）、`HeConfig`（`defaults()` / `isValid()`）、`HeAction`、`HeStats`
- **核心引擎 `HappyEyeballsEngine`**：
  - `init(cfg, start_ms): HappyEyeballsEngine?`
  - `addResolvedAddresses(*_, family, candidates, dns_complete, now_ms)` / `markDnsComplete(*_, family, now_ms)`：异步分批接收 AAAA / A 记录、自动去重与满容量防护；非首选族先到达时自动触发 RFC 8305 §3 Resolution Delay（默认 50ms），首选族到达或 DNS 结束时立即解除等待。
  - `previewInterleavedOrder(out_order)`：预览 RFC 8305 §4 首选族优先 + 双栈交替排序结果。
  - `nextAction(*_, now_ms): HeAction`：单调时间片推进决策（`start_attempt` / `wait_until(deadline_ms)` / `connected` / `all_failed`），默认按 250ms Connection Attempt Delay 错峰并发拨号。
  - `markAttemptFailed(*_, attempt_id, now_ms)`：单路失败立即清零错峰等待计时器，第一时间触发下一候选。
  - `markAttemptSucceeded(*_, attempt_id)` / `collectAttemptsToCancel(*_, out_attempt_ids)`：任一连接成功即胜出并自动将所有其他 `in_flight` 尝试标记为待取消，支持分批收集取消列表。

```cp
import {
    HeAddressFamily,
    HeActionKind,
    HeCandidate,
    HeConfig,
    HappyEyeballsEngine,
} from "cap:net";

pub fn demoHappyEyeballs() {
    let he = HappyEyeballsEngine.init(HeConfig.defaults(), 1000) ?? return;

    const v6: [1]HeCandidate = [HeCandidate.fromIpv6([0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], 443, 1)];
    const v4: [1]HeCandidate = [HeCandidate.fromIpv4([93, 184, 216, 34], 443, 2)];
    * = he.addResolvedAddresses(HeAddressFamily.ipv6, v6[0..], true, 1000);
    * = he.addResolvedAddresses(HeAddressFamily.ipv4, v4[0..], true, 1000);

    const a1 = he.nextAction(1000); // start_attempt IPv6
    const a2 = he.nextAction(1250); // 250ms 后错峰 start_attempt IPv4
    if (a2.kind == HeActionKind.start_attempt) {
        * = he.markAttemptSucceeded(a2.attempt_id);
        let to_cancel: [4]u16 = [0; 4];
        let cancel_s: []u16 = to_cancel[0..];
        const n_cancel = he.collectAttemptsToCancel(*cancel_s); // 1 (a1.attempt_id)
        * = n_cancel;
    }
    * = a1;
}
```

## 自适应网络重试退避与全局防重试风暴预算门控 (`retry.cp`)

面向 Google SRE / Envoy / AWS Architecture 防级联雪崩模型的 100% 纯 CP 零动态堆分配自适应重试退避、服务端 `Retry-After` 夹紧与令牌桶式全局重试预算门控引擎：

- **常量与枚举**：
  - `RETRY_BPS_DENOM = 10_000`、`RETRY_DEFAULT_RATIO_BPS = 2_000`（20%）、`RETRY_DEFAULT_MAX_DELAY_MS = 30_000`
  - `RetryJitterMode`（`none | full_jitter | equal_jitter | decorrelated_jitter`，底层确定性 SplitMix64 PRNG）
  - `RetryErrorClass`（可重试：`transient_network | timeout | rate_limited_429 | server_5xx`；不可重试：`client_4xx | cancelled`）
  - `RetryDecisionKind`（`retry_allowed | reject_non_retryable | reject_max_attempts | reject_deadline_exceeded | reject_budget_exhausted`）
- **配置与纯函数**：
  - `RetryPolicyConfig`（`max_attempts`, `base_delay_ms`, `max_delay_ms`, `jitter_mode`, `respect_retry_after`, `max_retry_after_ms`；`defaults()` / `isValid()`）
  - `RetryBudgetConfig`（`retry_ratio_bps`, `min_retries_per_sec`, `max_budget_tokens`, `initial_tokens`；`defaults()` / `isValid()`）
  - `RetryDecision`、`RetryControllerStats`
  - `isRetryableError(err_class)` / `computeBackoffDelayMs(cfg, attempt_idx, prev_delay_ms, rng_state: *u64): u64`（溢出保护饱和指数退避 + 4 种抖动）
- **核心引擎 `AdaptiveRetryController`**：
  - `init(policy, budget, rng_seed, start_ms): AdaptiveRetryController?`
  - `recordNormalRequest(*_, now_ms)`：按 `retry_ratio_bps` 与经过时间 `min_retries_per_sec` 补充预算令牌（带时钟回退保护与桶上限夹紧）。
  - `evaluateRetry(*_, err_class, attempt_idx, prev_delay_ms, retry_after_ms, now_ms, deadline_ms): RetryDecision`：依次执行错误可重试校验 -> 尝试次数校验 -> `Retry-After` 夹紧/抖动退避计算 -> 截止时间防溢出校验 -> 全局预算扣减（仅最终 `retry_allowed` 时消耗令牌）。
  - `availableBudgetTokens()` / `stats()`

```cp
import {
    RetryErrorClass,
    RetryDecisionKind,
    RetryPolicyConfig,
    RetryBudgetConfig,
    AdaptiveRetryController,
} from "cap:net";

pub fn demoAdaptiveRetry() {
    let ctrl = AdaptiveRetryController.init(
        RetryPolicyConfig.defaults(),
        RetryBudgetConfig.defaults(),
        42,
        10_000,
    ) ?? return;

    ctrl.recordNormalRequest(10_000);
    const decision = ctrl.evaluateRetry(RetryErrorClass.rate_limited_429, 1, 0, 1500, 10_000, 15_000);
    if (decision.kind == RetryDecisionKind.retry_allowed) {
        * = decision.delay_ms; // 1500 (采纳并夹紧 Retry-After)
    }
}
```

---

## 零分配上游集群离群驱逐、熔断器与半开探测池（`circuitPool.cp`）

对标 Envoy Outlier Detection & Circuit Breaker 的 100% 纯 CP 零动态堆分配上游多节点健康治理与加权路由池：

- **常量与枚举**：
  - `CIRCUIT_POOL_MAX_ENDPOINTS = 64`、`CIRCUIT_POOL_BPS_DENOM = 10_000`、`CIRCUIT_POOL_WINDOW_BUCKETS = 8`
  - `CircuitEndpointState`（`healthy | ejected_open | half_open_probe`）
  - `CircuitOutcomeKind`（`success | server_error_5xx | timeout | connect_failure | client_error_4xx`）
  - `CircuitPickMode`（`normal_healthy | half_open_probe | panic_fallback | empty_pool`）
- **配置与视图**：
  - `CircuitPoolConfig`：`consecutive_5xx_threshold`、`error_rate_threshold_bps`、`min_request_volume`、`window_ms`、`base_ejection_ms`、`max_ejection_ms`、`max_ejection_percent_bps`（默认 `5000` 即 50% 上限）、`half_open_success_threshold`
  - `CircuitEndpointView`、`CircuitPickResult`、`CircuitPoolStats`
- **核心引擎（`CircuitBreakerPool`）**：
  - `init(config)` / `addEndpoint(*_, endpoint_id, weight)` / `removeEndpoint(*_, endpoint_id)` / `updateWeight(*_, endpoint_id, new_weight)`
  - `recordOutcome(*_, endpoint_id, outcome, now_ms)`：维护 8 桶滑动窗口错误率 `error_rate_bps` 与连续 5xx/超时/连接失败计数（免疫 4xx）；支持 Envoy 最大驱逐比例上限保护与递增冷却时长 `min(base_ejection_ms * ejection_count, max_ejection_ms)`；单调时钟防回退。
  - `advanceClock(*_, now_ms)` / `forceRecover(*_, endpoint_id)`：到期自动转 `half_open_probe` 放行单路探测，探测成功恢复 `healthy`。
  - `pickHealthyEndpoint(*_, rng_seed, now_ms)`：优先放行单路半开探测、正常按权重选择健康节点、全驱逐时自动触发 `panic_fallback` 防止级联断流。

### Upstream Circuit Pool QuickStart 示例

```cp
import {
    CircuitOutcomeKind,
    CircuitPickMode,
    CircuitPoolConfig,
    CircuitBreakerPool,
} from "cap:net";

pub fn demoCircuitPool() {
    let pool = CircuitBreakerPool.init(CircuitPoolConfig.defaults()) ?? return;
    * = pool.addEndpoint(101, 50);
    * = pool.addEndpoint(102, 50);

    * = pool.recordOutcome(101, CircuitOutcomeKind.server_error_5xx, 1_000);
    const pick = pool.pickHealthyEndpoint(42, 1_500);
    if (pick.mode == CircuitPickMode.normal_healthy) {
        * = pick.endpoint_id;
    }
}
```

---

## 自适应尾延迟对冲请求与放大预算调度器 (`hedgedReq.cp`)

面向 *"The Tail at Scale"* 尾延迟优化的 100% 纯 CP 零动态堆分配自适应对冲请求与放大预算控制引擎：

- **常量与枚举**：
  - `HEDGE_MAX_INFLIGHT = 64`、`HEDGE_MAX_REPLICAS = 4`（1 主 + 3 备选）、`HEDGE_LATENCY_WINDOW = 64`、`HEDGE_BPS_DENOM = 10000`
  - `HedgeAttemptState`（`pending | in_flight | succeeded | failed | cancelled`）、`HedgeReqState`（`in_flight | completed_primary | completed_hedged | failed_all | timed_out`）、`HedgeCompletionKind`（`winner_primary | winner_hedged | attempt_failed_continue | all_attempts_failed | ignored_stale`）
- **核心组件与调度器**：
  - `TailLatencyEstimator`：定长 64 样本环形窗，冷启动返回 `default_delay_ms`，满样本后动态计算 P95/P99 并钳位至 `[min_delay_ms, max_delay_ms]`。
  - `HedgeAmplificationBudget`：高精度基点令牌桶，默认限制额外对冲流量不超过主请求流量的 10%（`1000 bps`），过载时自动抑制对冲防雪崩。
  - `HedgedRequestScheduler`：`init(cfg)`、`startRequest(*_, req_id, candidate_nodes, timeout_ms, now_ms)`、`pollDueHedges(*_, now_ms, *out_launches)`、`completeAttempt(*_, req_id, attempt_idx, succeeded, now_ms)`（首胜自动返回待取消的慢副本列表 `cancelNodesSlice()`，主副本失败立即触发备选 Fail-Fast）、`reapTimedOut(*_, now_ms, *out_ids)`、`currentHedgeDelayMs()`、`recordLatencySample(*_, lat_ms)`、`stats()`。

```cp
import {
    HedgeCompletionKind,
    HedgeSchedulerConfig,
    HedgeLaunchEvent,
    HedgedRequestScheduler,
} from "cap:net";

pub fn demoHedgedRequestScheduler() {
    let sched = HedgedRequestScheduler.init(HedgeSchedulerConfig.defaults()) ?? return;
    const nodes: [2]u64 = [101, 102];
    * = sched.startRequest(1, nodes[0..], 500, 1000);

    let launches: [4]HedgeLaunchEvent = [HedgeLaunchEvent{}; 4];
    let launches_s: []HedgeLaunchEvent = launches[0..];
    if (sched.pollDueHedges(1050, *launches_s) > 0) {
        const res = sched.completeAttempt(1, 1, true, 1065);
        if (res.kind == HedgeCompletionKind.winner_hedged) {
            * = res.cancelNodesSlice(); // [101] (取消慢主副本)
        }
    }
}
```

---

## IP 访问控制表 (`blockList.cp`) 与 Bun TCP 实战指南

> 同步自 `/data/dev/bun/src/runtime/node/net/BlockList.cp` 与 `/data/dev/bun/docs/runtime/networking/tcp.mdx`

### 1. `BlockList` 零分配 IPv4/IPv6 访问控制表

`lib/net/blockList.cp` 提供与 Node.js / Bun `net.BlockList` 语义对齐的 100% 纯 CP 零堆分配实现（固定容量 `BLOCKLIST_MAX_RULES = 256`）：

- **规则类型与 LIFO 优先级**：支持单地址（`addAddress` / `addAddressAuto`）、闭区间范围（`addRange` / `addRangeAuto`，严格校验 `start <= end` 与同协议族）以及 CIDR 子网（`addSubnet` / `addSubnetCidr`，IPv4 `0..32`、IPv6 `0..128`，含 `/0` 移位溢出防护）。新规则按 LIFO 顺序头插至索引 `0`，与 Bun `da_rules.insert(0, ...)` 一致。
- **IPv4-Mapped IPv6 双向互通**：`::ffff:a.b.c.d` 与原生 IPv4 在单地址、范围、子网匹配中自动互通；`check` 对畸形 IP 字符串返回 `false` 而不触发异常。
- **规则格式化导出**：`formatRule(idx, out)` 与 `formatAllRules(out)` 严格输出 Node/Bun 标准格式：`"Address: IPv4 127.0.0.1"`、`"Range: IPv4 10.0.0.1-10.0.0.10"`、`"Subnet: IPv6 2001:db8::/32"`。

```typescript
import { BlockList, BlockListFamily } from "cap:net";

let bl = BlockList.init();
bl.addAddress("127.0.0.1", BlockListFamily.Ipv4)?;
bl.addRange("10.0.0.1", "10.0.0.10", BlockListFamily.Ipv4)?;
bl.addSubnetCidr("2001:db8::/32")?;

const hit_v4 = bl.check("10.0.0.5", BlockListFamily.Ipv4);          // true
const hit_mapped = bl.check("::ffff:127.0.0.1", BlockListFamily.Ipv6); // true
const hit_v6 = bl.check("2001:db8::cafe", BlockListFamily.Ipv6);    // true

let buf: [256]u8 = [0; 256];
if (n := bl.formatAllRules(buf[0..])) {
    const summary = buf[0..n]; // 换行分隔的规范规则列表
    * = summary;
}
```

### 2. 高性能 TCP 服务端与客户端最佳实践（同步自 Bun `tcp.mdx`）

1. **按服务器共享事件处理器（避免单连接闭包分配）**：在高频连接场景中，将 `open` / `data` / `drain` / `close` / `error` 回调声明在服务器级共享结构中，而非为每个连接分配独立监听器；连接上下文通过 `open` 阶段绑定的轻量句柄数据结构承载。
2. **发送批聚合（Corking）与背压控制（Drain）**：避免逐字节或高频碎片化 `write()`（如拆分成多次 1 字节写），优先使用 `TcpStream` 的 16KB `StreamBuffer`（`cork()` / `uncork()` / `flush()`）合并小包；当底层返回短写（`wrote < payload.len`）时，将剩余切片保留在缓冲中，等待 `drain` 可写事件再续传。
3. **连接准入防护**：在 `TcpServer` 接受新连接后，优先用 `BlockList.checkAuto` / `CidrAllowlist` 对对端地址执行零分配 O(N) 准入校验，在应用层解析前即可拦截受限来源。

---

## 零分配 L4 双向五元组连接跟踪表与半连接配额防护引擎 (`connTrack.cp`)

面向 L4 网关、状态防火墙与资源耗尽防护的 100% 纯 CP 零动态堆分配 Conntrack 连接跟踪引擎：

- **常量与枚举**：
  - `CONNTRACK_MAX_FLOWS = 128`、`CONNTRACK_PROTO_TCP = 6`、`CONNTRACK_PROTO_UDP = 17`、`CT_TCP_FIN`、`CT_TCP_SYN`、`CT_TCP_RST`、`CT_TCP_ACK`
  - `ConnTrackState`（`syn_sent | syn_recv | established | fin_wait | time_wait | udp_unreplied | udp_assured | closed`）
  - `ConnTrackDir`（`original | reply`）、`ConnTrackAction`（`created | updated | rejected_half_open_quota | rejected_table_full | rejected_invalid`）
- **数据模型**：
  - `ConnTrackTuple`：IPv4/IPv6 归一化 16 字节地址（IPv4 自动映射 `::ffff:a.b.c.d`），提供 `fromIpv4`、`fromIpv6`、`reverse()`、`equals(other)` 与双向对称哈希 `symmetricHash()`（正反向回包哈希恒等）。
  - `ConnTrackConfig`（`max_half_open` 与各状态独立空闲超时；`defaults()` / `isValid()`）
  - `ConnTrackEntry`（双向 `orig_packets`/`orig_bytes`、`reply_packets`/`reply_bytes`、时间戳与状态）
  - `ConnTrackOutcome` / `ConnTrackStats`
- **核心引擎 `ConnTrackTable`**：
  - `init(cfg)` / `reset(*_)` / `lookup(tuple)` / `removeFlow(*_, flow_id)` / `stats()`
  - `observePacket(*_, tuple, tcp_flags, packet_bytes, now_ms)`: 双向五元组精确命中并区分 `original`/`reply`，驱动完整 TCP 三次握手/挥手/RST 与 UDP 应答晋升状态机；内置半连接（`syn_sent`/`syn_recv`）配额上限防护；满 128 槽时优先回收最旧的 `closed` / `time_wait` 槽位。
  - `reapExpired(*_, now_ms)`: 按各流当前状态独立空闲超时清扫过期流并同步维护活跃/半连接计数。

```cp
import {
    CONNTRACK_PROTO_TCP,
    CT_TCP_SYN,
    CT_TCP_ACK,
    ConnTrackConfig,
    ConnTrackTuple,
    ConnTrackTable,
} from "cap:net";

pub fn demoConnTrack() {
    let ct = ConnTrackTable.init(ConnTrackConfig.defaults()) ?? return;
    const c2s = ConnTrackTuple.fromIpv4(CONNTRACK_PROTO_TCP, [10, 0, 0, 1], 51000, [10, 0, 0, 2], 443);
    const s2c = c2s.reverse();

    * = ct.observePacket(c2s, CT_TCP_SYN, 60, 1000);                // created: syn_sent (original)
    * = ct.observePacket(s2c, CT_TCP_SYN | CT_TCP_ACK, 60, 1010);   // updated: syn_recv (reply)
    * = ct.observePacket(c2s, CT_TCP_ACK, 52, 1020);                // updated: established (original)
    * = ct.stats();
}
```

---

## 零分配多可用区拓扑感知与 Envoy 风格 Locality 溢出负载均衡路由器 (`zoneRouter.cp`)

面向多可用区（Multi-AZ）云原生服务网格降低跨区流量成本与尾部延迟的 100% 纯 CP 零动态堆分配 Locality 亲和与平滑溢出路由器：

- **常量与枚举**：
  - `ZONE_ROUTER_MAX_ENDPOINTS = 64`、`ZONE_ROUTER_MAX_ZONES = 8`（`zone_id: 0..=7`）、`ZONE_ROUTER_BPS_SCALE = 10000`、`ZONE_ROUTER_DEFAULT_PANIC_BPS = 5000`
  - `ZonePickMode`（`local_hit | remote_spillover | panic_fallback | no_endpoint`）
  - `ZoneRouterOpStatus`（`ok | invalid_arg | duplicate_endpoint | capacity_full | not_found`）
- **数据模型**：
  - `ZoneRouterConfig`（`panic_threshold_bps`, `fail_open_on_total_panic`）、`ZoneEndpoint`、`ZoneSummary`、`ZonePickDecision`（`mode`, `endpoint_id`, `selected_zone`, `local_health_bps`, `local_retain_bps`）、`ZoneRouterStats`
- **核心引擎 `ZoneLocalityRouter`**：
  - `init(cfg)` / `initDefault()` / `addEndpoint(*_, endpoint_id, zone_id, weight, healthy)` / `removeEndpoint` / `setHealth` / `setWeight` / `incActiveConns` / `decActiveConns`
  - `pickEndpoint(*_, caller_zone, selector_hash): ZonePickDecision`：本区健康占比 `>= panic_threshold_bps` 时 100% 本地亲和；低于恐慌阈值时按 `local_health_bps / panic_threshold_bps` 比例保留本地流量、其余平滑按健康权重溢出至远端健康 AZ；全局全不健康时按配置执行 `panic_fallback` 或 `no_endpoint` 拒流。
  - `getEndpoint(endpoint_id)` / `zoneSummary(zone_id)` / `stats()`。

```cp
import {
    ZonePickMode,
    ZoneLocalityRouter,
} from "cap:net";

pub fn demoZoneLocalityRouter() {
    let lb = ZoneLocalityRouter.initDefault();
    * = lb.addEndpoint(1, 0, 100, true);
    * = lb.addEndpoint(2, 0, 100, true);
    * = lb.addEndpoint(3, 1, 100, true);

    const pick = lb.pickEndpoint(0, 42);
    * = (pick.mode == ZonePickMode.local_hit && pick.selected_zone == 0);
}
```

## Envoy 风格零分配被动异常节点检测与指数驱逐健康引擎 (`outlierDetect.cp`)

面向微服务网格与上游集群健康治理的 100% 纯 CP 零动态堆分配被动异常节点检测器（Envoy Passive Outlier Detection），实时摘除连续失败与统计学成功率离群节点并防止级联全集群熔断：

- **常量与枚举**：
  - `OUTLIER_MAX_HOSTS = 64`、`OUTLIER_HOST_ID_MAX_LEN = 48`
  - `OutlierEjectReason`（`none | consecutive_5xx | consecutive_local_origin | success_rate_anomaly`）
  - `OutlierOutcome`（`success | http_5xx | local_origin_error | gateway_error`）
- **配置与模型**：
  - `OutlierConfig`（`consecutive_5xx = 5`, `consecutive_local_origin = 5`, `base_ejection_time_ms = 30000`, `max_ejection_time_ms = 300000`, `max_ejection_percent = 50`, `min_hosts_for_success_rate = 5`, `success_rate_min_volume = 10`, `success_rate_stdev_factor = 1.9`；提供 `defaults()`）
  - `OutlierHostSlot`（`host_id()`, `ejected`, `ejection_count`, `ejected_until_ms`, `last_reason`, `window_requests`, `window_successes`, `consecutive_5xx`, `consecutive_local_errors`, `successRate()`）
  - `OutlierEvalReport`、`OutlierDetectorStats`
- **核心引擎 `OutlierDetector`**：
  - `init(cfg)` / `registerHost(*_, host_id)` / `removeHost(*_, host_id)` / `isHostHealthy(host_id, now_ms)` / `getHost(host_id)` / `stats()`
  - `recordOutcome(*_, host_id, outcome, now_ms): OutlierEjectReason`：实时追踪连续 5xx / 网关错误与本地连接错误，超阈值且未触及 `max_ejection_percent` 上限时按 `min(base_ejection_time_ms * ejection_count, max_ejection_time_ms)` 驱逐。
  - `runIntervalCheck(*_, now_ms): OutlierEvalReport`：到期自动复职并衰减健康周期历史计数；对满足最小请求量的活跃主机计算群体成功率均值与标准差，按从差到优驱逐低于 `mean - stdev_factor * stdev` 的离群节点，并清零统计窗口。

```cp
import {
    OutlierConfig,
    OutlierOutcome,
    OutlierEjectReason,
    OutlierDetector,
} from "cap:net";

pub fn demoOutlierDetector() {
    let cfg = OutlierConfig.defaults();
    cfg.consecutive_5xx = 3;
    let det = OutlierDetector.init(cfg);
    * = det.registerHost("10.0.0.1:8080");
    * = det.registerHost("10.0.0.2:8080");

    * = det.recordOutcome("10.0.0.1:8080", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("10.0.0.1:8080", OutlierOutcome.http_5xx, 1100);
    const reason = det.recordOutcome("10.0.0.1:8080", OutlierOutcome.http_5xx, 1200);
    * = (reason == OutlierEjectReason.consecutive_5xx);
}
```

## 收敛子模块深度文档

- [amqp.md](net/amqp.md)
- [arp.md](net/arp.md)
- [bacnet.md](net/bacnet.md)
- [bgp.md](net/bgp.md)
- [cidr.md](net/cidr.md)
- [coap.md](net/coap.md)
- [dhcp.md](net/dhcp.md)
- [diameter.md](net/diameter.md)
- [ftp.md](net/ftp.md)
- [gemini.md](net/gemini.md)
- [geneve.md](net/geneve.md)
- [gopher.md](net/gopher.md)
- [gre.md](net/gre.md)
- [haproxy.md](net/haproxy.md)
- [icmp.md](net/icmp.md)
- [igmp.md](net/igmp.md)
- [imap.md](net/imap.md)
- [ipfix.md](net/ipfix.md)
- [ipsec.md](net/ipsec.md)
- [irc.md](net/irc.md)
- [iscsi.md](net/iscsi.md)
- [isis.md](net/isis.md)
- [l2tp.md](net/l2tp.md)
- [ldap.md](net/ldap.md)
- [ldif.md](net/ldif.md)
- [mpls.md](net/mpls.md)
- [nats.md](net/nats.md)
- [nbns.md](net/nbns.md)
- [netflow.md](net/netflow.md)
- [netrc.md](net/netrc.md)
- [nfs.md](net/nfs.md)
- [nntp.md](net/nntp.md)
- [ntp.md](net/ntp.md)
- [ospf.md](net/ospf.md)
- [pop3.md](net/pop3.md)
- [pppoe.md](net/pppoe.md)
- [quic.md](net/quic.md)
- [radius.md](net/radius.md)
- [rip.md](net/rip.md)
- [rtcp.md](net/rtcp.md)
- [rtp.md](net/rtp.md)
- [rtsp.md](net/rtsp.md)
- [sctp.md](net/sctp.md)
- [sdp.md](net/sdp.md)
- [sip.md](net/sip.md)
- [smb.md](net/smb.md)
- [smtp.md](net/smtp.md)
- [snmp.md](net/snmp.md)
- [socks.md](net/socks.md)
- [socks5.md](net/socks5.md)
- [stomp.md](net/stomp.md)
- [stun.md](net/stun.md)
- [tftp.md](net/tftp.md)
- [turn.md](net/turn.md)
- [vrrp.md](net/vrrp.md)
- [whois.md](net/whois.md)
- [wireguard.md](net/wireguard.md)
