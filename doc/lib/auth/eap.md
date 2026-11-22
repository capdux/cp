# `cap:eap` — 零分配 RFC 3748 EAP + IEEE 802.1X EAPOL 线协议解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:auth`，导入请使用 `import { ... } from "cap:auth"`。

> **模块路径**：`lib/eap/`（入口 `lib/eap/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 IEEE 802.1X EAPOL 帧解包、RFC 3748 EAP Request / Response / Success / Failure 严格解析，以及 Section 5.7 Expanded Type (`254`) 24-bit Vendor-ID / 32-bit Vendor-Type 零拷贝提取。

---

## 一、导入方式

```cp
import {
    EAPOL_ETHERTYPE,
    EAPOL_HEADER_SIZE,
    EAP_HEADER_SIZE,
    EAP_EXPANDED_HEADER_SIZE,
    EAP_TYPE_IDENTITY,
    EAP_TYPE_NOTIFICATION,
    EAP_TYPE_NAK,
    EAP_TYPE_MD5_CHALLENGE,
    EAP_TYPE_OTP,
    EAP_TYPE_GTC,
    EAP_TYPE_TLS,
    EAP_TYPE_TTLS,
    EAP_TYPE_AKA,
    EAP_TYPE_PEAP,
    EAP_TYPE_MSCHAPV2,
    EAP_TYPE_EXPANDED,
    EapolType,
    EapCode,
    EapolFrame,
    EapPacket,
    EapExpanded,
    parseEapol,
    isEapol,
    parseEap,
    isEap,
    parseEapExpanded,
} from "cap:auth";
```

---

## 二、核心常量与数据结构

| 符号 | 说明 |
| :--- | :--- |
| 协议常量 | `EAPOL_ETHERTYPE (0x888E)`, `EAPOL_HEADER_SIZE (4)`, `EAP_HEADER_SIZE (4)`, `EAP_EXPANDED_HEADER_SIZE (8)` |
| EAP Type | `EAP_TYPE_IDENTITY (1)`, `NOTIFICATION (2)`, `NAK (3)`, `MD5_CHALLENGE (4)`, `OTP (5)`, `GTC (6)`, `TLS (13)`, `TTLS (21)`, `AKA (23)`, `PEAP (25)`, `MSCHAPV2 (26)`, `EXPANDED (254)` |
| `EapolType: u8` | `eap_packet (0)`, `start (1)`, `logoff (2)`, `key (3)`, `encap_asf_alert (4)`, `unknown (255)` |
| `EapCode: u8` | `request (1)`, `response (2)`, `success (3)`, `failure (4)` |
| `EapolFrame` | `version`, `pkt_type`, `raw_type`, `body_len`, `body`; `asEap()` |
| `EapPacket` | `code`, `identifier`, `length`, `eap_type: u8?`, `type_data` |
| `EapExpanded` | `vendor_id: u32` (24-bit), `vendor_type: u32`, `vendor_data` |

---

## 三、核心 API

- `parseEapol(data)` / `isEapol(data)` — 严格校验 `version in 1..3` 与 `4 + body_len <= data.len`。
- `parseEap(data)` / `isEap(data)` — 严格校验 `code in 1..4`、Success/Failure `length == 4`、Request/Response `5 <= length <= data.len`。
- `parseEapExpanded(type_data)` — 解析 `eap_type == 254` 的 3B Vendor-ID + 4B Vendor-Type + 变长负载。

---

## 四、使用示例

```cp
import { parseEapol, parseEapExpanded, EAP_TYPE_EXPANDED } from "cap:auth";

fn inspect8021x(frame_bytes: []const u8) {
    if (eapol := parseEapol(frame_bytes)) {
        if (eap := eapol.asEap()) {
            if (et := eap.eap_type) {
                if (et == EAP_TYPE_EXPANDED) {
                    if (exp := parseEapExpanded(eap.type_data)) {
                        * = exp.vendor_id;
                        * = exp.vendor_type;
                    }
                }
            }
        }
    }
}
```
