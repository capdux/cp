# @cap/dns-packet

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dns-packet` ([https://github.com/mafintosh/dns-packet](https://github.com/mafintosh/dns-packet))

Zero-heap RFC 1035 DNS binary packet encoder and decoder in pure CP (npm `dns-packet` parity).

## Features

- 12-byte big-endian `DnsHeader` encoding & decoding
- Standard record type constants (`TYPE_A`, `TYPE_NS`, `TYPE_CNAME`, `TYPE_SOA`, `TYPE_PTR`, `TYPE_MX`, `TYPE_TXT`, `TYPE_AAAA`, `TYPE_SRV`) & `CLASS_IN`
- Domain label encoding (`encodeName`) with root `"."` and trailing-dot normalization
- RFC 1035 `0xC0` compression pointer decoding (`decodeName`) with max 16-jump cycle guard
- One-call standard query builder (`encodeQuery`)

## Usage

```cp
import { encodeQuery, TYPE_A } from "@cap/dns-packet";

let pkt: [128]u8 = undefined;
const len = encodeQuery(0x1234, "api.capdux.org", TYPE_A, true, pkt[0..128]);
```
