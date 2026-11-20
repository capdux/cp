# CP 标准库：DNS 报文编解码、本地 Hosts 解析与 TTL 缓存手册 (`cap:dns`)

## 一、 架构总览与设计准则

`cap:dns` 是 CP 语言官方内置的工业级域名解析（DNS）协议编解码、本地 `/etc/hosts` 静态表解析、泛型 TTL 缓存与 `c-ares` 异步解析绑定标准库，全面吸收自 Bun 原生 DNS 子系统（`/data/dev/bun/src/dns/` 与 `src/cares_sys/`）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├──────────────────────┬─────────────────────────┬───────────────────────┤
│ RFC 1035 报文编解码   │  本地 Hosts 静态解析表   │  编译期泛型 TTL 缓存  │
│ DnsHeader (12 Bytes) │  HostsTable.parse       │  DnsCache<const CAP>  │
│ DnsWireCodec         │  HostsTable.parseIPv4   │  put / lookup         │
│ 0xC0 指针解压缩       │  lookup / lookupIPv4    │  evictExpired         │
├──────────────────────┴─────────────────────────┴───────────────────────┤
│         c-ares 原生异步解析通道绑定 (_cares.cp / CaresStatus)          │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/dns/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/dns/packet.cp` | `// Source: /data/dev/bun/src/dns/` | RFC 1035 12 字节 `DnsHeader`、响应码 `DnsRCode`、`DnsWireCodec`（域名标签编码、带防环保护的 `0xC0` 指针压缩解码、查询包构造） |
| `lib/dns/cache.cp` | `// Source: /data/dev/bun/src/dns/` | `DnsCache<const CAP: usize>` 零堆分配 TTL 缓存、`HostsTable` `/etc/hosts` 静态映射解析器、`ResolveOptions`、`DnsFamily` |
| `lib/dns/_cares.cp` | `// Source: /data/dev/bun/src/dns/` | `NSClass`、`NSType`（A/AAAA/CNAME/MX/TXT/SRV/CAA）、`CaresStatus` 错误分类与 `c-ares` C-ABI 声明 |
| `lib/dns/index.cp` | `// Source: /data/dev/bun/src/dns/` | `cap:dns` 标准库对外统一导出入口 |

---

## 二、 RFC 1035 12 字节 DNS 帧头与 `0xC0` 域名压缩指针

### 1. RFC 1035 Section 4.1.1 12 字节固定报文头 (`DnsHeader`)

```text
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 0  |                      ID                       |  事务标识符 (u16 BE)
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 2  |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |  标志位与响应码
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 4  |                    QDCOUNT                    |  问题数 (Questions)
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 6  |                    ANCOUNT                    |  回答资源记录数 (Answers)
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 8  |                    NSCOUNT                    |  权威名称服务器记录数
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
10  |                    ARCOUNT                    |  附加资源记录数
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

### 2. 域名标签序列与 `0xC0` 压缩指针解码 (`DnsWireCodec`)

- **标签序列编码 (`encodeDomainName`)**：
  将点分域名 `"api.cap.dev"` 编码为长度前缀标签序列 `"\x03api\x03cap\x03dev\x00"`（单标签最大 63 字节，总长最大 253 字节）。
- **RFC 1035 Section 4.1.4 指针压缩解码 (`decodeDomainName`)**：
  当遇到高 2 位为 `11`（即 `len_byte & 0xC0 == 0xC0`）的字节时，自动将其与下一字节组合为 14 位报文内偏移量 `((len_byte & 0x3F) << 8) | next_byte` 并跳转读取后缀标签；内置 **16 跳（`hops < 16`）防恶意死循环指针炸弹保护**，同时精确返回首次跳转前在当前记录中消耗的字节数 `packet_bytes_consumed`。

---

## 三、 本地 `/etc/hosts` 解析 (`HostsTable`) 与 DNS TTL 缓存 (`DnsCache<CAP>`)

### 1. 零分配 `/etc/hosts` 解析器 (`HostsTable`)

- 自动剥离 `#` 行内注释、空行与 `\r\n`；
- 支持单行多别名映射（如 `127.0.0.1 localhost loopback api.local`）；
- 自动将点分十进制 IPv4 地址（如 `"10.0.0.42"`）解析为大端序 `u32` 紧凑整数（`0x0A00002A`），同时兼容 IPv6 条目（如 `"::1 ip6-localhost"`）。

### 2. 编译期常量泛型 TTL 缓存 (`DnsCache<const CAP: usize>`)

