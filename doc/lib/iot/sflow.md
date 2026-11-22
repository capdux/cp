# `cap:sflow` — sFlow v5 实时网络采样遥测零分配线协议解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

纯 CP 实现、零堆分配的 sFlow v5 数据报、Flow/Expanded Flow/Counter Sample 以及 Raw Packet Header / Extended Switch Record 解析库。

## 导入

```cp
import {
    SFLOW_UDP_PORT,
    SFLOW_V5,
    ADDR_TYPE_IPV4,
    ADDR_TYPE_IPV6,
    SAMPLE_FLOW,
    SAMPLE_COUNTERS,
    SAMPLE_EXPANDED_FLOW,
    REC_RAW_PACKET_HEADER,
    REC_EXTENDED_SWITCH,
    SflowDatagram,
    SflowFlowSample,
    SflowRawPacketHeader,
    SflowExtendedSwitch,
    isSflow,
    parseSflow,
    parseFlowSample,
    parseRawPacketHeader,
    parseExtendedSwitch,
} from "cap:iot";
```

## 核心 API

- `isSflow(data)` / `parseSflow(data): SflowDatagram?`
- `SflowDatagram.samples(): SflowSampleIter`
- `parseFlowSample(sample): SflowFlowSample?`（同时支持 `SAMPLE_FLOW (1)` 与 `SAMPLE_EXPANDED_FLOW (3)`）
- `SflowFlowSample.records(): SflowRecordIter`
- `parseRawPacketHeader(rec): SflowRawPacketHeader?`
- `parseExtendedSwitch(rec): SflowExtendedSwitch?`

## 使用示例

```cp
import { parseSflow, parseFlowSample, parseRawPacketHeader } from "cap:iot";

fn inspectSflow(udp: []const u8) {
    const dg = parseSflow(udp) ?? return;
    let sit = dg.samples();
    while (sample := sit.next()) {
        if (fs := parseFlowSample(sample)) {
            let rit = fs.records();
            while (rec := rit.next()) {
                if (raw := parseRawPacketHeader(rec)) {
                    * = raw.frame_len;
                }
            }
        }
    }
}
```
