# CP 标准库：DNS 协议套件、记录编解码、DoH 与解析配置手册 (`cap:dns`)

> 参考来源：`/data/dev/bun/docs/runtime/networking/dns.mdx`, `/data/dev/bun/src/dns/`, `/data/dev/bun/src/dns.zig` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:dns` 是 CP 语言官方内置的工业生产级域名解析（DNS）协议套件，涵盖 RFC 1035/2782/3596/8484/8659 报文与资源记录编解码、DNS-over-HTTPS（DoH）、POSIX `/etc/resolv.conf` 解析器、`/etc/hosts` 静态表解析、泛型 TTL 缓存与 `c-ares` 异步解析绑定。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                         CP 用户层应用 (Application)                         │
├──────────────────────┬──────────────────────────┬──────────────────────────┤
│ RFC 1035/2782/8659   │ RFC 8484 DNS-over-HTTPS  │ POSIX 解析配置与本地表    │
│ DnsHeader / Wire     │ DohQueryBuilder (GET/POST│ ResolvConf (/etc/resolv) │
│ DnsNameCodec (PTR/防环│ DohJsonResponse / Answer │ HostsTable (/etc/hosts)  │
│ SoaRecord/Srv/Caa    │ application/dns-message  │ DnsCache<const CAP>      │
├──────────────────────┴──────────────────────────┴──────────────────────────┤
│         c-ares 原生异步解析通道绑定 (_cares.cp / CaresStatus)              │
└────────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/dns/packet.cp` | `// Source: /data/dev/bun/src/dns/` | RFC 1035 12 字节 `DnsHeader`、响应码 `DnsRCode`、`DnsWireCodec`（域名标签编码、`0xC0` 指针压缩解码、查询包构造、基础 RR 解析） |
| `lib/dns/cache.cp` | `// Source: /data/dev/bun/src/dns/` | `DnsCache<const CAP: usize>` 零堆分配 TTL 缓存、`HostsTable` `/etc/hosts` 静态映射解析器、`ResolveOptions`、`DnsFamily` |
| `lib/dns/records.cp` | `// Source: cp/lib/dns/records.cp (...)` | `DnsNameCodec`（16 跳防环解压 + IPv4/IPv6 PTR `.arpa` 构造）、`SoaRecord`、`SrvRecord`、`SrvSelector`、`CaaRecord` |
| `lib/dns/doh.cp` | `// Source: cp/lib/dns/doh.cp (...)` | RFC 8484 `DohQueryBuilder`（Base64URL 无填充 GET URL / POST 头）与 `DohJsonResponse` / `DohJsonAnswer` 解析器 |
| `lib/dns/resolv_conf.cp` | `// Source: cp/lib/dns/resolv_conf.cp (...)` | POSIX `/etc/resolv.conf` 解析器、`loadSystem` / `loadFromFile`、`expandSearchDomains`（`ndots` 展开） |
| `lib/dns/caa.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (...)` | RFC 8659 CAA 记录文本/二进制解析与 CA 签发策略裁定（`CaaTag`、`CaaPolicyRecord`、`isCaAuthorized`） |
| `lib/dns/name.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (...)` | RFC 1035/1123 域名标签校验、线格式编解码、子域判定与规范化（`isValidDnsLabel`、`isValidHostname`、`encodeWireName`、`decodeWireName`、`isSubdomainOf`、`normalizeHostname`） |
| `lib/dns/rtype.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (...)` | IANA RRType/Class/RCODE 枚举与 SRV/MX 记录辅助（`DnsRrType`、`DnsRcode`、`DnsClass`、`RrSrvRecord`、`MxRecord`） |
| `lib/dns/srv.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (...)` | RFC 2782 SRV 文本/二进制头解析、`_service._proto.name` 拆解与加权轮询选择（`SrvWireHeader`、`SrvEntry`、`SrvServiceName`、`compareSrvOrder`、`selectSrvByRoll`） |
| `lib/dns/txt.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (...)` | DNS TXT 分号键值对迭代、DKIM/BIMI 标签查找与 SPF/DMARC 记录解析（`TxtTag`、`TxtTagIter`、`findTxtTag`、`DmarcPolicy`、`DmarcRecord`、`SpfQualifier`、`SpfTerm`、`SpfIter`） |
| `lib/dns/wire.cp` | `// Source: cp/lib/dns_wire/types.cp (...)` | 纯 CP RFC 1035/6891 全量 DNS 报文编解码与指针压缩（`DnsWireError`、`DnsOpcode`、`WireDnsRcode`、`DnsRecordType`、`WireDnsClass`、`WireDnsHeader`、`WireDnsQuestion`、`WireResourceRecord`、`DnsRdata`、`DnsPacket`、`encodeDnsPacket`、`decodeDnsPacket`） |
| `lib/dns/_cares.cp` | `// Source: /data/dev/bun/src/dns/` | `NSClass`、`NSType`、`CaresStatus` 错误分类与 `c-ares` C-ABI 声明 |
| `lib/dns/index.cp` | `// Source: /data/dev/bun/src/dns/` | `cap:dns` 标准库对外统一导出入口 |

