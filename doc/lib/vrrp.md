# `cap:vrrp` — RFC 3768 VRRPv2 & RFC 5798 VRRPv3 零分配线协议解析库

纯 CP 实现、零堆分配的 VRRPv2 / VRRPv3 虚拟路由冗余协议报文解析、IPv4/IPv6 虚拟地址迭代与校验和计算/验证库。

## 导入

```cp
import {
    VRRP_IP_PROTO,
    VRRP_V2,
    VRRP_V3,
    VRRP_TYPE_ADVERTISEMENT,
    VRRP_HEADER_SIZE,
    VRRP_V2_AUTH_SIZE,
    VRRP_AUTH_NONE,
    VRRP_AUTH_SIMPLE,
    VRRP_AUTH_AH,
    VrrpHeader,
    VrrpIpv4Iter,
    VrrpIpv6Iter,
    VrrpPacket,
    isVrrp,
    parseVrrp,
    vrrp2Checksum,
    verifyVrrp2Checksum,
    vrrp3ChecksumIpv4,
    verifyVrrp3ChecksumIpv4,
    vrrp3ChecksumIpv6,
    verifyVrrp3ChecksumIpv6,
} from "cap:vrrp";
```

## 核心 API

- **嗅探与解析**：
  - `isVrrp(data: []const u8): bool`
  - `parseVrrp(data: []const u8, is_ipv6: bool): VrrpPacket?`
- **虚拟地址迭代**：
  - `VrrpPacket.ipv4Addrs(): VrrpIpv4Iter` -> `next(*_): ([4]u8)?`
  - `VrrpPacket.ipv6Addrs(): VrrpIpv6Iter` -> `next(*_): ([16]u8)?`
- **校验和**：
  - VRRPv2（无伪首部）：`vrrp2Checksum(data)` / `verifyVrrp2Checksum(data)`
  - VRRPv3 over IPv4（含 12B IPv4 伪首部）：`vrrp3ChecksumIpv4(src_ip, dst_ip, data)` / `verifyVrrp3ChecksumIpv4(...)`
  - VRRPv3 over IPv6（含 40B IPv6 伪首部）：`vrrp3ChecksumIpv6(src_ip, dst_ip, data)` / `verifyVrrp3ChecksumIpv6(...)`

## 使用示例

```cp
import { parseVrrp, verifyVrrp2Checksum } from "cap:vrrp";

fn inspectVrrp2(raw: []const u8) {
    if (!verifyVrrp2Checksum(raw)) { return; }
    const pkt = parseVrrp(raw, false) ?? return;
    let it = pkt.ipv4Addrs();
    while (vip := it.next()) {
        * = vip[0];
    }
}
```
