# `cap:ipsec` — 零分配 RFC 4302 AH / RFC 4303 ESP / RFC 7296 IKEv2 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**：`lib/ipsec/`（入口 `lib/ipsec/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 RFC 4303 ESP 报头与解密尾剥离、RFC 4302 AH 认证头与 ICV 提取，以及 RFC 7296 IKEv2 28 字节报头与链式 Generic Payload TLV 零拷贝迭代。

---

## 一、导入方式

```cp
import {
    IPSEC_PROTO_ESP,
    IPSEC_PROTO_AH,
    IKE_UDP_PORT,
    IKE_NATT_UDP_PORT,
    ESP_MIN_HEADER_SIZE,
    AH_MIN_HEADER_SIZE,
    IKEV2_HEADER_SIZE,
    IKE_EXCH_SA_INIT,
    IKE_EXCH_AUTH,
    IKE_EXCH_CREATE_CHILD_SA,
    IKE_EXCH_INFORMATIONAL,
    IKE_FLAG_INITIATOR,
    IKE_FLAG_VERSION,
    IKE_FLAG_RESPONSE,
    PL_NONE,
    PL_SA,
    PL_KE,
    PL_IDI,
    PL_IDR,
    PL_CERT,
    PL_CERTREQ,
    PL_AUTH,
    PL_NONCE,
    PL_NOTIFY,
    PL_DELETE,
    PL_VENDOR_ID,
    PL_TSI,
    PL_TSR,
    PL_SK,
    EspPacket,
    EspTrailer,
    AhPacket,
    Ikev2Header,
    Ikev2Payload,
    Ikev2PayloadIter,
    parseEsp,
    parseEspTrailer,
    parseAh,
    parseIkev2,
    isIkev2,
} from "cap:net";
```

---

## 二、核心数据结构

| 符号 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `EspPacket` | `spi`, `seq_num`, `payload_and_trailer` | RFC 4303 ESP 报头视图 |
| `EspTrailer` | `inner_payload`, `pad_len`, `next_header`, `icv` | 解密后剥离尾部填充与 ICV 的视图 |
| `AhPacket` | `next_header`, `payload_len_words`, `header_bytes`, `spi`, `seq_num`, `icv`, `payload` | RFC 4302 AH 报头视图 |
| `Ikev2Header` | `initiator_spi`, `responder_spi`, `next_payload`, `major_version`, `minor_version`, `exchange_type`, `flags`, `message_id`, `length`, `payloads_slice`; `isInitiator()`, `isResponse()`, `payloads()` | RFC 7296 IKEv2 28B 报头视图 |
| `Ikev2Payload` | `payload_type`, `next_payload`, `is_critical`, `body` | 单个 IKEv2 Generic Payload 视图 |

---

## 三、核心 API

- `parseEsp(data)` / `parseEspTrailer(decrypted_payload, icv_len)` — 解析 ESP 报头及剥离解密后的 `pad_len` + `next_header` + ICV。
- `parseAh(data)` — 严格校验 `reserved == 0` 与 `header_bytes = (payload_len_words + 2) * 4`。
- `parseIkev2(data)` / `isIkev2(data)` — 严格校验 `major_version == 2` 与 `28 <= length <= data.len`。
- `Ikev2PayloadIter` — 按 `next_payload` 链式零分配迭代所有 Generic Payload，直至 `PL_NONE (0)`。

---

## 四、使用示例

```cp
import { parseIkev2 } from "cap:net";

fn inspectIkev2(pkt: []const u8) {
    if (hdr := parseIkev2(pkt)) {
        let it = hdr.payloads();
        while (pl := it.next()) {
            * = pl.payload_type;
            * = pl.is_critical;
            * = pl.body;
        }
    }
}
```