---

## 二、 RFC 1035 帧头、`0xC0` 防环解压与反向 PTR 构造

### 1. RFC 1035 Section 4.1.1 12 字节固定报文头 (`DnsHeader`)

- `DnsHeader.encode` / `DnsHeader.decode`：编解码 `id`、`is_response`、`opcode`、`authoritative`、`truncated`、`recursion_desired`、`recursion_available`、`rcode`、`qd_count`、`an_count`、`ns_count`、`ar_count`。

### 2. 安全域名解码与防环保护 (`DnsNameCodec`)

- `DnsNameCodec.decode(packet, start_offset, out_name)`：
  - 支持 RFC 1035 Section 4.1.4 `0xC0` 压缩指针跳转；
  - 内置 `MAX_JUMPS = 16` 跳转上限与自环检测，遇到死循环指针立即返回 `DnsRecordError.PointerLoop`；
  - 严格校验标签长度（`<= 63`）、总域名长度（`<= 253`）、保留位（拒绝 `0x40`/`0x80`）与报文边界。
- 反向解析 PTR 域名构造：
  - `DnsNameCodec.ipv4ToArpa(addr, out)` / `ipv4StrToArpa(ip_str, out)`：生成 `d.c.b.a.in-addr.arpa`；
  - `DnsNameCodec.ipv6ToArpa(addr, out)` / `ipv6StrToArpa(ip_str, out)`：按 RFC 3596 将 128 位地址展开为 32 个十六进制半字节逆序格式 `<32 nibbles>.ip6.arpa`（支持 `::` 零压缩解析）。

---

## 三、 SOA / SRV / CAA 生产级资源记录与 RFC 2782 排序

- **`SoaRecord` (RFC 1035)**：解析与编码 `mname`、`rname`（均支持压缩指针）、`serial`、`refresh`、`retry`、`expire`、`minimum`。
- **`SrvRecord` & `SrvSelector` (RFC 2782)**：
  - `SrvRecord.parseRdata` / `parseWire` / `encodeRdata`：编解码 `priority`、`weight`、`port`、`target`；
  - `SrvSelector.sortByPriorityAndWeight(records)`：按 `priority` 升序排列，同优先级内按 `weight` 降序排列（`weight == 0` 置于同优先级末尾）；
  - `SrvSelector.shuffleWeighted(records, seed)`：在同优先级组内按 RFC 2782 权重概率加权洗牌。
- **`CaaRecord` (RFC 8659)**：解析与编码 `flags`（含 `isCritical()` 检查 bit 0 / `0x80`）、`tag`（校验 1..=15 字节 ASCII 字母数字）、`value`。

---

## 四、 RFC 8484 DNS-over-HTTPS (`doh.cp`)

- **`DohQueryBuilder`**：
  - `buildGetUrl(alloc, endpoint, wire_query)`：生成 `?dns=<base64url_no_pad>`（若 `endpoint` 已含 `?` 则自动使用 `&dns=`）；
  - `buildDnsQueryGetUrl(alloc, endpoint, id, domain, qtype)`：一步构建 DNS 查询报文并生成 DoH GET URL；
  - `buildPostHeaders()`：返回标准 `Content-Type: application/dns-message` 与 `Accept: application/dns-message` 头。
- **`DohJsonResponse` & `DohJsonAnswer`**：
  - `DohJsonResponse.parse(alloc, json_bytes)`：解析 Cloudflare / Google `application/dns-json` 响应中的 `Status`、`TC`、`RD`、`RA`、`AD`、`CD`、`Question[]` 与 `Answer[]`（`name`/`rtype`/`TTL`/`data`），使用完毕后调用 `.deinit(alloc)` 释放内存。

---

## 五、 POSIX `/etc/resolv.conf` 解析器 (`resolv_conf.cp`) 与缓存/Hosts

