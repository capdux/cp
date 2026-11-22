// Source: Absorbed from /data/dev/bun/src/uws/uws.cp | Synced: 2026-11-21T16:46:57Z
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

// === Consolidated L2/L3/L4, Routing, Application & Messaging Protocols ===

// --- arp ---
export {
    ARP_ETHERTYPE,
    ARP_MIN_HEADER_SIZE,
    ARP_IPV4_PACKET_SIZE,
    ARP_HTYPE_ETHERNET,
    ARP_PTYPE_IPV4,
    ARP_HLEN_ETHERNET,
    ARP_PLEN_IPV4,
    ArpOp,
    arpOpFromU16,
    ArpPacket,
    ArpRawHeader,
    isArpIpv4,
    parseArpIpv4,
    parseArpRaw,
    encodeArpIpv4,
    encodeArpRequest,
    encodeArpReply,
    encodeArpProbe,
    encodeGratuitousArp,
} from "./arp.cp";

// --- bgp ---
export {
    BGP_HEADER_SIZE,
    BGP_MARKER_LEN,
    BGP_MAX_MESSAGE_LEN,
    BGP_PORT,
    ATTR_ORIGIN,
    ATTR_AS_PATH,
    ATTR_NEXT_HOP,
    ATTR_MULTI_EXIT_DISC,
    ATTR_LOCAL_PREF,
    ATTR_ATOMIC_AGGREGATE,
    ATTR_AGGREGATOR,
    ATTR_COMMUNITIES,
    BgpMsgType,
    BgpHeader,
    BgpMessage,
    BgpOptParam,
    BgpOptParamIter,
    BgpOpen,
    BgpUpdate,
    BgpPrefix,
    BgpPrefixIter,
    BgpPathAttr,
    BgpPathAttrIter,
    BgpNotification,
    BgpStreamIter,
    parseBgpMessage,
    parseBgpOpen,
    parseBgpUpdate,
    parseBgpNotification,
} from "./bgp.cp";

// --- dhcp ---
export {
    DHCP_SERVER_PORT,
    DHCP_CLIENT_PORT,
    DHCP_FIXED_HEADER_SIZE,
    DHCP_MIN_PACKET_SIZE,
    DHCP_MAGIC_COOKIE,
    DHCP_OP_REQUEST,
    DHCP_OP_REPLY,
    DHCP_FLAG_BROADCAST,
    OPT_PAD,
    OPT_SUBNET_MASK,
    OPT_ROUTER,
    OPT_DNS,
    OPT_HOSTNAME,
    OPT_DOMAIN_NAME,
    OPT_REQUESTED_IP,
    OPT_LEASE_TIME,
    OPT_MSG_TYPE,
    OPT_SERVER_ID,
    OPT_PARAM_REQ_LIST,
    OPT_MAX_MSG_SIZE,
    OPT_RENEWAL_TIME,
    OPT_REBINDING_TIME,
    OPT_VENDOR_CLASS,
    OPT_CLIENT_ID,
    OPT_END,
    DhcpMsgType,
    DhcpHeader,
    DhcpOption,
    DhcpOptIter,
    DhcpPacket,
    DhcpWriter,
    parseDhcp,
    isDhcp,
} from "./dhcp.cp";

// --- icmp ---
export {
    ICMP_HEADER_SIZE,
    ICMP_IP_PROTO_V4,
    ICMP_IP_PROTO_V6,
    ICMP4_ECHO_REPLY,
    ICMP4_DEST_UNREACH,
    ICMP4_REDIRECT,
    ICMP4_ECHO_REQUEST,
    ICMP4_TIME_EXCEEDED,
    ICMP4_PARAM_PROBLEM,
    ICMP4_TIMESTAMP_REQ,
    ICMP4_TIMESTAMP_REPLY,
    ICMP6_DEST_UNREACH,
    ICMP6_PACKET_TOO_BIG,
    ICMP6_TIME_EXCEEDED,
    ICMP6_PARAM_PROBLEM,
    ICMP6_ECHO_REQUEST,
    ICMP6_ECHO_REPLY,
    ICMP6_ROUTER_SOLICIT,
    ICMP6_ROUTER_ADVERT,
    ICMP6_NEIGHBOR_SOLICIT,
    ICMP6_NEIGHBOR_ADVERT,
    IcmpPacket,
    icmp4Checksum,
    verifyIcmp4Checksum,
    icmp6Checksum,
    verifyIcmp6Checksum,
    parseIcmp,
    encodeIcmp4,
    encodeIcmp4Echo,
    encodeIcmp6,
    encodeIcmp6Echo,
} from "./icmp.cp";

// --- ipfix ---
export {
    IPFIX_PORT,
    IPFIX_TLS_PORT,
    IPFIX_VERSION,
    IPFIX_HEADER_SIZE,
    IPFIX_SET_HEADER_SIZE,
    SET_ID_TEMPLATE,
    SET_ID_OPTIONS_TEMPLATE,
    SET_ID_DATA_MIN,
    IE_ENTERPRISE_BIT,
    IE_VARLEN_INDICATOR,
    IpfixHeader,
    IpfixFieldSpec,
    IpfixFieldSpecIter,
    IpfixTemplateRecord,
    IpfixTemplateIter,
    IpfixSet,
    IpfixSetIter,
    IpfixMessage,
    isIpfix,
    parseIpfix,
    IpfixVarLenValue,
    decodeIpfixVarLen,
} from "./ipfix.cp";

