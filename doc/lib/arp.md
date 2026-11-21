# `cap:arp` — RFC 826 / RFC 5227 ARP 零分配编解码库

纯 CP、零堆分配的以太网 IPv4 ARP（28 字节）及通用变长 ARP 报文编解码库，内置 RFC 5227 ARP Probe 与 Gratuitous ARP / Announcement 运维判定。

## 导入

```cp
import {
    ARP_ETHERTYPE,
    ARP_IPV4_PACKET_SIZE,
    ARP_HTYPE_ETHERNET,
    ARP_PTYPE_IPV4,
    ARP_HLEN_ETHERNET,
    ARP_PLEN_IPV4,
    ArpOp,
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
} from "cap:arp";
```

## 常量与枚举

- `ARP_ETHERTYPE = 0x0806`、`ARP_IPV4_PACKET_SIZE = 28`、`ARP_HTYPE_ETHERNET = 1`、`ARP_PTYPE_IPV4 = 0x0800`、`ARP_HLEN_ETHERNET = 6`、`ARP_PLEN_IPV4 = 4`
- `ArpOp: u16`：`unknown(0)`, `request(1)`, `reply(2)`, `rarp_request(3)`, `rarp_reply(4)`, `inarp_request(8)`, `inarp_reply(9)`

## 核心 API

### 解析与判定
- `isArpIpv4(data: []const u8): bool` — 快速校验 28B Ethernet+IPv4 ARP 报文。
- `parseArpIpv4(data: []const u8): ArpPacket?` — 解析 28B 标准 Ethernet+IPv4 ARP。
  - 只读方法：`isRequest()`, `isReply()`, `isProbe()`（RFC 5227 `sender_ip == 0.0.0.0`）, `isGratuitous()`（`sender_ip == target_ip != 0.0.0.0`）, `isAnnouncement()`。
- `parseArpRaw(data: []const u8): ArpRawHeader?` — 通用变长 ARP 解析，返回零拷贝 `sha`/`spa`/`tha`/`tpa` 切片。

### 零分配编码
- `encodeArpIpv4(op, sender_mac, sender_ip, target_mac, target_ip, out): usize?`
- `encodeArpRequest(sender_mac, sender_ip, target_ip, out): usize?`
- `encodeArpReply(sender_mac, sender_ip, target_mac, target_ip, out): usize?`
- `encodeArpProbe(sender_mac, target_ip, out): usize?`
- `encodeGratuitousArp(sender_mac, ip, out): usize?`

## 示例

```cp
import std from "std";
import { encodeArpRequest, parseArpIpv4 } from "cap:arp";

fn example() {
    let buf: [28]u8 = std.mem.zeroes([28]u8);
    const mac: [6]u8 = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55];
    const sip: [4]u8 = [192, 168, 1, 10];
    const tip: [4]u8 = [192, 168, 1, 1];
    if (n := encodeArpRequest(mac, sip, tip, buf[0..])) {
        if (pkt := parseArpIpv4(buf[0..n])) {
            * = pkt.isRequest();
        }
    }
}
```
