# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:tpm` TCG TPM 2.0 command/response header, TPMS_ATTEST/Quote, PCR Selection & TPMT_PUBLIC codec（5/5 PASS，自动动态挂载 `cap.tpm`）
新增纯 CP 零分配标准库 cap:tpm（TCG TPM 2.0 命令头、TPMS_ATTEST/Quote、PCR Selection 与 TPMT_PUBLIC 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:apfs` Apple APFS Fletcher-64 checksum, Object Header, Container (NXSB) & Volume (APSB) parser（5/5 PASS，自动动态挂载 `cap.apfs`）
新增纯 CP 零分配标准库 cap:apfs（Apple APFS Fletcher-64 校验和、Container NXSB 与 Volume APSB 超级块解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:erofs` Linux EROFS 128B superblock, Compact/Extended Inode, Dirent & CRC32C zero-alloc parser（5/5 PASS，自动动态挂载 `cap.erofs`）
新增纯 CP 零分配标准库 cap:erofs（Linux EROFS 128B 超级块、Compact/Extended Inode、Dirent 与 CRC32C 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:luks` Linux LUKS1 (592B/8 keyslots) & LUKS2 (4096B dual header & JSON area) zero-alloc parser（5/5 PASS，自动动态挂载 `cap.luks`）
新增纯 CP 零分配标准库 cap:luks（Linux LUKS1 592B/8 密钥槽与 LUKS2 4096B 主备头/JSON 元数据解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:vhd` Microsoft VHD 1.0 Footer, Dynamic Header, BAT lookup & one's-complement checksum parser（5/5 PASS，自动动态挂载 `cap.vhd`）
新增纯 CP 零分配标准库 cap:vhd（Microsoft VHD 1.0 Footer、Dynamic Header、BAT 查表与一补码校验和）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:xfs` Linux XFS v4/v5 superblock, CRC32C, Inode Core & 128-bit BMBT Extent zero-alloc codec（5/5 PASS，自动动态挂载 `cap.xfs`）
新增纯 CP 零分配标准库 cap:xfs（Linux XFS v4/v5 超级块、CRC32C、Inode Core 与 128-bit BMBT Extent 编解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:exfat` Microsoft exFAT boot sector, 11-sector Boot Checksum & 32B directory entry set parser（5/5 PASS，自动动态挂载 `cap.exfat`）
新增纯 CP 零分配标准库 cap:exfat（Microsoft exFAT 引导扇区、Boot Checksum 与 32B 目录项集合解析）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:vdi` VirtualBox VDI 1.1 virtual disk header & Block Allocation Map zero-alloc parser（5/5 PASS，自动动态挂载 `cap.vdi`）
新增纯 CP 零分配标准库 cap:vdi（VirtualBox VDI 1.1 虚拟磁盘报头与 Block Allocation Map 查表解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:btrfs` Linux Btrfs 4KB superblock, CRC32C, B-Tree 101B header & sys_chunk_array zero-alloc parser（5/5 PASS，自动动态挂载 `cap.btrfs`）
新增纯 CP 零分配标准库 cap:btrfs（Linux Btrfs 4KB 超级块、CRC32C、B-Tree 节点头与 sys_chunk_array 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:vmdk` VMware VMDK SparseExtentHeader 512B binary & text Descriptor/Extent zero-alloc parser（5/5 PASS，自动动态挂载 `cap.vmdk`）
新增纯 CP 零分配标准库 cap:vmdk（VMware VMDK 512B 稀疏报头与文本 Descriptor/Extent 解析）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:vhdx` Microsoft VHDX 1.0 FileId, dual Header, Region Table, BAT & CRC32C zero-alloc parser（5/5 PASS，自动动态挂载 `cap.vhdx`）
新增纯 CP 零分配标准库 cap:vhdx（Microsoft VHDX 1.0 FileId、双 Header 选主、Region Table、BAT 与 CRC32C 校验）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:qcow2` QEMU QCOW2 v2/v3 virtual disk header, Header Extensions & L1/L2 entry zero-alloc parser（5/5 PASS，自动动态挂载 `cap.qcow2`）
新增纯 CP 零分配标准库 cap:qcow2（QEMU QCOW2 v2/v3 虚拟磁盘报头、Header Extensions 与 L1/L2 簇表解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:scsi` SCSI (SPC-5/SBC-4) CDB codec, INQUIRY, READ CAPACITY(10/16) & Sense Data zero-alloc parser（5/5 PASS，自动动态挂载 `cap.scsi`）
新增纯 CP 零分配标准库 cap:scsi（SCSI SPC-5/SBC-4 CDB 编码、INQUIRY、READ CAPACITY 与 Fixed/Descriptor Sense 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:ntfs` NTFS VBR, MFT FILE record, Resident/Non-Resident attributes & Data Runlist parser（5/5 PASS，自动动态挂载 `cap.ntfs`）
新增纯 CP 零分配标准库 cap:ntfs（Microsoft NTFS VBR、MFT FILE 记录、属性与 Data Runlist 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:squashfs` SquashFS v4.0 96B superblock, Metadata/Data block header, Fragment & InodeRef parser（5/5 PASS，自动动态挂载 `cap.squashfs`）
新增纯 CP 零分配标准库 cap:squashfs（Linux SquashFS v4.0 96B 超级块、Metadata/Data 块头与 Fragment 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:ext4` Linux ext2/ext3/ext4 superblock, 64-bit Inode, Extent tree & dir zero-alloc parser（5/5 PASS，自动动态挂载 `cap.ext4`）
新增纯 CP 零分配标准库 cap:ext4（Linux ext2/ext3/ext4 超级块、64-bit Inode、Extent 树与线性目录解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:fat` FAT12/16/32 BPB, 32B directory entry, 8.3 name, DOS time & LFN checksum zero-alloc parser（5/5 PASS，自动动态挂载 `cap.fat`）
新增纯 CP 零分配标准库 cap:fat（FAT12/16/32 BPB、32B 目录项、DOS 时间与 LFN 校验和解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:hid` USB HID 1.11 Report Descriptor short/long item stream & layout summary zero-alloc parser（5/5 PASS，自动动态挂载 `cap.hid`）
新增纯 CP 零分配标准库 cap:hid（USB HID 1.11 Report Descriptor 短/长项迭代与报告布局摘要）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:pci` PCI/PCIe 256B/4096B config space, 32/64-bit BAR & Capability list zero-alloc parser（5/5 PASS，自动动态挂载 `cap.pci`）
新增纯 CP 零分配标准库 cap:pci（PCI/PCIe 配置空间、32/64-bit BAR 与标准/扩展 Capability 防环链表解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:iscsi` iSCSI (RFC 7143) 48B BHS, SCSI/Login PDU, Key=Value iter & CRC32C zero-alloc parser（5/5 PASS，自动动态挂载 `cap.iscsi`）
新增纯 CP 零分配标准库 cap:iscsi（RFC 7143 iSCSI 48B BHS、SCSI/Login PDU、键值迭代与 CRC32C 校验）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:acpi` ACPI 6.5 RSDP/SDT/RSDT/XSDT/MADT/MCFG firmware tables zero-alloc parser（5/5 PASS，自动动态挂载 `cap.acpi`）
新增纯 CP 零分配标准库 cap:acpi（ACPI 6.5 RSDP/SDT/RSDT/XSDT/MADT/MCFG 固件系统描述表解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:smbios` SMBIOS 2.1/3.0+ (DMTF) Entry Point, structure table & BIOS/System/Memory zero-alloc parser（5/5 PASS，自动动态挂载 `cap.smbios`）
新增纯 CP 零分配标准库 cap:smbios（DMTF SMBIOS 2.1/3.0+ Entry Point、结构表迭代与 BIOS/System/Memory 视图）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:nfs` ONC RPC v2 (RFC 5531) & NFSv3 (RFC 1813) Record Mark, Call/Reply & fattr3 zero-alloc parser（5/5 PASS，自动动态挂载 `cap.nfs`）
新增纯 CP 零分配标准库 cap:nfs（RFC 5531 ONC RPC v2 与 RFC 1813 NFSv3 Record Mark/Call/Reply/fattr3 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:edid` VESA EDID 1.3/1.4 128B display block, PNP ID, Detailed/Standard Timing zero-alloc parser（5/5 PASS，自动动态挂载 `cap.edid`）
新增纯 CP 零分配标准库 cap:edid（VESA EDID 1.3/1.4 显示器识别块、PNP ID、Detailed/Standard Timing 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:gpt` UEFI GPT & Protective MBR partition table, UTF-16LE name & IEEE CRC32 zero-alloc parser（5/5 PASS，自动动态挂载 `cap.gpt`）
新增纯 CP 零分配标准库 cap:gpt（UEFI GPT 与 Protective MBR 分区表、双 IEEE CRC32 与 GUID 格式化）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:xdr` XDR (RFC 4506) 4B-aligned big-endian scalar, opaque & string zero-alloc codec（5/5 PASS，自动动态挂载 `cap.xdr`）
新增纯 CP 零分配标准库 cap:xdr（RFC 4506 XDR 4 字节对齐大端编解码器 XdrReader/XdrWriter）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:nbns` NBNS (RFC 1001/1002) NetBIOS 32B first-level name codec, NB & NBSTAT zero-alloc parser（5/5 PASS，自动动态挂载 `cap.nbns`）
新增纯 CP 零分配标准库 cap:nbns（RFC 1001/1002 NetBIOS 32B 一级编码与 NB/NBSTAT 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:gopher` Gopher (RFC 1436) selector/search request & Menu item zero-alloc codec（5/5 PASS，自动动态挂载 `cap.gopher`）
新增纯 CP 零分配标准库 cap:gopher（RFC 1436 Gopher 请求与 Menu 目录条目编解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:llmnr` LLMNR (RFC 4795) link-local multicast query/response & C/TC/T flags zero-alloc codec（5/5 PASS，自动动态挂载 `cap.llmnr`）
新增纯 CP 零分配标准库 cap:llmnr（RFC 4795 LLMNR 链路本地组播查询/响应与防环压缩域名编解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:mdns` mDNS (RFC 6762) & DNS-SD (RFC 6763) QU/Cache-Flush, SRV/TXT & name-compression codec（5/5 PASS，自动动态挂载 `cap.mdns`）
新增纯 CP 零分配标准库 cap:mdns（RFC 6762/6763 mDNS & DNS-SD 报头、QU/Cache-Flush、SRV/TXT 与防环压缩域名编解码）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:kerberos` Kerberos v5 (RFC 4120) ASN.1 DER Ticket, KRB-ERROR, PrincipalName & TCP record parser（5/5 PASS，自动动态挂载 `cap.kerberos`）
新增纯 CP 零分配标准库 cap:kerberos（RFC 4120 Kerberos v5 ASN.1 DER Ticket/KRB-ERROR/PrincipalName 解析）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:ssdp` SSDP (UPnP/DLNA) M-SEARCH/NOTIFY/200 OK header parser & encoder（5/5 PASS，自动动态挂载 `cap.ssdp`）
新增纯 CP 零分配标准库 cap:ssdp（UPnP/DLNA SSDP M-SEARCH/NOTIFY/200 OK 编解码与头部迭代）与 5/5 深度测试

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

