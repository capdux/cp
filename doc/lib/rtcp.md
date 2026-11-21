# `cap:rtcp` — RFC 3550 RTCP 复合包零分配线协议解析库

纯 CP 实现、零堆分配的 RFC 3550 Real-time Transport Control Protocol 解析库，支持复合包迭代、Padding 自动剥离、SR/RR 24B ReportBlock（含 24-bit 有符号累计丢包）、BYE Reason、APP 与 SDES Item 解析。

## 导入

```cp
import {
    RTCP_VERSION,
    PT_SR,
    PT_RR,
    PT_SDES,
    PT_BYE,
    PT_APP,
    RtcpPacket,
    RtcpCompoundIter,
    RtcpReportBlock,
    RtcpSr,
    RtcpRr,
    RtcpBye,
    RtcpApp,
    isRtcp,
    parseRtcpPacket,
    iterRtcpCompound,
    parseRtcpSr,
    parseRtcpRr,
    parseRtcpBye,
    parseRtcpApp,
} from "cap:rtcp";
```

## 核心 API

- `isRtcp(data)` / `parseRtcpPacket(data): RtcpPacket?` / `iterRtcpCompound(data): RtcpCompoundIter`
- `parseRtcpSr(pkt): RtcpSr?`（`reports(): RtcpReportBlockIter`）
- `parseRtcpRr(pkt): RtcpRr?`（`reports(): RtcpReportBlockIter`）
- `parseRtcpBye(pkt): RtcpBye?`（`ssrcAt(idx): u32?`、`reason`）
- `parseRtcpApp(pkt): RtcpApp?`（`subtype`、`ssrc`、`name`、`data`）
- `RtcpSdesChunk` / `RtcpSdesItemIter`

## 使用示例

```cp
import { iterRtcpCompound, parseRtcpSr, PT_SR } from "cap:rtcp";

fn handleCompound(udp_payload: []const u8) {
    let it = iterRtcpCompound(udp_payload);
    while (pkt := it.next()) {
        if (pkt.header.pt == PT_SR) {
            const sr = parseRtcpSr(pkt) ?? continue;
            * = sr.packet_count;
        }
    }
}
```
