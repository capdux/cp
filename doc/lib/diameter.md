# `cap:diameter` — 零分配 RFC 6733 Diameter AAA 协议编解码库

> **模块路径**：`lib/diameter/`（入口 `lib/diameter/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 6733 Diameter Base Protocol 大端序规范：支持 20 字节报头与 24-bit Command-Code / Message-Length 解析、8B 标准与 12B Vendor-Specific 4 字节对齐 AVP 零拷贝迭代、强类型数值提取及零分配 `DiameterWriter` 编码器（自动 0 填充与总长度回填）。

---

## 一、导入方式

```cp
import {
    DIAMETER_HEADER_SIZE,
    DIAMETER_VERSION,
    DIAMETER_PORT,
    DIAM_FLAG_REQUEST,
    DIAM_FLAG_PROXIABLE,
    DIAM_FLAG_ERROR,
    DIAM_FLAG_RETRANSMIT,
    AVP_FLAG_VENDOR,
    AVP_FLAG_MANDATORY,
    AVP_FLAG_PROTECTED,
    CMD_CER_CEA,
    CMD_ACR_ACA,
    CMD_CCR_CCA,
    CMD_DWR_DWA,
    CMD_DPR_DPA,
    CMD_AIR_AIA,
    AVP_AUTH_APP_ID,
    AVP_SESSION_ID,
    AVP_ORIGIN_HOST,
    AVP_VENDOR_ID,
    AVP_RESULT_CODE,
    AVP_DEST_REALM,
    AVP_DEST_HOST,
    AVP_ORIGIN_REALM,
    DiameterHeader,
    DiameterAvp,
    DiameterMessage,
    DiameterAvpIter,
    DiameterWriter,
    parseDiameterMessage,
    isDiameter,
} from "cap:diameter";
```

---

## 二、核心数据结构与常量

| 符号 | 说明 |
| :--- | :--- |
| 协议常量 | `DIAMETER_HEADER_SIZE = 20`, `DIAMETER_VERSION = 1`, `DIAMETER_PORT = 3868` |
| 报头标志 | `DIAM_FLAG_REQUEST` (`0x80`), `DIAM_FLAG_PROXIABLE` (`0x40`), `DIAM_FLAG_ERROR` (`0x20`), `DIAM_FLAG_RETRANSMIT` (`0x10`) |
| AVP 标志 | `AVP_FLAG_VENDOR` (`0x80`), `AVP_FLAG_MANDATORY` (`0x40`), `AVP_FLAG_PROTECTED` (`0x20`) |
| Command Code | `CMD_CER_CEA` (`257`), `CMD_ACR_ACA` (`271`), `CMD_CCR_CCA` (`272`), `CMD_DWR_DWA` (`280`), `CMD_DPR_DPA` (`282`), `CMD_AIR_AIA` (`318`) |
| AVP Code | `AVP_AUTH_APP_ID` (`258`), `AVP_SESSION_ID` (`263`), `AVP_ORIGIN_HOST` (`264`), `AVP_VENDOR_ID` (`266`), `AVP_RESULT_CODE` (`268`), `AVP_DEST_REALM` (`283`), `AVP_DEST_HOST` (`293`), `AVP_ORIGIN_REALM` (`296`) |
| `DiameterHeader` | `version`, `msg_len`, `flags`, `command_code`, `application_id`, `hop_by_hop_id`, `end_to_end_id`; `isRequest()`, `isProxiable()`, `isError()`, `isRetransmit()` |
| `DiameterAvp` | `code`, `flags`, `avp_len`, `vendor_id`, `data`; `isVendor()`, `isMandatory()`, `isProtected()`, `asU32()`, `asI32()`, `asU64()` |
| `DiameterMessage` | `header`, `avps_slice`; `isRequest()`, `isProxiable()`, `isError()`, `isRetransmit()`, `avps()`, `findAvp(code)` |

---

## 三、核心 API

- `parseDiameterMessage(data: []const u8): DiameterMessage?` — 严格校验并解析 20B 报头及 AVP 切片。
- `isDiameter(data: []const u8): bool` — 快速判定输入是否符合 Diameter 协议报头规范。
- `DiameterAvpIter` — 零分配迭代 4 字节对齐的 AVP（支持 8B 与 12B Vendor-Specific 头部）。
- `DiameterWriter` — 零分配报文编码器：`init(buf, flags, command_code, app_id, hbh_id, e2e_id)`、`writeAvp(...)`、`writeU32Avp(...)`、`finish(): ([]const u8)?`。

---

## 四、使用示例

```cp
import {
    DIAM_FLAG_REQUEST,
    DIAM_FLAG_PROXIABLE,
    AVP_FLAG_MANDATORY,
    CMD_CCR_CCA,
    AVP_SESSION_ID,
    AVP_ORIGIN_HOST,
    AVP_RESULT_CODE,
    DiameterWriter,
    parseDiameterMessage,
} from "cap:diameter";

fn roundtripCcr() {
    let buf: [256]u8 = [0; 256];
    let w = DiameterWriter.init(
        buf[0..256],
        DIAM_FLAG_REQUEST | DIAM_FLAG_PROXIABLE,
        CMD_CCR_CCA,
        4,
        0x11223344,
        0x55667788,
    );
    * = w.writeAvp(AVP_SESSION_ID, AVP_FLAG_MANDATORY, 0, "pgw.example.com;1;2");
    * = w.writeAvp(AVP_ORIGIN_HOST, AVP_FLAG_MANDATORY, 0, "pgw.example.com");
    * = w.writeU32Avp(AVP_RESULT_CODE, true, 2001);
    if (pkt := w.finish()) {
        if (msg := parseDiameterMessage(pkt)) {
            if (sid := msg.findAvp(AVP_SESSION_ID)) {
                * = sid.data;
            }
        }
    }
}
```
