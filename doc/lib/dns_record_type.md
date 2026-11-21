# cap:dns_record_type

> 源码吸收自 `/data/dev/bun/src/dns.zig`（IANA DNS RRType、Class、RCODE 与 RFC 2782 SRV / RFC 1035 MX 规范），提供纯 CP、零外部依赖、零堆分配的 DNS 记录类型编解码与文本记录解析库。

## 导入方式

```cp
import {
    DnsRrType,
    DnsRcode,
    DnsClass,
    SrvRecord,
    MxRecord,
} from "cap:dns_record_type";
```

## 核心类型与 API

- `DnsRrType`：`a`(1), `ns`(2), `cname`(5), `soa`(6), `ptr`(12), `mx`(15), `txt`(16), `aaaa`(28), `srv`(33), `opt`(41), `ds`(43), `rrsig`(46), `nsec`(47), `dnskey`(48), `svcb`(64), `https`(65), `any_q`(255), `caa`(257)。
  - `fromU16(code: u16): DnsRrType?` / `toU16(): u16` / `parseName(name: []const u8): DnsRrType?`（大小写无关）/ `toName(): []const u8`。
- `DnsRcode`：`noerror`(0), `formerr`(1), `servfail`(2), `nxdomain`(3), `notimp`(4), `refused`(5), `yxdomain`(6), `nxrrset`(8), `notauth`(9), `notzone`(10)。
  - `fromU16(code: u16): DnsRcode?` / `toU16(): u16` / `toName(): []const u8` / `isError(): bool`。
- `DnsClass`：`in_class`(1), `ch`(3), `hs`(4), `any_class`(255)。
  - `fromU16(code: u16): DnsClass?` / `toU16(): u16` / `parseName(name: []const u8): DnsClass?` / `toName(): []const u8`。
- `SrvRecord`：`priority: u16`, `weight: u16`, `port: u16`, `target: []const u8`，提供 `SrvRecord.parseText(rdata: []const u8): SrvRecord?`。
- `MxRecord`：`preference: u16`, `exchange: []const u8`，提供 `MxRecord.parseText(rdata: []const u8): MxRecord?`。

## 使用示例

```cp
import { DnsRrType, DnsRcode, SrvRecord, MxRecord } from "cap:dns_record_type";

fn example() {
    if (rr := DnsRrType.parseName("aaaa")) {
        * = rr.toU16(); // 28
    }
    if (rc := DnsRcode.fromU16(3)) {
        * = rc.isError(); // true (NXDOMAIN)
    }
    if (srv := SrvRecord.parseText("10 60 5060 sipserver.example.com.")) {
        * = srv.port; // 5060
    }
    if (mx := MxRecord.parseText("10 mail.example.com.")) {
        * = mx.preference; // 10
    }
}
```
