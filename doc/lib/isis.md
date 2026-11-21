# `cap:isis` — ISO/IEC 10589 / RFC 1195 IS-IS 零分配线协议解析库

纯 CP 实现、零堆分配的 IS-IS 链路状态路由协议解析库，支持 8B 公共报头、L1/L2 LSP 27B 固定报头、TLV 零拷贝迭代、Dynamic Hostname 提取与 ISO 8473 Fletcher-16 校验和。

## 导入

```cp
import {
    ISIS_NLPID,
    ISIS_COMMON_HEADER_SIZE,
    ISIS_VERSION,
    ISIS_LSP_HEADER_SIZE,
    PDU_L1_LAN_IIH,
    PDU_L2_LAN_IIH,
    PDU_P2P_IIH,
    PDU_L1_LSP,
    PDU_L2_LSP,
    TLV_AREA_ADDR,
    TLV_PROTOCOLS,
    TLV_IP_INTF_ADDR,
    TLV_HOSTNAME,
    IsisCommonHeader,
    IsisLspHeader,
    IsisTlv,
    IsisTlvIter,
    IsisPacket,
    isisFletcher16,
    isisLspChecksum,
    verifyIsisLspChecksum,
    isIsis,
    parseIsis,
    parseIsisLsp,
} from "cap:isis";
```

## 核心 API

- **嗅探与解析**：`isIsis(data)`、`parseIsis(data): IsisPacket?`、`parseIsisLsp(pkt): IsisLspHeader?`
- **TLV 迭代与查询**：`IsisPacket.tlvs(): IsisTlvIter`、`findTlv(code): IsisTlv?`、`hostname(): ([]const u8)?`
- **Fletcher-16 校验和**：`isisFletcher16(data): u16`、`isisLspChecksum(lsp_from_id): u16`、`verifyIsisLspChecksum(lsp_from_id): bool`

## 使用示例

```cp
import { parseIsis, parseIsisLsp, verifyIsisLspChecksum } from "cap:isis";

fn inspectLsp(raw: []const u8) {
    const pkt = parseIsis(raw) ?? return;
    const lsp = parseIsisLsp(pkt) ?? return;
    const valid = verifyIsisLspChecksum(raw[12..lsp.pdu_len as usize]);
    const name = pkt.hostname();
    * = valid;
    * = name;
}
```
