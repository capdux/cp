# `cap:pim` — RFC 7761 PIM-SM v2 组播协议零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

纯 CP、零堆分配的 Protocol Independent Multicast - Sparse Mode（PIM-SM v2）4 字节头、Hello TLV 选项、Register 封装报文以及 IPv4/IPv6（含 Register 8B 特例）校验和库。

## 导入

```cp
import {
    PIM_IP_PROTO,
    PIM_VERSION,
    PIM_HEADER_SIZE,
    PIM_HELLO,
    PIM_REGISTER,
    PIM_REGISTER_STOP,
    PIM_JOIN_PRUNE,
    PIM_BOOTSTRAP,
    PIM_ASSERT,
    PIM_GRAFT,
    PIM_GRAFT_ACK,
    PIM_CANDIDATE_RP_ADV,
    OPT_HOLDTIME,
    OPT_LAN_PRUNE_DELAY,
    OPT_DR_PRIORITY,
    OPT_GENERATION_ID,
    OPT_ADDRESS_LIST,
    REG_FLAG_BORDER,
    REG_FLAG_NULL,
    pimChecksumIpv4,
    verifyPimChecksumIpv4,
    pimChecksumIpv6,
    verifyPimChecksumIpv6,
    PimHeader,
    PimHelloOption,
    PimHelloOptIter,
    PimPacket,
    PimRegister,
    parsePimRegister,
    isPim,
    parsePim,
    PimWriter,
} from "cap:iot";
```

## 核心 API

- **校验和（RFC 7761 Section 4.9）**：
  - `pimChecksumIpv4(data)` / `verifyPimChecksumIpv4(data)` — IPv4 无伪首部（Register 仅校验前 8B）。
  - `pimChecksumIpv6(src_ip, dst_ip, data)` / `verifyPimChecksumIpv6(src_ip, dst_ip, data)` — 含 40B IPv6 伪首部（Register 仅校验前 8B）。
- **解析**：
  - `isPim(data)`、`parsePim(data): PimPacket?`（校验 `version == 2`、`msg_type <= 8` 及 Hello TLV / Register 长度）。
  - `PimPacket`：`helloOptions()`, `findHelloOption(opt_type)`, `holdtime()`, `drPriority()`, `generationId()`。
  - `parsePimRegister(body): PimRegister?`（`flags`, `isBorder()`, `isNullRegister()`, `inner_packet`）。
- **零分配编码器**：`PimWriter`（`init`, `writeHelloOption`, `writeHoldtime`, `writeDrPriority`, `writeGenerationId`, `writeRegister`, `finishIpv4`, `finishIpv6`）。

## 示例

```cp
import std from "std";
import { PIM_HELLO, PimWriter, parsePim, verifyPimChecksumIpv4 } from "cap:iot";

fn example() {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = PimWriter.init(buf[0..], PIM_HELLO);
    * = w.writeHoldtime(105);
    * = w.writeDrPriority(100);
    * = w.writeGenerationId(0x1234_5678);
    if (wire := w.finishIpv4()) {
        if (verifyPimChecksumIpv4(wire)) {
            if (pkt := parsePim(wire)) {
                * = pkt.holdtime();
            }
        }
    }
}
```
