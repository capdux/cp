[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dns-packet

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 1035 DNS binary packet encoder/decoder with 12-byte header, domain label encoding, 0xC0 compression pointer decoding, and query builder.

## 安装与导入

```bash
cap add dns-packet
```

```cp
import { putBeU16 } from "dns-packet";
```

## 核心导出 API

- `export fn putBeU16(dst: []u8, offset: usize, v: u16);`
- `export fn getBeU16(src: &[]u8, offset: usize): u16;`
- `export fn encodeHeader(h: DnsHeader, dst: []u8): usize;`
- `export fn decodeHeader(src: &[]u8): DnsHeader;`
- `export fn encodeName(domain: &[]u8, dst: []u8): i64;`
- `export fn decodeName(packet: &[]u8, offset: usize, dst: []u8): NameDecodeResult;`
- `export fn encodeQuery(id: u16, domain: &[]u8, qtype: u16, rd: bool, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`dns-packet@1.0.0`)，许可证：MIT
