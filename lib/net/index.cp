// Source: /data/dev/bun/src/uws/uws.cp & /data/dev/bun/src/runtime/socket/
// lib/net/index.cp
// Unified Public Exports for cap:net
// Architecture:
//   - _usockets.cp  : Low-level @c uSockets FFI bindings, opaque handles, and constants
//   - address.cp    : IPv4 / IPv6 SocketAddress parsing & manipulation
//   - socket.cp     : SafeSocket with RAII, socket options, and legacy Socket compatibility
//   - tcp_stream.cp : High-throughput TCP Stream with 16KB StreamBuffer corking
//   - tcp_server.cp : TcpServer, ListenConfig, and legacy Server / listen() API
//   - udp.cp        : UdpSocket and UdpPacket datagram abstraction
//   - proxy.cp      : HttpConnectTunnel RFC 7231 proxy tunnel helper
//   - cidr.cp       : IPv4/IPv6 CIDR parsing, matching, IpClass & CidrAllowlist
//   - sockopts.cp   : POSIX/Linux socket options & PollEvent / pollSingleFd
//   - socks5.cp     : Full RFC 1928/1929 SOCKS5 & HttpConnectCodec wire codecs
//   - pool.cp       : Production TCP ConnectionPool with TTL & capacity eviction

export {
    LIBUS_TIMEOUT_GRANULARITY,
    LIBUS_RECV_BUFFER_PADDING,
    LIBUS_EXT_ALIGNMENT,
    LIBUS_LISTEN_DEFAULT,
    LIBUS_LISTEN_EXCLUSIVE_PORT,
    LIBUS_SOCKET_ALLOW_HALF_OPEN,
    LIBUS_LISTEN_REUSE_PORT,
    LIBUS_SOCKET_IPV6_ONLY,
    LIBUS_LISTEN_REUSE_ADDR,
    LIBUS_LISTEN_DISALLOW_REUSE_PORT_FAILURE,
    SocketCloseCode,
    SocketKind,
    SendStatus,
    us_loop_t,
    us_socket_t,
    us_socket_context_t,
    us_listen_socket_t,
    us_connecting_socket_t,
    us_udp_socket_t,
    us_udp_packet_buffer_t,
    us_timer_t,
    UsVerifyError
} from "./_usockets";

export {
    AddressFamily,
    Ipv4Addr,
    Ipv6Addr,
    SocketAddress
} from "./address";

export {
    SocketState,
    SocketStats,
    SafeSocket,
    Socket
} from "./socket";

export {
    STREAM_CORK_BUFFER_SIZE,
    StreamBuffer,
    TcpStream
} from "./tcp_stream";

export {
    ListenConfig,
    TcpServer,
    Server,
    listen,
    connect
} from "./tcp_server";

export {
    UdpPacket,
    UdpSocket
} from "./udp";

export {
    HttpConnectTunnel
} from "./proxy";

export {
    IpKind,
    IpAddr,
    IpAddrCodec,
    Ipv4Cidr,
    Ipv6Cidr,
    IpClass,
    CidrAllowlist
} from "./cidr";

export {
    PollEvent,
    SocketOptions,
    setNoDelay,
    getNoDelay,
    setKeepAlive,
    getKeepAlive,
    setReuseAddr,
    getReuseAddr,
    setReusePort,
    getReusePort,
    setRecvBufSize,
    getRecvBufSize,
    setSendBufSize,
    getSendBufSize,
    setRecvTimeoutMs,
    setSendTimeoutMs,
    setNonBlocking,
    isNonBlocking,
    getSocketError,
    pollSingleFd,
    pollManyFds,
    createTcpSocket,
    createSocketPair,
    closeSocketFd,
    writeFd,
    readFd
} from "./sockopts";

export {
    Socks5AddrType,
    Socks5AuthMethod,
    Socks5ReplyCode,
    Socks5MethodSelection,
    Socks5ConnectReply,
    Socks5Codec,
    HttpConnectCodec
} from "./socks5";

export {
    PoolCloseFn,
    ConnPoolConfig,
    PoolEntry,
    PoolStats,
    ConnectionPool
} from "./pool";

export {
    Ipv4Addr as RfcIpv4Addr,
    Ipv6Addr as RfcIpv6Addr,
    IpVersion,
    IpAddr as RfcIpAddr,
} from "./ipaddr";

export {
    MacAddr,
} from "./mac";

export {
    HostKind,
    SocketAddr,
} from "./sockaddr";

export {
    ProxyProtoFamily,
    ProxyV1Header,
} from "./proxyv1";

export {
    PROXY_V2_SIGNATURE,
    hasProxyV2Signature,
    ProxyV2Command,
    ProxyV2Family,
    ProxyV2Transport,
    ProxyV2Tlv,
    ProxyV2TlvIter,
    ProxyV2Header,
} from "./proxyv2";

export {
    HostsLine,
    HostsIter,
    lookupHost,
    lookupHostV4,
    lookupHostV6,
    reverseLookup,
} from "./hosts";

export {
    Ipv4Addr as NoProxyIpv4Addr,
    Ipv4Cidr as NoProxyIpv4Cidr,
    shouldBypassProxy,
} from "./noproxy";
