# @cap/der

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:der` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

零堆分配的 CP ITU-T X.690 ASN.1 DER TLV 编解码引擎 — 洁净室 MIT 实现，对标 RustCrypto `der`（crates.io #274，6.13 亿+ 总下载），服务 X.509 证书、PKI 与 JWT/ASN.1 安全协议。

Zero-allocation ITU-T X.690 ASN.1 DER TLV encoder & strict canonical decoder for CP — clean-room MIT parity with RustCrypto `der` (613M+ downloads).

## 安装 / Install

```bash
cap add @cap/der
```

## API 参考 / API Reference

- **Tag 常量**：`TAG_BOOLEAN`, `TAG_INTEGER`, `TAG_BIT_STRING`, `TAG_OCTET_STRING`, `TAG_NULL`, `TAG_OID`, `TAG_UTF8_STRING`, `TAG_PRINTABLE_STRING`, `TAG_SEQUENCE`, `TAG_SET`
- **解析与解码**：`DerTlv`、`parseDerTlv(input)`、`decodeDerBool(tlv)`、`decodeDerU64(tlv)`
- **规范编码**：`encodeDerTlv(tag, val, out)`、`encodeDerBool(v, out)`、`encodeDerNull(out)`、`encodeDerU64(v, out)`、`encodeDerOctetString(data, out)`、`encodeDerSequence(inner, out)`

## 使用示例 / Usage

```cp
import { encodeDerU64, parseDerTlv, decodeDerU64 } from "@cap/der";

let buf: [32]u8 = undefined;
const enc = encodeDerU64(128 as u64, buf[0..32]).?; // 02 02 00 80
const tlv = parseDerTlv(enc);
const val = decodeDerU64(tlv).?; // 128
```

## License

MIT
