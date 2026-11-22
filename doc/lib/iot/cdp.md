# `cap:cdp` — Cisco Discovery Protocol v1/v2 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

纯 CP、零堆分配的 Cisco Discovery Protocol（CDP v1/v2）4 字节头、RFC 1071 16-bit 校验和与全套 TLV 零拷贝编解码库。

## 导入

```cp
import {
    CDP_SNAP_PID,
    CDP_V1,
    CDP_V2,
    CDP_HEADER_SIZE,
    CDP_TLV_HEADER_SIZE,
    TLV_DEVICE_ID,
    TLV_ADDRESSES,
    TLV_PORT_ID,
    TLV_CAPABILITIES,
    TLV_VERSION,
    TLV_PLATFORM,
    TLV_VTP_DOMAIN,
    TLV_NATIVE_VLAN,
    TLV_DUPLEX,
    TLV_MGMT_ADDR,
    CAP_ROUTER,
    CAP_TRANSPARENT_BRIDGE,
    CAP_SOURCE_ROUTE_BRIDGE,
    CAP_SWITCH,
    CAP_HOST,
    CAP_IGMP,
    CAP_REPEATER,
    cdpChecksum,
    verifyCdpChecksum,
    CdpHeader,
    CdpTlv,
    CdpTlvIter,
    CdpPacket,
    isCdp,
    parseCdp,
    CdpWriter,
} from "cap:iot";
```

## 核心 API

- **校验和**：`cdpChecksum(data: []const u8): u16`、`verifyCdpChecksum(data: []const u8): bool`（RFC 1071 16-bit 一补码，含奇数尾字节补零处理）。
- **解析与查询**：
  - `isCdp(data)`、`parseCdp(data): CdpPacket?`（校验版本 `1|2` 与所有 TLV 边界 `length >= 4 && offset + length <= data.len`）。
  - `CdpPacket`：`tlvs()`、`findTlv(tlv_type)`、`deviceId()`、`portId()`、`platform()`、`softwareVersion()`、`vtpDomain()`、`capabilities()`、`nativeVlan()`、`isFullDuplex()`。
- **零分配编码器**：`CdpWriter`（`init`、`writeTlv`、`writeCapabilities`、`writeNativeVlan`、`writeDuplex`、`finish`）。

## 示例

```cp
import std from "std";
import { CDP_V2, TLV_DEVICE_ID, TLV_PORT_ID, CAP_ROUTER, CAP_SWITCH, CdpWriter, parseCdp, verifyCdpChecksum } from "cap:iot";

fn example() {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = CdpWriter.init(buf[0..], CDP_V2, 180);
    * = w.writeTlv(TLV_DEVICE_ID, "core-sw-01");
    * = w.writeTlv(TLV_PORT_ID, "GigabitEthernet0/1");
    * = w.writeCapabilities(CAP_ROUTER | CAP_SWITCH);
    * = w.writeNativeVlan(100);
    * = w.writeDuplex(true);
    if (wire := w.finish()) {
        if (verifyCdpChecksum(wire)) {
            if (pkt := parseCdp(wire)) {
                * = pkt.deviceId();
            }
        }
    }
}
```
