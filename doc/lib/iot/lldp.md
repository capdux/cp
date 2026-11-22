# `cap:lldp` — 零分配 IEEE 802.1AB LLDP 链路层发现协议编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

> **模块路径**：`lib/lldp/`（入口 `lib/lldp/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 IEEE 802.1AB-2016 LLDPDU 规范：支持 2 字节大端 7-bit Type / 9-bit Length（最长 511B）TLV 零拷贝迭代、必选前 3 TLV（`Chassis ID` -> `Port ID` -> `TTL`）顺序强校验、邻居能力位掩码判定、组织自定义 TLV (`127`) 快捷解码以及零分配 `LldpWriter` 编码器（自动追加 `0x00 0x00` End-of-LLDPDU）。

---

## 一、导入方式

```cp
import {
    LLDP_ETHERTYPE,
    TLV_END,
    TLV_CHASSIS_ID,
    TLV_PORT_ID,
    TLV_TTL,
    TLV_PORT_DESC,
    TLV_SYSTEM_NAME,
    TLV_SYSTEM_DESC,
    TLV_SYSTEM_CAPS,
    TLV_MGMT_ADDR,
    TLV_ORG_SPECIFIC,
    CAP_REPEATER,
    CAP_BRIDGE,
    CAP_WLAN_AP,
    CAP_ROUTER,
    CAP_TELEPHONE,
    CAP_STATION_ONLY,
    LldpTlv,
    LldpOrgTlv,
    LldpNeighbor,
    LldpTlvIter,
    LldpWriter,
    parseLldpOrgTlv,
    parseLldpdu,
} from "cap:iot";
```

---

## 二、核心常量与数据结构

| 符号 | 说明 |
| :--- | :--- |
| EtherType | `LLDP_ETHERTYPE = 0x88CC` |
| TLV Type | `TLV_END (0)`, `TLV_CHASSIS_ID (1)`, `TLV_PORT_ID (2)`, `TLV_TTL (3)`, `TLV_PORT_DESC (4)`, `TLV_SYSTEM_NAME (5)`, `TLV_SYSTEM_DESC (6)`, `TLV_SYSTEM_CAPS (7)`, `TLV_MGMT_ADDR (8)`, `TLV_ORG_SPECIFIC (127)` |
| 能力位掩码 | `CAP_REPEATER (0x0002)`, `CAP_BRIDGE (0x0004)`, `CAP_WLAN_AP (0x0008)`, `CAP_ROUTER (0x0010)`, `CAP_TELEPHONE (0x0020)`, `CAP_STATION_ONLY (0x0080)` |
| `LldpTlv` | `tlv_type: u8`, `length: u16`, `value: []const u8` |
| `LldpOrgTlv` | `oui: [3]u8`, `subtype: u8`, `info: []const u8` |
| `LldpNeighbor` | `chassis_subtype`, `chassis_id`, `port_subtype`, `port_id`, `ttl_sec`, `port_desc`, `system_name`, `system_desc`, `caps_supported`, `caps_enabled`, `has_end_tlv`, `org_tlv_count`; `isRouter()`, `isBridge()`, `isWlanAp()` |

---

## 三、核心 API

- `LldpTlvIter` — 零拷贝迭代 LLDPDU 中的 TLV，遇 `TLV_END (0)` 且 `length == 0` 正常终止，异常置 `hasError() == true`。
- `parseLldpdu(data: []const u8): LldpNeighbor?` — 严格按 `Chassis ID -> Port ID -> TTL` 顺序解析完整邻居信息。
- `parseLldpOrgTlv(tlv: LldpTlv): LldpOrgTlv?` — 解码组织自定义 TLV（type `127`）的 3B OUI、1B Subtype 与负载。
- `LldpWriter` — 零分配编码器：`init(buf)`、`writeChassisId`、`writePortId`、`writeTtl`、`writeStringTlv`、`writeSystemCaps`、`writeOrgTlv`、`finish()`。

---

## 四、使用示例

```cp
import {
    TLV_SYSTEM_NAME,
    CAP_BRIDGE,
    CAP_ROUTER,
    LldpWriter,
    parseLldpdu,
} from "cap:iot";

fn roundtripLldp() {
    let buf: [256]u8 = [0; 256];
    let w = LldpWriter.init(buf[0..256]);
    * = w.writeChassisId(4, "\x00\x11\x22\x33\x44\x55");
    * = w.writePortId(5, "Eth1/1");
    * = w.writeTtl(120);
    * = w.writeStringTlv(TLV_SYSTEM_NAME, "core-sw-01");
    * = w.writeSystemCaps(CAP_BRIDGE | CAP_ROUTER, CAP_BRIDGE | CAP_ROUTER);
    if (pdu := w.finish()) {
        if (nbr := parseLldpdu(pdu)) {
            * = nbr.isRouter();
            * = nbr.system_name;
        }
    }
}
```
