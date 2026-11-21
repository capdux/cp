# `cap:radius` — RFC 2865 / 2866 RADIUS 认证与计费协议零分配编解码库

`cap:radius` 提供纯 CP、零堆分配的 RFC 2865 / 2866 / 5176 RADIUS 二进制报文编解码能力（大端序），支持 20 字节固定报头、AVP 属性迭代与查询、Vendor-Specific Attributes (Type 26) 多子 TLV 解码以及零分配报文编码器 `RadiusWriter`。

## 导入方式

```cp
import {
    RADIUS_HEADER_SIZE,
    RADIUS_MAX_PACKET_LEN,
    RADIUS_AUTH_PORT,
    RADIUS_ACCT_PORT,
    ATTR_USER_NAME,
    ATTR_FRAMED_IP_ADDRESS,
    ATTR_VENDOR_SPECIFIC,
    ATTR_SESSION_TIMEOUT,
    ATTR_MESSAGE_AUTHENTICATOR,
    RadiusCode,
    RadiusAttr,
    RadiusVsa,
    RadiusPacket,
    RadiusAttrIter,
    RadiusVsaIter,
    RadiusWriter,
    parseRadiusPacket,
} from "cap:radius";
```

## 常量与枚举

- 协议常量：`RADIUS_HEADER_SIZE = 20`, `RADIUS_MAX_PACKET_LEN = 4096`, `RADIUS_AUTH_PORT = 1812`, `RADIUS_ACCT_PORT = 1813`
- 常用属性：`ATTR_USER_NAME (1)`, `ATTR_USER_PASSWORD (2)`, `ATTR_NAS_IP_ADDRESS (4)`, `ATTR_NAS_PORT (5)`, `ATTR_SERVICE_TYPE (6)`, `ATTR_FRAMED_IP_ADDRESS (8)`, `ATTR_REPLY_MESSAGE (18)`, `ATTR_STATE (24)`, `ATTR_CLASS (25)`, `ATTR_VENDOR_SPECIFIC (26)`, `ATTR_SESSION_TIMEOUT (27)`, `ATTR_CALLED_STATION_ID (30)`, `ATTR_CALLING_STATION_ID (31)`, `ATTR_NAS_IDENTIFIER (32)`, `ATTR_ACCT_STATUS_TYPE (40)`, `ATTR_ACCT_SESSION_ID (44)`, `ATTR_MESSAGE_AUTHENTICATOR (80)`
- `RadiusCode: u8`：`access_request`, `access_accept`, `access_reject`, `accounting_request`, `accounting_response`, `access_challenge`, `status_server`, `status_client`, `disconnect_request`, `disconnect_ack`, `disconnect_nak`, `coa_request`, `coa_ack`, `coa_nak`, `unknown`

## 核心结构与 API

- `parseRadiusPacket(data): RadiusPacket?` — 校验 `20 <= length <= data.len && length <= 4096`，零拷贝切分 `attrs_slice`
- `RadiusPacket`：`code`, `raw_code`, `identifier`, `length`, `authenticator: [16]u8`, `attrs_slice`，附 `attrs(): RadiusAttrIter`、`findAttr(attr_type)`、`findU32Attr(attr_type)`
- `RadiusAttr`（附 `asU32(): u32?`）& `RadiusAttrIter`
- `RadiusVsa`（`vendor_id: u32`, `vendor_type: u8`, `value: []const u8`）& `RadiusVsaIter`
- `RadiusWriter`：`init(buf, code, identifier, authenticator)`、`writeAttr(attr_type, value)`、`writeU32Attr(attr_type, v)`、`writeVsa(vendor_id, vtype, value)`、`finish(): ([]const u8)?`

## 快速示例

```cp
import { RadiusWriter, RadiusCode, ATTR_USER_NAME, ATTR_SESSION_TIMEOUT, parseRadiusPacket } from "cap:radius";

fn roundtrip() {
    let buf: [256]u8 = [0; 256];
    let auth: [16]u8 = [0x11; 16];
    let w = RadiusWriter.init(buf[0..], RadiusCode.access_accept, 7, auth);
    * = w.writeAttr(ATTR_USER_NAME, "alice");
    * = w.writeU32Attr(ATTR_SESSION_TIMEOUT, 3600);
    if (pkt_bytes := w.finish()) {
        if (pkt := parseRadiusPacket(pkt_bytes)) {
            * = pkt.findU32Attr(ATTR_SESSION_TIMEOUT);
        }
    }
}
```