- **`ResolvConf.parse(text)`**：零堆分配解析：
  - 自动剥离 `#` 与 `;` 行内注释；
  - 提取 `nameserver`（IPv4/IPv6，最多 16 个）、`search` / `domain`（最多 16 个搜索域，后出现的覆盖前者）；
  - 提取 `options ndots:N timeout:N attempts:N rotate edns0 trust-ad use-vc`。
- **`ResolvConf.loadSystem(alloc)` / `loadFromFile(alloc, path)`**：从宿主文件加载配置（需调用 `.deinit(alloc)`）。
- **`expandSearchDomains(alloc, host)`**：严格按 `ndots` 阈值展开搜索域（末尾带 `.` 的 FQDN 原样返回；`dots >= ndots` 时先查裸域名再查搜索域；`dots < ndots` 时先查搜索域再兜底裸域名）。
- **`HostsTable` & `DnsCache<CAP>`**：零堆分配 `/etc/hosts` 静态表解析与泛型 TTL 缓存（保持 100% 向后兼容）。

---

## 六、 完整代码示例

```typescript
const std = @import("std");
import {
    NSType,
    DnsWireCodec,
    DnsNameCodec,
    SrvRecord,
    SrvSelector,
    DohQueryBuilder,
    DohJsonResponse,
    ResolvConf,
} from "cap:dns";

pub fn demoDnsProduction(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 反向 PTR 域名构造
    let ptr_buf: [80]u8 = undefined;
    const v4_ptr = try DnsNameCodec.ipv4StrToArpa("192.0.2.1", &ptr_buf);
    // => "1.2.0.192.in-addr.arpa"

    // 2. SRV 记录优先级与权重排序
    let srvs: [3]SrvRecord = [
        SrvRecord.init(20, 10, 8443, "backup.cp-lang.org"),
        SrvRecord.init(10, 5, 443, "edge-b.cp-lang.org"),
        SrvRecord.init(10, 80, 443, "edge-a.cp-lang.org"),
    ];
    SrvSelector.sortByPriorityAndWeight(&srvs);
    // srvs[0] -> edge-a (prio 10, weight 80)

    // 3. DoH GET URL 构造与 JSON 响应解析
    const doh_url = try DohQueryBuilder.buildDnsQueryGetUrl(
        alloc,
        "https://cloudflare-dns.com/dns-query",
        0x1234,
        "cp-lang.org",
        NSType.A,
    );
    defer alloc.free(doh_url);

    // 4. /etc/resolv.conf 解析与 ndots 搜索域展开
    let rc = ResolvConf.parse("nameserver 1.1.1.1\nsearch corp.internal\noptions ndots:2\n");
    const candidates = try rc.expandSearchDomains(alloc, "api.svc");
    defer ResolvConf.freeExpandedDomains(alloc, candidates);
    // candidates => ["api.svc.corp.internal", "api.svc"]
}
```

---

## 上游 Bun DNS 行为契约与安全边界注意事项

1. **有界 TTL 缓存与失败驱逐（参考 `/data/dev/bun/docs/runtime/networking/dns.mdx`）**：
   - 上游 Bun DNS 缓存采用固定条目上限（默认 255 条）与有界 TTL（默认 30s），连接失败时主动驱逐失效缓存；在 CP 中可使用 `DnsCache<256>` 零堆分配实现同等有界缓存，并在高并发建连前合并重复域名查询。
2. **DNS 压缩指针防环与标签长度边界（RFC 1035）**：
   - `DnsWireCodec` 与 `DnsNameCodec` 解码 `0xC0` 压缩指针时强制限制最大跳转深度（16 跳）并校验单标签长度 `<= 63`、总域名长度 `<= 253`（`isValidDnsLabel` / `isValidHostname`），彻底拦截畸形压缩指针死循环。
3. **DoH（RFC 8484）无填充 Base64URL 约定**：
   - `DohQueryBuilder.buildDnsQueryGetUrl` 生成 GET 请求时严格使用**无 `=` 填充**的 Base64URL 编码 `dns=` 参数，POST 请求使用 `Content-Type: application/dns-message`。
4. **`/etc/resolv.conf` `ndots` 搜索域展开顺序**：
   - 当查询主机名中的点号 `.` 数量少于 `ndots` 时，优先拼接 `search` 列表逐一尝试，最后回退原名；超过或等于 `ndots` 时优先按绝对域名直接解析。