- 采用编译期确定容量的固定数组 `[CAP]DnsCacheEntry`，**100% 零堆内存分配**；
- `put(host, ipv4_packed, port, now_ms, ttl_ms)`：插入或刷新缓存项，若槽位已满自动回收已过期的槽位；
- `lookup(host, port, now_ms)`：命中且未过期时自增 `hits` 与 `total_hits`；若已过期则自动惰性驱逐（Lazy Eviction）并计入 `total_misses`；
- `evictExpired(now_ms)`：主动批量清理所有已过期条目并返回清理数量。

---

## 四、 完整代码示例

```typescript
import {
    NSType,
    DnsRCode,
    DnsHeader,
    DnsWireCodec,
    DnsCache,
    HostsTable,
} from "cap:dns";

pub fn demoDnsSubsystem() {
    // =========================================================================
    // 1. 解析本地 /etc/hosts 静态映射表
    // =========================================================================
    const hosts_text =
        "# Local cluster hosts\n" +
        "127.0.0.1\tlocalhost loopback\n" +
        "10.24.0.88  redis.internal db.internal # cache node\n" +
        "::1         ip6-localhost\n";

    let hosts = HostsTable.parse(hosts_text);
    // hosts.entryCount() == 5
    const redis_ip_str = hosts.lookup("redis.internal").?; // "10.24.0.88"
    const redis_ipv4 = hosts.lookupIPv4("db.internal").?;  // 0x0A180058

    // =========================================================================
    // 2. 构造 RFC 1035 DNS 查询报文并解压 0xC0 压缩域名
    // =========================================================================
    let pkt: [128]u8 = [0; 128];
    const pkt_len = DnsWireCodec.buildQueryPacket(0x9901, "registry.cp-lang.org", NSType.A, &pkt);

    let decoded_name: [64]u8 = undefined;
    if (dom := DnsWireCodec.decodeDomainName(pkt[0..pkt_len], 12, &decoded_name)) {
        // decoded_name[0..dom.name_len] => "registry.cp-lang.org"
    }

    // =========================================================================
    // 3. 编译期泛型 DnsCache<32> 高速 TTL 缓存
    // =========================================================================
    let cache = DnsCache<32>.init();
    const now_ms: u64 = 100000;
    const ttl_ms: u64 = 30000; // 30s TTL

    * = cache.put("registry.cp-lang.org", redis_ipv4, 443, now_ms, ttl_ms);

    // 10s 后查询 -> 命中 (0x0A180058)
    const hit_ip = cache.lookup("registry.cp-lang.org", 443, now_ms + 10000);

    // 35s 后查询 -> 已过期返回 null 并自动回收槽位
    const expired_ip = cache.lookup("registry.cp-lang.org", 443, now_ms + 35000);
}
```

---

## 五、 与 `Bun.dns` / `node:dns` API 对照表

| 功能特性 | Bun (`Bun.dns` / `src/dns/`) | CP 标准库 (`cap:dns`) |
| :--- | :--- | :--- |
| **DNS 记录类型枚举** | `NSType` (`A`, `AAAA`, `CNAME`, `MX`, `TXT`, `SRV`, `CAA`) | `NSType` (`A`, `AAAA`, `CNAME`, `MX`, `TXT`, `SRV`, `CAA`) |
| **RFC 1035 报文头编解码** | `dns/packet` 12-byte Header | `DnsHeader.encode` / `DnsHeader.decode` |
| **域名标签与 `0xC0` 解压** | `c-ares` / 内置 DNS 解析器 | `DnsWireCodec.encodeDomainName` / `decodeDomainName` / `buildQueryPacket` / `decodeQuestion` |
| **资源记录 (RR)  wire 解码** | `SRV` / `TXT` / `MX` / `AAAA` / `A` / `CNAME` | `decodeResourceRecord` + `parseARdata` / `parseAAAARdata` / `parseNameRdata` / `parseMxRdata` / `parseSrvRdata` / `parseTxtRdata` + `formatIPv4` / `formatIPv6` |
| **`/etc/hosts` 静态解析** | `etc_hosts` 本地覆盖表 | `HostsTable.parse(content)` -> `.lookup(host)` / `.lookupIPv4(host)` / `.lookupIPv6(host)` |
| **TTL 缓存管理** | `Bun.dns.prefetch` / 内置 DNS Cache | `DnsCache<CAP>.init()` -> `.put` / `.putAAAA` / `.lookup` / `.lookupAAAA` / `.evictExpired` |
| **c-ares 状态码诊断** | `ares_strerror` | `CaresStatus.codeName(code)` / `CaresStatus.isRetryable(code)` |
