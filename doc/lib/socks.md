# `cap:socks` — RFC 1928 / RFC 1929 / SOCKS4/4a 代理协议零分配编解码库

纯 CP、零堆分配的 SOCKS5（RFC 1928）、用户名/口令子协商（RFC 1929）及 SOCKS4/4a 线协议编解码库，覆盖方法协商、认证、IPv4/Domain/IPv6 请求与回复、UDP Associate 中继报文以及 SOCKS4a 域名解析。

## 导入

```cp
import {
    SOCKS_TCP_PORT,
    SOCKS_V4,
    SOCKS_V5,
    SOCKS5_USERPASS_VER,
    AUTH_NONE,
    AUTH_GSSAPI,
    AUTH_USERPASS,
    AUTH_NO_ACCEPTABLE,
    CMD_CONNECT,
    CMD_BIND,
    CMD_UDP_ASSOCIATE,
    ATYP_IPV4,
    ATYP_DOMAIN,
    ATYP_IPV6,
    REP_SUCCEEDED,
    REP_GENERAL_FAILURE,
    REP_NOT_ALLOWED,
    REP_NET_UNREACHABLE,
    REP_HOST_UNREACHABLE,
    REP_CONN_REFUSED,
    REP_TTL_EXPIRED,
    REP_CMD_NOT_SUPPORTED,
    REP_ATYP_NOT_SUPPORTED,
    Socks5Greeting,
    parseSocks5Greeting,
    encodeSocks5Greeting,
    encodeSocks5MethodSelect,
    Socks5UserPass,
    parseSocks5UserPass,
    encodeSocks5UserPass,
    Socks5Addr,
    Socks5Request,
    parseSocks5Request,
    encodeSocks5Request,
    Socks5Reply,
    parseSocks5Reply,
    encodeSocks5Reply,
    Socks5UdpPacket,
    parseSocks5Udp,
    encodeSocks5Udp,
    Socks4Request,
    parseSocks4Request,
} from "cap:socks";
```

## 核心 API

- **SOCKS5 握手与方法选择**：`parseSocks5Greeting(data): Socks5Greeting?`（附 `supports(method)`）、`encodeSocks5Greeting(methods, out): usize?`、`encodeSocks5MethodSelect(method, out): usize?`。
- **RFC 1929 用户名/口令认证**：`parseSocks5UserPass(data): Socks5UserPass?`、`encodeSocks5UserPass(user, pass, out): usize?`。
- **SOCKS5 请求与回复**：`Socks5Addr { atyp, raw, port }`（支持 `ATYP_IPV4` 4B、`ATYP_DOMAIN` 1..255B、`ATYP_IPV6` 16B）；`parseSocks5Request` / `encodeSocks5Request`、`parseSocks5Reply` / `encodeSocks5Reply`。
- **SOCKS5 UDP 中继**：`parseSocks5Udp(data): Socks5UdpPacket?`、`encodeSocks5Udp(frag, dst_addr, payload, out): usize?`（零拷贝切片 `payload`）。
- **SOCKS4 / SOCKS4a 请求**：`parseSocks4Request(data): Socks4Request?`（附 `isSocks4a()`，当 `ip == 0.0.0.x && x != 0` 自动解析 NUL 终止域名）。

## 示例

```cp
import {
    ATYP_DOMAIN,
    CMD_CONNECT,
    Socks5Addr,
    encodeSocks5Request,
    parseSocks5Request,
} from "cap:socks";

fn example() {
    let buf: [64]u8 = [0; 64];
    const addr = Socks5Addr{ .atyp = ATYP_DOMAIN, .raw = "example.com", .port = 443 };
    if (n := encodeSocks5Request(CMD_CONNECT, addr, buf[0..])) {
        if (req := parseSocks5Request(buf[0..n])) {
            * = req.addr.port; // 443
        }
    }
}
```
