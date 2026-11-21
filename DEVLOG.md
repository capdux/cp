## 2026-11-22: 新建 `cap:hid` USB HID 1.11 Report Descriptor short/long item stream & layout summary zero-alloc parser（5/5 PASS，文档直写 `doc/lib/hid.md`）
新增 cap:hid 纯 CP 零分配外设协议库（Short/Long Item 解码、有符号值扩展、16 层 Push/Pop 栈与 Collection 嵌套平衡校验），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:pci` PCI/PCIe 256B/4096B config space, 32/64-bit BAR & Capability list zero-alloc parser（5/5 PASS，文档直写 `doc/lib/pci.md`）
新增 cap:pci 纯 CP 零分配总线硬件配置库（Endpoint/Bridge 配置头、BAR 解码与 48/96 跳防环 Capability 遍历），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:iscsi` iSCSI (RFC 7143) 48B BHS, SCSI/Login PDU, Key=Value iter & CRC32C zero-alloc parser（5/5 PASS，文档直写 `doc/lib/iscsi.md`）
新增 cap:iscsi 纯 CP 零分配存储网络协议库（Castagnoli CRC32C 双摘要校验、48B BHS/AHS/4B 对齐 Data Segment 与 Login 键值零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:acpi` ACPI 6.5 RSDP/SDT/RSDT/XSDT/MADT/MCFG firmware tables zero-alloc parser（5/5 PASS，文档直写 `doc/lib/acpi.md`）
新增 cap:acpi 纯 CP 零分配固件描述表库（RSDP v1/v2 双校验和、36B SDT 通用表、MADT/MCFG 零拷贝条目迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:smbios` SMBIOS 2.1/3.0+ (DMTF) Entry Point, structure table & BIOS/System/Memory zero-alloc parser（5/5 PASS，文档直写 `doc/lib/smbios.md`）
新增 cap:smbios 纯 CP 零分配系统管理固件表库（双锚点双校验和、1-based 字符串区迭代与 Type 0/1/17 视图解析），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:nfs` ONC RPC v2 (RFC 5531) & NFSv3 (RFC 1813) Record Mark, Call/Reply & fattr3 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/nfs.md`）
新增 cap:nfs 纯 CP 零分配网络文件系统协议库（TCP Record Mark、ONC RPC v2 Call/Reply、AUTH_SYS 与 NFSv3 filehandle/84B fattr3），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:edid` VESA EDID 1.3/1.4 128B display block, PNP ID, Detailed/Standard Timing zero-alloc parser（5/5 PASS，文档直写 `doc/lib/edid.md`）
新增 cap:edid 纯 CP 零分配显示硬件元数据库（128B 块校验和、5-bit PNP ID 编解码、Preferred Timing 刷新率与 Monitor Name/Serial 描述符提取），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:gpt` UEFI GPT & Protective MBR partition table, UTF-16LE name & IEEE CRC32 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/gpt.md`）
新增 cap:gpt 纯 CP 零分配磁盘分区表库（MBR 保护分区识别、GPT 92B 头与分区数组双 CRC32 校验、UTF-16LE 名称解码与 mixed-endian GUID 格式化），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:xdr` XDR (RFC 4506) 4B-aligned big-endian scalar, opaque & string zero-alloc codec（5/5 PASS，文档直写 `doc/lib/xdr.md`）
新增 cap:xdr 纯 CP 零分配外部数据表示标准库（全标量、定/变长 Opaque 零填充校验与 XdrWriter 自动对齐），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:nbns` NBNS (RFC 1001/1002) NetBIOS 32B first-level name codec, NB & NBSTAT zero-alloc parser（5/5 PASS，文档直写 `doc/lib/nbns.md`）
新增 cap:nbns 纯 CP 零分配局域网名称服务库（RFC 1001 半字节编解码、NB 多 IP 与 NBSTAT 节点表/MAC 零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:gopher` Gopher (RFC 1436) selector/search request & Menu item zero-alloc codec（5/5 PASS，文档直写 `doc/lib/gopher.md`）
新增 cap:gopher 纯 CP 零分配经典信息检索协议库（根目录/搜索请求往返、GopherMenuIter 零拷贝迭代与防注入编码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:llmnr` LLMNR (RFC 4795) link-local multicast query/response & C/TC/T flags zero-alloc codec（5/5 PASS，文档直写 `doc/lib/llmnr.md`）
新增 cap:llmnr 纯 CP 零分配链路本地名称解析协议库（Query/A/AAAA 编解码往返、C/TC/T 标志位与 0xC0 压缩指针防环解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:mdns` mDNS (RFC 6762) & DNS-SD (RFC 6763) QU/Cache-Flush, SRV/TXT & name-compression codec（5/5 PASS，文档直写 `doc/lib/mdns.md`）
新增 cap:mdns 纯 CP 零分配零配置服务发现协议库（QU/Cache-Flush 位、PTR/SRV/TXT/A/AAAA 解析、<=16 跳防环 0xC0 压缩解码与 Query 编码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:kerberos` Kerberos v5 (RFC 4120) ASN.1 DER Ticket, KRB-ERROR, PrincipalName & TCP record parser（5/5 PASS，文档直写 `doc/lib/kerberos.md`）
新增 cap:kerberos 纯 CP 零分配企业认证协议库（TCP 4B Record Mark 解封、APPLICATION [1] Ticket 与 [30] KRB-ERROR 零拷贝 DER 解析），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ssdp` SSDP (UPnP/DLNA) M-SEARCH/NOTIFY/200 OK header parser & encoder（5/5 PASS，文档直写 `doc/lib/ssdp.md`）
新增 cap:ssdp 纯 CP 零分配局域网服务发现库（M-SEARCH/NOTIFY/HTTP 200 解析、max-age 提取与防 CRLF 注入编码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:s7` Siemens S7comm / TPKT (RFC 1006) / COTP (ISO 8073) & S7-Any PLC zero-alloc parser（5/5 PASS，文档直写 `doc/lib/s7.md`）
新增 cap:s7 纯 CP 零分配工业 PLC 通信协议库（TPKT/COTP DT 解封、S7 10B/12B 报头与 12B S7-Any 变量项零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:knx` KNXnet/IP (ISO 14543-3-2) HPAI, cEMI frame, address & DPT9 float zero-alloc codec（5/5 PASS，文档直写 `doc/lib/knx.md`）
新增 cap:knx 纯 CP 零分配智能建筑总线协议库（6B 报头、HPAI/ConnHeader/cEMI 分层解包、物理/组地址与 DPT9 2B 浮点解码），通过 cpc check 与 5/5 cap test



