# `cap:knx` — ISO/IEC 14543-3-2 KNXnet/IP、cEMI 帧与 DPT9 浮点零分配库

> **模块路径**: `lib/knx/`  
> **导入协议**: `import { KNX_UDP_PORT, SVC_TUNNELLING_REQ, CEMI_L_DATA_IND, APCI_GROUP_WRITE, parseKnx, parseKnxHpai, parseKnxConnHeader, parseKnxCemi, encodeKnxHeader, decodeIndividualAddr, encodeIndividualAddr, decodeGroupAddr, encodeGroupAddr, decodeDpt9 } from "cap:knx";`  
> **动态挂载**: `import cap from "cap";` -> `cap.knx.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:knx` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 ISO/IEC 14543-3-2 KNXnet/IP 智能建筑/家居总线线协议库：

1. **KNXnet/IP 报文解析与头编码 (`isKnx` / `parseKnx` / `encodeKnxHeader`)**：
   - 严格校验 6B 头部长度、协议版本 `0x10` 与 `6 <= total_len <= data.len`。
   - 完整内置 Discovery、Connection Management、Tunnelling、Routing 服务类型常量。
2. **HPAI 与 Connection Header (`parseKnxHpai` / `parseKnxConnHeader`)**：
   - 零拷贝解析 8 字节 IPv4 UDP/TCP HPAI 端点与 4 字节 Tunnelling 连接头。
3. **cEMI L_Data 帧解析 (`parseKnxCemi`)**：
   - 自动处理可选 Additional Info、`ctrl1`/`ctrl2` 标志位、源/目的地址、10-bit APCI（`GroupValueRead`/`Response`/`Write`）、6-bit 短数据及多字节负载。
4. **物理/组地址与 DPT9 浮点 (`decodeIndividualAddr` / `decodeGroupAddr` / `decodeDpt9`)**：
   - 支持 `area.line.device` 与 3 级 `main/middle/sub` 地址双向编解码。
   - 完整实现 KNX DPT9 2 字节浮点解码：`0.01 * signed_11bit_mantissa * 2^exp_4bit`。

---

## 二、 核心 API 速查表 (`lib/knx/knx.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `KNX_UDP_PORT (3671)`, `KNX_HEADER_SIZE (6)`, `KNX_PROTOCOL_V10 (0x10)`, `KNX_HPAI_SIZE (8)`, `KNX_CONN_HEADER_SIZE (4)` |
| Service / cEMI / APCI | `SVC_SEARCH_REQ` .. `SVC_ROUTING_INDICATION`；`CEMI_L_DATA_REQ (0x11)`, `CEMI_L_DATA_IND (0x29)`, `CEMI_L_DATA_CON (0x2E)`；`APCI_GROUP_READ (0x000)`, `APCI_GROUP_RESPONSE (0x040)`, `APCI_GROUP_WRITE (0x080)` |
| `KnxHeader` / `KnxPacket` | `isKnx(data)`、`parseKnx(data)`、`encodeKnxHeader(service_type, body_len, out)` |
| `KnxHpai` / `KnxConnHeader` | `parseKnxHpai(data)`、`parseKnxConnHeader(data)` |
| `KnxIndividualAddr` / `KnxGroupAddr` | `decodeIndividualAddr` / `encodeIndividualAddr`、`decodeGroupAddr` / `encodeGroupAddr` |
| `KnxCemi` | `parseKnxCemi(data)`；含 `isGroupAddress()`, `isStandardFrame()`, `priority()` |
| `decodeDpt9(raw: u16): f64` | 解码 KNX 2-byte DPT9 浮点数（温度、湿度、照度等） |

---

## 三、 使用示例

```typescript
import { parseKnx, parseKnxConnHeader, parseKnxCemi, decodeDpt9 } from "cap:knx";

fn demo(pkt_bytes: []const u8) {
    if (pkt := parseKnx(pkt_bytes)) {
        if (ch := parseKnxConnHeader(pkt.body)) {
            if (cemi := parseKnxCemi(pkt.body[4..])) {
                if (cemi.payload.len == 2) {
                    const raw: u16 = ((cemi.payload[0] as u16) << 8) | (cemi.payload[1] as u16);
                    const temp_c = decodeDpt9(raw);
                    * = ch;
                    * = temp_c;
                }
            }
        }
    }
}
```
