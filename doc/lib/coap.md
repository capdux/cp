# `cap:coap` — 零分配 RFC 7252 CoAP 物联网轻量 REST 协议编解码库

> **模块路径**：`lib/coap/`（入口 `lib/coap/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 7252 Constrained Application Protocol 规范：支持 4 字节报头 + 0..8B Token、Delta/Length 扩展编码（`0..12` / `13 (+1B)` / `14 (+2B)`）、`0xFF` Payload Marker 严格校验、零拷贝 `CoapOptIter` 与零分配 `CoapWriter` 编码器。

---

## 一、导入方式

```cp
import {
    COAP_UDP_PORT,
    COAPS_UDP_PORT,
    COAP_VERSION,
    COAP_HEADER_SIZE,
    COAP_PAYLOAD_MARKER,
    COAP_CON,
    COAP_NON,
    COAP_ACK,
    COAP_RST,
    CODE_EMPTY,
    CODE_GET,
    CODE_POST,
    CODE_PUT,
    CODE_DELETE,
    CODE_CREATED,
    CODE_DELETED,
    CODE_VALID,
    CODE_CHANGED,
    CODE_CONTENT,
    CODE_BAD_REQUEST,
    CODE_NOT_FOUND,
    CODE_INTERNAL_ERROR,
    OPT_IF_MATCH,
    OPT_URI_HOST,
    OPT_ETAG,
    OPT_OBSERVE,
    OPT_URI_PORT,
    OPT_URI_PATH,
    OPT_CONTENT_FORMAT,
    OPT_MAX_AGE,
    OPT_URI_QUERY,
    OPT_ACCEPT,
    OPT_BLOCK2,
    OPT_BLOCK1,
    OPT_SIZE1,
    coapCode,
    CoapOption,
    CoapOptIter,
    CoapMessage,
    CoapWriter,
    parseCoap,
    isCoap,
} from "cap:coap";
```

---

## 二、核心 API

- `coapCode(class: u8, detail: u8): u8` — 按 3-bit class 与 5-bit detail 构造 8-bit CoAP Code。
- `parseCoap(data)` / `isCoap(data)` — 严格校验 `version == 1`、`tkl <= 8`、全部 Option Delta/Length 边界与 `0xFF` 后至少 1B Payload。
- `CoapMessage` — 提供 `codeClass()`, `codeDetail()`, `isEmpty()`, `isRequest()`, `isResponse()`, `options()`, `findOption(num)`, `contentFormat()`。
- `CoapOptIter` — 零分配迭代全部 Delta 编码 Option。
- `CoapWriter` — 零分配编码器：`init`, `writeOption`, `writeUintOption`, `writePayload`, `finish`。

---

## 三、使用示例

```cp
import {
    COAP_CON,
    CODE_GET,
    OPT_URI_PATH,
    CoapWriter,
    parseCoap,
} from "cap:coap";

fn sendCoapGet() {
    let buf: [128]u8 = [0; 128];
    let w = CoapWriter.init(buf[0..128], COAP_CON, CODE_GET, 0x1234, "\xAB\xCD");
    * = w.writeOption(OPT_URI_PATH, "sensors");
    * = w.writeOption(OPT_URI_PATH, "temp");
    if (pkt := w.finish()) {
        if (msg := parseCoap(pkt)) {
            * = msg.isRequest();
            * = msg.token;
        }
    }
}
```