## 2026-11-22: 新建 `cap:smb` SMB2/SMB3 (MS-SMB2) header, Compound chain, Negotiate & Transform zero-alloc parser（5/5 PASS，文档直写 `doc/lib/smb.md`）
新增 cap:smb 纯 CP 零分配企业文件共享协议库（NetBIOS 解封、64B Sync/Async 报头、8B 对齐复合链与 52B SMB3 Transform 头），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:bacnet` BACnet/IP (ASHRAE 135) BVLC, NPDU, APDU, Object ID & Tag TLV zero-alloc parser（5/5 PASS，文档直写 `doc/lib/bacnet.md`）
新增 cap:bacnet 纯 CP 零分配楼宇自动化协议库（BVLC/NPDU/APDU 分层解包、32-bit Object ID 与扩展 Context/Application Tag TLV 零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:dnp3` DNP3 (IEEE 1815) SCADA link/transport/app codec & 16B chunked CRC-16 (0xA6BC)（5/5 PASS，文档直写 `doc/lib/dnp3.md`）
新增 cap:dnp3 纯 CP 零分配电力与 SCADA 协议库（0x0564 帧头、0xA6BC 分块 CRC-16 编解码往返、Transport 与含 IIN 的 Application 层解析），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ntlm` NTLM v1/v2 (MS-NLMP) Negotiate/Challenge/Authenticate & AV_PAIR TLV zero-alloc parser（5/5 PASS，文档直写 `doc/lib/ntlm.md`）
新增 cap:ntlm 纯 CP 零分配企业认证协议库（NTLMSSP 签名、Security Buffer 边界校验与 AV_PAIR 零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:can` CAN 2.0/FD (ISO 11898), SocketCAN, SAE J1939 PGN & DBC bit-signal zero-alloc codec（5/5 PASS，文档直写 `doc/lib/can.md`）
新增 cap:can 纯 CP 零分配车载与工业总线库（CAN/CAN FD DLC 映射、SocketCAN LE/BE 往返、J1939 PDU1/PDU2 与 Intel/Motorola DBC 信号提取），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:modbus` Modbus TCP (MBAP) & RTU (CRC-16/Modbus) industrial automation zero-alloc codec（5/5 PASS，文档直写 `doc/lib/modbus.md`）
新增 cap:modbus 纯 CP 零分配工业自动化协议库（MBAP 7B 帧、RTU CRC-16/Modbus 0xA001 校验与编码往返、ModbusRegIter），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:socks` SOCKS5 (RFC 1928/1929) & SOCKS4/4a proxy handshake, UDP & address zero-alloc codec（5/5 PASS，文档直写 `doc/lib/socks.md`）
新增 cap:socks 纯 CP 零分配代理协议库（SOCKS5 Greeting/UserPass/Request/Reply/UDP 与 SOCKS4/4a 双向编解码），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:turn` TURN (RFC 8656) attributes, IPv4/IPv6 XOR-ADDRESS & ChannelData zero-alloc codec（5/5 PASS，文档直写 `doc/lib/turn.md`）
新增 cap:turn 纯 CP 零分配 NAT 中继协议库（20B 报头、4B 对齐属性迭代、XOR-ADDRESS 双向编解码与 4B ChannelData 帧），通过 cpc check 与 5/5 cap test

