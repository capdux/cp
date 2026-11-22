# `cap:mdns` — RFC 6762 Multicast DNS & RFC 6763 DNS-SD 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

纯 CP、零堆分配（zero-alloc）的 Multicast DNS（mDNS）与 DNS-Based Service Discovery（DNS-SD）线协议编解码库，支持 QU 单播响应位、Cache-Flush 位、`0xC0` 域名压缩指针安全解码（最大 16 次跳转防环形死循环）、PTR/SRV/TXT/A/AAAA 记录解析与 RFC 6763 TXT 键值迭代。

## 导入

```cp
import {
    MDNS_UDP_PORT,
    MDNS_MULTICAST_V4,
    MDNS_MULTICAST_V6,
    MDNS_HEADER_SIZE,
    MDNS_QU_BIT,
    MDNS_CACHE_FLUSH_BIT,
    CLASS_IN,
    TYPE_A,
    TYPE_PTR,
    TYPE_TXT,
    TYPE_AAAA,
    TYPE_SRV,
    TYPE_NSEC,
    TYPE_ANY,
    MdnsHeader,
    MdnsQuestion,
    MdnsQuestionIter,
    MdnsRecord,
    MdnsRecordIter,
    MdnsSrv,
    MdnsTxtEntry,
    MdnsTxtIter,
    MdnsPacket,
    isMdns,
    parseMdns,
    parseMdnsSrv,
    parseMdnsA,
    parseMdnsAaaa,
    encodeMdnsQuery,
} from "cap:iot";
```

## 核心 API

- **查询编码与解析**：`encodeMdnsQuery(qname, qtype, qu, out: []u8): usize?`、`isMdns(data)`、`parseMdns(data): MdnsPacket?`。
- **报文视图**：`MdnsPacket` 提供 `decodeName(offset, out: []u8): usize?`、`questions(): MdnsQuestionIter`、`records(): MdnsRecordIter`（统一迭代 Answer / Authority / Additional 全部 RR）。
- **DNS-SD 记录辅助**：`parseMdnsSrv(rec): MdnsSrv?`、`parseMdnsA(rec): [4]u8?`、`parseMdnsAaaa(rec): [16]u8?`、`MdnsTxtIter`（零拷贝迭代 `key=value` 与无 `=` 布尔键）。

## 示例

```cp
import std from "std";
import { TYPE_PTR, encodeMdnsQuery, parseMdns } from "cap:iot";

fn example() {
    let wire: [128]u8 = std.mem.zeroes([128]u8);
    if (n := encodeMdnsQuery("_http._tcp.local", TYPE_PTR, true, wire[0..])) {
        if (pkt := parseMdns(wire[0..n])) {
            let qit = pkt.questions();
            if (q := qit.next()) {
                let name_buf: [256]u8 = std.mem.zeroes([256]u8);
                * = pkt.decodeName(q.name_offset, name_buf[0..]);
            }
        }
    }
}
```