// --- ipsec ---
export {
    IPSEC_PROTO_ESP,
    IPSEC_PROTO_AH,
    IKE_UDP_PORT,
    IKE_NATT_UDP_PORT,
    ESP_MIN_HEADER_SIZE,
    AH_MIN_HEADER_SIZE,
    IKEV2_HEADER_SIZE,
    IKE_EXCH_SA_INIT,
    IKE_EXCH_AUTH,
    IKE_EXCH_CREATE_CHILD_SA,
    IKE_EXCH_INFORMATIONAL,
    IKE_FLAG_INITIATOR,
    IKE_FLAG_VERSION,
    IKE_FLAG_RESPONSE,
    PL_NONE,
    PL_SA,
    PL_KE,
    PL_IDI,
    PL_IDR,
    PL_CERT,
    PL_CERTREQ,
    PL_AUTH,
    PL_NONCE,
    PL_NOTIFY,
    PL_DELETE,
    PL_VENDOR_ID,
    PL_TSI,
    PL_TSR,
    PL_SK,
    EspPacket,
    EspTrailer,
    AhPacket,
    Ikev2Header,
    Ikev2Payload,
    Ikev2PayloadIter,
    parseEsp,
    parseEspTrailer,
    parseAh,
    parseIkev2,
    isIkev2,
} from "./ipsec.cp";

// --- netflow ---
export {
    NETFLOW_UDP_PORT,
    NETFLOW_V5,
    NETFLOW_V9,
    NF5_HEADER_SIZE,
    NF5_RECORD_SIZE,
    NF9_HEADER_SIZE,
    NF9_FLOWSET_HEADER_SIZE,
    NF9_FLOWSET_TEMPLATE,
    NF9_FLOWSET_OPTIONS_TEMPLATE,
    NF9_FLOWSET_DATA_MIN,
    Nf5Header,
    Nf5Record,
    Nf5RecordIter,
    Nf5Packet,
    Nf9Header,
    Nf9FieldSpec,
    Nf9FieldSpecIter,
    Nf9TemplateRecord,
    Nf9TemplateIter,
    Nf9FlowSet,
    Nf9FlowSetIter,
    Nf9Packet,
    peekNetflowVersion,
    parseNetflowV5,
    parseNetflowV9,
    isNetflow,
} from "./netflow.cp";

// --- haproxy ---
export {
    HaSectionKind,
    HaSection,
    HaDirective,
    HaSectionIter,
    HaDirectiveIter,
    findHaSection,
    findHaDirective,
} from "./haproxy.cp";

// --- amqp ---
export {
    PROTOCOL_HEADER,
    FRAME_END,
    AMQP_MAX_DEPTH,
    DEFAULT_MAX_FRAME_SIZE,
    PROP_FLAG_CONTENT_TYPE,
    PROP_FLAG_HEADERS,
    AmqpError,
    AmqpFrameType,
    frameTypeFromU8,
    frameTypeToU8,
    AmqpField,
    AmqpValue,
    AmqpFrame,
    AmqpMethodPayload,
    AmqpContentHeader,
    AmqpStreamDecoder,
    deinitFieldTable,
    getField,
    eqlFieldTable,
    encodeFieldTable,
    decodeFieldTable,
    decodeFieldTableWithMaxDepth,
    encodeFrame,
    encodeMethodFrame,
    decodeMethodPayload,
    encodeContentHeader,
    decodeContentHeader,
    encodeHeartbeat,
} from "./amqp.cp";

// --- coap ---
export {
    COAP_UDP_PORT,
    COAPS_UDP_PORT,
    COAP_VERSION,
    COAP_HEADER_SIZE,
    COAP_PAYLOAD_MARKER,
    COAP_CON,
    COAP_NON,
    COAP_ACK,
    COAP_RST,
    CODE_EMPTY,
    CODE_GET,
    CODE_POST,
    CODE_PUT,
    CODE_DELETE,
    CODE_CREATED,
    CODE_DELETED,
    CODE_VALID,
    CODE_CHANGED,
    CODE_CONTENT,
    CODE_BAD_REQUEST,
    CODE_NOT_FOUND,
    CODE_INTERNAL_ERROR,
    OPT_IF_MATCH,
    OPT_URI_HOST,
    OPT_ETAG,
    OPT_OBSERVE,
    OPT_URI_PORT,
    OPT_URI_PATH,
    OPT_CONTENT_FORMAT,
    OPT_MAX_AGE,
    OPT_URI_QUERY,
    OPT_ACCEPT,
    OPT_BLOCK2,
    OPT_BLOCK1,
    OPT_SIZE1,
    coapCode,
    CoapOption,
    CoapOptIter,
    CoapMessage,
    CoapWriter,
    parseCoap,
    isCoap,
} from "./coap.cp";

