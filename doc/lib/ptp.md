# `cap:ptp` — IEEE 1588-2008 / 1588-2019 PTPv2/v2.1 零分配线协议编解码库

纯 CP 实现、零堆分配的精密时间协议 (PTP v2 / v2.1) 公共报头、48-bit 秒时间戳、Announce 与 Delay_Resp 编解码库。

## 导入

```cp
import {
    PTP_ETHERTYPE,
    PTP_UDP_EVENT_PORT,
    PTP_UDP_GENERAL_PORT,
    PTP_V2,
    PTP_HEADER_SIZE,
    PTP_TIMESTAMP_SIZE,
    MSG_SYNC,
    MSG_FOLLOW_UP,
    MSG_DELAY_RESP,
    MSG_ANNOUNCE,
    FLAG_TWO_STEP,
    FLAG_UNICAST,
    PtpTimestamp,
    PtpPortIdentity,
    PtpHeader,
    PtpPacket,
    PtpAnnounceBody,
    PtpDelayRespBody,
    isPtp,
    parsePtp,
    parsePtpTimestamp,
    parsePtpAnnounce,
    parsePtpDelayResp,
    encodePtpHeader,
} from "cap:ptp";
```

## 核心 API

- **嗅探与解析**：`isPtp(data)`、`parsePtp(data): PtpPacket?`、`parsePtpTimestamp(data): PtpTimestamp?`、`parsePtpAnnounce(body): PtpAnnounceBody?`、`parsePtpDelayResp(body): PtpDelayRespBody?`
- **编码**：`encodePtpHeader(hdr: PtpHeader, out: *[]u8): usize?`
- **时间与标志辅助**：
  - `PtpTimestamp.toNanoseconds(): u128`、`PtpTimestamp.fromNanoseconds(ns: u128)`
  - `PtpHeader.correctionNs(): i64`、`isEvent()`、`isTwoStep()`、`isUnicast()`

## 使用示例

```cp
import { parsePtp, parsePtpTimestamp, MSG_SYNC } from "cap:ptp";

fn inspectSync(raw: []const u8) {
    const pkt = parsePtp(raw) ?? return;
    if (pkt.header.msg_type == MSG_SYNC) {
        const ts = parsePtpTimestamp(pkt.body) ?? return;
        * = ts.toNanoseconds();
    }
}
```
