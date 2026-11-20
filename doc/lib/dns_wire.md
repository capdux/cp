# `cap:dns_wire` — RFC 1035 / 6891 二进制 DNS 报文与指针压缩标准库

## 1. 概述
`cap:dns_wire`（动态挂载 `cap.dns_wire`）是纯 CP 实现的 RFC 1035 / EDNS0 (RFC 6891) 二进制 DNS 报文编解码库。支持 12 字节报文头、`0xC0xx` 后缀压缩编码与防循环指针炸弹（`CompressionLoop`）安全解码，覆盖 `A`/`AAAA`/`CNAME`/`NS`/`PTR`/`MX`/`TXT`/`SRV`/`SOA`/`OPT` 记录。

## 2. 核心类型
- `DnsWireError`：`UnexpectedEof`、`InvalidHeader`、`InvalidLabel`、`NameTooLong`、`CompressionLoop`、`TruncatedRecord`、`InvalidRdata`、`OutOfMemory`
- `DnsHeader`：`id`, `qr`, `opcode`, `aa`, `tc`, `rd`, `ra`, `ad`, `cd`, `rcode`, `qd_count`, `an_count`, `ns_count`, `ar_count`
- `DnsQuestion` / `DnsResourceRecord` / `DnsRdata` / `DnsPacket`

## 3. API 速查
- `DnsHeader.encode(self)` / `DnsHeader.decode(buf)`
- `encodeDnsName(buf, alloc, name, comp_entries)` / `decodeDnsName(alloc, packet, offset)`
- `encodeDnsPacket(alloc, pkt, compress_names)` -> `[]u8`
- `decodeDnsPacket(alloc, bytes)` -> `DnsPacket`（使用完毕调用 `pkt.deinit(alloc)`）

## 4. 示例
```cp
import { DnsHeader, DnsQuestion, DnsPacket, encodeDnsPacket, decodeDnsPacket } from "cap:dns_wire";
```
