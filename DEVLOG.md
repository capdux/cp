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

