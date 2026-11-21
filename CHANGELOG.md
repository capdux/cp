# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:s7` Siemens S7comm / TPKT (RFC 1006) / COTP (ISO 8073) & S7-Any PLC zero-alloc parser（5/5 PASS，自动动态挂载 `cap.s7`）
新增纯 CP 零分配标准库 cap:s7（Siemens S7comm / TPKT / COTP 与 S7-Any 地址解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:knx` KNXnet/IP (ISO 14543-3-2) HPAI, cEMI frame, address & DPT9 float zero-alloc codec（5/5 PASS，自动动态挂载 `cap.knx`）
新增纯 CP 零分配标准库 cap:knx（ISO 14543-3-2 KNXnet/IP、HPAI、cEMI 帧与 DPT9 浮点解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:smb` SMB2/SMB3 (MS-SMB2) header, Compound chain, Negotiate & Transform zero-alloc parser（5/5 PASS，自动动态挂载 `cap.smb`）
新增纯 CP 零分配标准库 cap:smb（MS-SMB2 SMB2/SMB3 报头、复合链、Negotiate 与 Transform 加密头解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:bacnet` BACnet/IP (ASHRAE 135) BVLC, NPDU, APDU, Object ID & Tag TLV zero-alloc parser（5/5 PASS，自动动态挂载 `cap.bacnet`）
新增纯 CP 零分配标准库 cap:bacnet（ASHRAE 135 BACnet/IP BVLC/NPDU/APDU、Object ID 与 Tag TLV 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:dnp3` DNP3 (IEEE 1815) SCADA link/transport/app codec & 16B chunked CRC-16 (0xA6BC)（5/5 PASS，自动动态挂载 `cap.dnp3`）
新增纯 CP 零分配标准库 cap:dnp3（IEEE 1815 DNP3 链路/传输/应用层与 16B 分块 CRC-16 编解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:ntlm` NTLM v1/v2 (MS-NLMP) Negotiate/Challenge/Authenticate & AV_PAIR TLV zero-alloc parser（5/5 PASS，自动动态挂载 `cap.ntlm`）
新增纯 CP 零分配标准库 cap:ntlm（MS-NLMP NTLM v1/v2 Type 1/2/3 与 AV_PAIR TLV 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:can` CAN 2.0/FD (ISO 11898), SocketCAN, SAE J1939 PGN & DBC bit-signal zero-alloc codec（5/5 PASS，自动动态挂载 `cap.can`）
新增纯 CP 零分配标准库 cap:can（ISO 11898 CAN 2.0/FD、SocketCAN 编解码、J1939 PGN 与 DBC 位域信号提取）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:modbus` Modbus TCP (MBAP) & RTU (CRC-16/Modbus) industrial automation zero-alloc codec（5/5 PASS，自动动态挂载 `cap.modbus`）
新增纯 CP 零分配标准库 cap:modbus（Modbus TCP MBAP、Modbus RTU CRC-16 与寄存器迭代编解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:socks` SOCKS5 (RFC 1928/1929) & SOCKS4/4a proxy handshake, UDP & address zero-alloc codec（5/5 PASS，自动动态挂载 `cap.socks`）
新增纯 CP 零分配标准库 cap:socks（RFC 1928/1929 SOCKS5 握手/认证/UDP 与 SOCKS4/4a 编解码）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:turn` TURN (RFC 8656) attributes, IPv4/IPv6 XOR-ADDRESS & ChannelData zero-alloc codec（5/5 PASS，自动动态挂载 `cap.turn`）
新增纯 CP 零分配标准库 cap:turn（RFC 8656 TURN 属性、IPv4/IPv6 XOR-ADDRESS 与 ChannelData 编解码）与 5/5 深度测试