// --- bacnet ---
export {
    BACNET_UDP_PORT,
    BVLC_TYPE_IPV4,
    BVLC_HEADER_SIZE,
    NPDU_VERSION,
    BVLC_RESULT,
    BVLC_FORWARDED_NPDU,
    BVLC_REGISTER_FOREIGN_DEVICE,
    BVLC_ORIGINAL_UNICAST,
    BVLC_ORIGINAL_BROADCAST,
    APDU_CONFIRMED_REQ,
    APDU_UNCONFIRMED_REQ,
    APDU_SIMPLE_ACK,
    APDU_COMPLEX_ACK,
    APDU_SEGMENT_ACK,
    APDU_ERROR,
    APDU_REJECT,
    APDU_ABORT,
    SVC_I_AM,
    SVC_WHO_IS,
    SVC_READ_PROPERTY,
    SVC_READ_PROPERTY_MULTIPLE,
    SVC_WRITE_PROPERTY,
    isBvlc,
    BvlcPacket,
    parseBvlc,
    encodeBvlcUnicast,
    encodeBvlcBroadcast,
    NpduPacket,
    parseNpdu,
    ApduHeader,
    parseApdu,
    BacnetObjectId,
    decodeObjectId,
    encodeObjectId,
    BacnetTag,
    BacnetTagIter,
} from "./bacnet.cp";

// --- ftp ---
export {
    FTP_CONTROL_PORT,
    FTP_DATA_PORT,
    FTPS_IMPLICIT_PORT,
    RPL_DATA_OPEN,
    RPL_FILE_OK,
    RPL_OK,
    RPL_READY,
    RPL_CLOSING,
    RPL_TRANSFER_OK,
    RPL_PASV,
    RPL_EPSV,
    RPL_LOGGED_IN,
    RPL_PATHNAME,
    RPL_NEED_PASSWORD,
    RPL_NOT_LOGGED_IN,
    RPL_FILE_UNAVAILABLE,
    FtpCommand,
    parseFtpCommand,
    FtpReplyLine,
    parseFtpReplyLine,
    FtpPasvAddr,
    parsePasvReply,
    parseEpsvPort,
    FtpMlsxFact,
    FtpMlsxFactIter,
    FtpMlsxEntry,
    parseMlsxLine,
    encodeFtpCommand,
    encodePortCommand,
    encodeEprtCommand,
} from "./ftp.cp";

// --- gopher ---
export {
    GOPHER_TCP_PORT,
    TYPE_TEXT,
    TYPE_DIR,
    TYPE_CSO,
    TYPE_ERROR,
    TYPE_BINHEX,
    TYPE_DOS,
    TYPE_UUENCODED,
    TYPE_SEARCH,
    TYPE_TELNET,
    TYPE_BINARY,
    TYPE_MIRROR,
    TYPE_GIF,
    TYPE_IMAGE,
    TYPE_INFO,
    TYPE_HTML,
    TYPE_SOUND,
    GopherRequest,
    parseGopherRequest,
    encodeGopherRequest,
    GopherItem,
    parseGopherItem,
    encodeGopherItem,
    GopherMenuIter,
} from "./gopher.cp";

// --- imap ---
export {
    ImapError,
    ImapStatus,
    ImapResponseKind,
    ImapStreamParser,
    ImapCommandCodec,
    parseCapability,
    parseSearchUids,
    parseExistsCount,
    parseFetchFlags,
} from "./imap.cp";

// --- irc ---
export {
    IRC_TCP_PORT,
    IRCS_TCP_PORT,
    IRC_MAX_LINE_LEN,
    IRCV3_MAX_TAGS_LEN,
    RPL_WELCOME,
    RPL_YOURHOST,
    RPL_CREATED,
    RPL_MYINFO,
    RPL_ISUPPORT,
    RPL_TOPIC,
    RPL_NAMREPLY,
    RPL_ENDOFNAMES,
    RPL_MOTD,
    RPL_ENDOFMOTD,
    ERR_NOSUCHNICK,
    ERR_NOSUCHCHANNEL,
    ERR_NICKNAMEINUSE,
    IrcTag,
    IrcTagIter,
    unescapeTagValue,
    escapeTagValue,
    IrcPrefix,
    parseIrcPrefix,
    IrcParamIter,
    IrcMessage,
    parseIrcLine,
    IrcWriter,
} from "./irc.cp";

// --- ldap ---
export {
    DEFAULT_MAX_DEPTH,
    LdapError,
    LdapAttr,
    LdapAttrVal,
    LdapSubstring,
    DnComponent,
    LdapFilter,
    parseFilter,
    parseFilterWithMaxDepth,
    formatFilter,
    escapeFilterValue,
    matchesEntry,
    parseDn,
    freeDn,
} from "./ldap.cp";

// --- nats ---
export {
    DEFAULT_MAX_PAYLOAD_BYTES,
    NATS_HEADER_PREFIX,
    NatsError,
    NatsHeader,
    NatsMsg,
    NatsOp,
    NatsEncoder,
    NatsParser,
} from "./nats.cp";

// --- netrc ---
export { NetrcEntry, NetrcIter, lookupNetrc } from "./netrc.cp";
