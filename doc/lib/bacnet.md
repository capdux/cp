# `cap:bacnet` — ASHRAE 135 BACnet/IP 零分配楼宇自动化协议编解码库

纯 CP、零堆分配（zero-alloc）的 ASHRAE 135 BACnet/IP 线协议解析与编码库，完整覆盖 BVLC（含 Forwarded-NPDU）、NPDU（含 DNET/DADR/SNET/SADR/HopCount）、APDU（全 8 种 PDU 类型）、32-bit Object Identifier 及 Application / Context TLV 标签迭代。

## 导入

```cp
import {
    BACNET_UDP_PORT,
    BVLC_TYPE_IPV4,
    BVLC_HEADER_SIZE,
    NPDU_VERSION,
    BVLC_RESULT,
    BVLC_FORWARDED_NPDU,
    BVLC_REGISTER_FOREIGN_DEVICE,
    BVLC_ORIGINAL_UNICAST,
    BVLC_ORIGINAL_BROADCAST,
    APDU_CONFIRMED_REQ,
    APDU_UNCONFIRMED_REQ,
    APDU_SIMPLE_ACK,
    APDU_COMPLEX_ACK,
    APDU_SEGMENT_ACK,
    APDU_ERROR,
    APDU_REJECT,
    APDU_ABORT,
    SVC_I_AM,
    SVC_WHO_IS,
    SVC_READ_PROPERTY,
    SVC_READ_PROPERTY_MULTIPLE,
    SVC_WRITE_PROPERTY,
    isBvlc,
    BvlcPacket,
    parseBvlc,
    encodeBvlcUnicast,
    encodeBvlcBroadcast,
    NpduPacket,
    parseNpdu,
    ApduHeader,
    parseApdu,
    BacnetObjectId,
    decodeObjectId,
    encodeObjectId,
    BacnetTag,
    BacnetTagIter,
} from "cap:bacnet";
```

## 核心 API

- **BVLC**：`isBvlc(data)`、`parseBvlc(data): BvlcPacket?`（支持 Original-Unicast / Broadcast / Forwarded-NPDU 6B 源地址）、`encodeBvlcUnicast(npdu, out)`、`encodeBvlcBroadcast(npdu, out)`。
- **NPDU**：`parseNpdu(data): NpduPacket?`（严格校验 `version == 0x01`，完整解析可选 `dnet`/`dadr`/`snet`/`sadr`/`hop_count` 与 `apdu` 切片，`NpduPacket` 提供 `isNetworkMessage()`, `expectingReply()`, `priority()`）。
- **APDU**：`parseApdu(data): ApduHeader?`（覆盖 Confirmed-Request / Unconfirmed-Request / Simple-ACK / Complex-ACK / Segment-ACK / Error / Reject / Abort）。
- **Object Identifier**：`decodeObjectId(raw: u32): BacnetObjectId`（10-bit `object_type` + 22-bit `instance`）、`encodeObjectId(obj_type: u16, instance: u32): u32?`。
- **TLV 标签迭代器**：`BacnetTagIter`（`init`, `next`, `hasError`），零拷贝解码标准 0..4 字节长度、扩展 `tag_number == 15`、扩展长度 `lvt == 5`（1B/2B/4B）与 Context Opening/Closing 标签。

## 示例

```cp
import std from "std";
import { parseBvlc, parseNpdu, parseApdu, BacnetTagIter } from "cap:bacnet";

fn example(udp_pkt: []const u8) {
    if (bvlc := parseBvlc(udp_pkt)) {
        if (npdu := parseNpdu(bvlc.payload)) {
            if (!npdu.isNetworkMessage()) {
                if (apdu := parseApdu(npdu.apdu)) {
                    let it = BacnetTagIter.init(apdu.payload);
                    while (true) {
                        if (tag := it.next()) {
                            * = tag;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }
}
```
