# cap:proxy_protocol_v2

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（HAProxy PROXY Protocol v2 二进制规范），提供纯 CP、零外部依赖、零堆分配的 PROXY v2 头部解析、编码与 TLV 扩展流式迭代库。

## 导入方式

```cp
import {
    PROXY_V2_SIGNATURE,
    hasProxyV2Signature,
    ProxyV2Command,
    ProxyV2Family,
    ProxyV2Transport,
    ProxyV2Tlv,
    ProxyV2TlvIter,
    ProxyV2Header,
} from "cap:proxy_protocol_v2";
```

## 核心类型与 API

- `PROXY_V2_SIGNATURE: [12]u8`：12 字节固定二进制签名 `\x0D\x0A\x0D\x0A\x00\x0D\x0AQUIT\x0A`。
- `hasProxyV2Signature(buf: []const u8): bool`：快速前缀签名嗅探。
- `ProxyV2Command`：`local` / `proxy`。
- `ProxyV2Family`：`unspec` / `inet4` / `inet6` / `unix`。
- `ProxyV2Transport`：`unspec` / `stream` / `dgram`。
- `ProxyV2Header`：
  - 字段：`command`、`family`、`transport`、`src_ipv4: [4]u8`、`dst_ipv4: [4]u8`、`src_ipv6: [16]u8`、`dst_ipv6: [16]u8`、`src_port: u16`、`dst_port: u16`、`tlvs_raw: []const u8`、`header_len: usize`（`16 + payload_len`）。
  - `ProxyV2Header.parse(buf: []const u8): ProxyV2Header?`：校验签名、`ver == 2`、合法命令/地址族/传输协议与地址及 TLV 边界防越界。
  - `ProxyV2Header.encodeIpv4(src, dst, src_port, dst_port, out): ([]const u8)?`：编码 28 字节 IPv4/TCP PROXY 头。
  - `ProxyV2Header.encodeLocal(out): ([]const u8)?`：编码 16 字节 `LOCAL` 头。
- `ProxyV2Tlv`（`tlv_type: u8`、`value: []const u8`）与 `ProxyV2TlvIter`（`init` / `next(*_): ProxyV2Tlv?`）：流式解析 `[type:1][len:2 BE][val:len]` 扩展（如 `0x01` ALPN、`0x02` AUTHORITY）。

## 使用示例

```cp
import { ProxyV2Header, ProxyV2TlvIter } from "cap:proxy_protocol_v2";

fn example() {
    let out: [32]u8 = undefined;
    const src: [4]u8 = [203, 0, 113, 10];
    const dst: [4]u8 = [198, 51, 100, 1];
    if (encoded := ProxyV2Header.encodeIpv4(src, dst, 54321, 443, &out)) {
        if (hdr := ProxyV2Header.parse(encoded)) {
            let it = ProxyV2TlvIter.init(hdr.tlvs_raw);
            while (tlv := it.next()) {
                * = tlv;
            }
        }
    }
}
```
