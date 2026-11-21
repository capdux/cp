## 2026-11-22: 新建 `cap:yenc` yEnc v1.3 binary-to-text codec, =ybegin/=ypart/=yend headers & IEEE CRC32（5/5 PASS，文档直写 `doc/lib/yenc.md`）
新增 cap:yenc 纯 CP 零分配高效二进制文本编码库（全临界字节转义、=ybegin/=ypart/=yend 解析与 CRC32 验证），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:bwt` Pure-CP zero-alloc Burrows-Wheeler Transform, O(n) LF-Mapping & Move-To-Front codec（5/5 PASS，文档直写 `doc/lib/bwt.md`）
新增 cap:bwt 纯 CP 零分配压缩变换前端库（原地堆排序 BWT、O(n) LF-Mapping 还原与 256B MTF 管道），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:rle` Pure-CP zero-alloc RLE codec suite: PackBits (TIFF/PSD), PCX, TGA pixels & RLE0（5/5 PASS，文档直写 `doc/lib/rle.md`）
新增 cap:rle 纯 CP 零分配多标准游程编码库（PackBits/PCX/TGA/RLE0 双向编解码与溢出防护），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:lzss` Pure-CP zero-alloc LZSS (4KB window, 3..18B match, overlapping RLE) codec（5/5 PASS，文档直写 `doc/lib/lzss.md`）
新增 cap:lzss 纯 CP 零分配滑动窗口压缩库（Flag Byte 分组、3..18B 匹配与 4B Framed 封装），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:lzw` Pure-CP zero-alloc LZW (GIF LSB / TIFF MSB, 9..12-bit) compressor & decompressor（5/5 PASS，文档直写 `doc/lib/lzw.md`）
新增 cap:lzw 纯 CP 零分配字典压缩算法库（栈上 4096 字典、KwKwK 特例、自动 9..12-bit 扩展与 Clear 重置），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:minix` MINIX v1/v2/v3 superblock, 32B/64B Inode & 16B/32B/64B dir entry zero-alloc parser（5/5 PASS，文档直写 `doc/lib/minix.md`）
新增 cap:minix 纯 CP 零分配经典文件系统解析库（v1/v2/v3 五种魔数分类、统一 Inode 与 16B/32B/64B 目录块零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:lzo` lzop (.lzo) container, Adler32/CRC32 & pure-CP zero-alloc LZO1X decompressor（5/5 PASS，文档直写 `doc/lib/lzo.md`）
新增 cap:lzo 纯 CP 零分配压缩容器与解压库（9B 头部双校验、LzopBlockIter 与完整 LZO1X M1/M2/M3/M4 解压状态机），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ubifs` Linux UBI EC/VID 64B headers & UBIFS Superblock/Master/Node CRC32 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/ubifs.md`）
新增 cap:ubifs 纯 CP 零分配闪存文件系统库（UBI 大端 64B EC/VID、UBIFS 24B 通用头与无取反 CRC32 校验），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:lzh` LZH/LHA archive Level 0/1/2 headers, chained extended headers & CRC-16/ARC parser（5/5 PASS，文档直写 `doc/lib/lzh.md`）
新增 cap:lzh 纯 CP 零分配经典归档容器库（Level 0/1/2 统一解析、2B 链式扩展头迭代与 CRC-16/ARC 0xA001 校验），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:bzip2` bzip2 (.bz2) MSB-first bitstream, pi/sqrt(pi) block/EOS headers & CRC32 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/bzip2.md`）
新增 cap:bzip2 纯 CP 零分配压缩流解析库（MSB-first 0x04C11DB7 CRC32、Bzip2BitReader、48-bit 魔数块头与空流探测），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:jffs2` Linux JFFS2 flash node stream, Inode/Dirent/Cleanmarker & CRC32 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/jffs2.md`）
新增 cap:jffs2 纯 CP 零分配嵌入式闪存文件系统库（4B 对齐节点流、0xFFFF 擦除区跳过与多级 CRC32 校验），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:rar` RAR4 & RAR5 archive container, 7-bit Vint, block CRC32 & file entry zero-alloc parser（5/5 PASS，文档直写 `doc/lib/rar.md`）
新增 cap:rar 纯 CP 零分配归档容器解析库（RAR5 7-bit Vint 与完整 IEEE CRC32、RAR4 低 16 位 CRC32 迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:xz` XZ v1.1.0 (.xz) Stream Header/Footer, 7-bit VLI, Filter Chain, Index & CRC32 parser（5/5 PASS，文档直写 `doc/lib/xz.md`）
新增 cap:xz 纯 CP 零分配压缩容器解析库（VLI 1..9B 编解码、Stream/Block/Index 三重 CRC32 校验与 LZMA2 字典大小解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:cramfs` Linux CramFS 76B superblock, IEEE CRC32, 12B bitfield Inode & dir zero-alloc parser（5/5 PASS，文档直写 `doc/lib/cramfs.md`）
新增 cap:cramfs 纯 CP 零分配压缩只读文件系统库（76B 超级块 CRC32、24/6/26-bit 紧缩 Inode 解码与目录零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:romfs` Linux romfs big-endian superblock, 32-bit checksum & 16B-aligned directory tree parser（5/5 PASS，文档直写 `doc/lib/romfs.md`）
新增 cap:romfs 纯 CP 零分配极简内核只读文件系统库（大端 32-bit 校验和、16B 对齐文件头与递增防环目录迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:coff` Microsoft COFF/PE-COFF (.obj) 20B header, 40B sections, 18B symbols & 10B relocs parser（5/5 PASS，文档直写 `doc/lib/coff.md`）
新增 cap:coff 纯 CP 零分配目标文件解析库（20B 头、40B 节表含 /<offset> 长节名、18B 符号与 10B 重定位项），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:multiboot` GNU Multiboot1 & Multiboot2 kernel headers, Boot Info Tags & Mmap zero-alloc parser（5/5 PASS，文档直写 `doc/lib/multiboot.md`）
新增 cap:multiboot 纯 CP 零分配引导协议库（MB1/MB2 32-bit 环绕校验和、8B 对齐 Boot Info Tag 迭代与 Mmap/Framebuffer/Module 视图），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:bpf` eBPF (RFC 9669) 64/128-bit LD_IMM64 bytecode codec, iterator & validator（5/5 PASS，文档直写 `doc/lib/bpf.md`）
新增 cap:bpf 纯 CP 零分配内核虚拟机指令集库（8B 指令、16B LD_IMM64 自动合并、程序构造器与校验），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:btf` Linux BPF Type Format (.BTF) 19-kind type graph, bitfields & string table parser（5/5 PASS，文档直写 `doc/lib/btf.md`）
新增 cap:btf 纯 CP 零分配 eBPF 类型元数据库（24B Header、19 种 Kind 变长载荷零拷贝遍历与位域解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:fdt` Devicetree v0.4 Flattened Device Tree (.dtb) header, MemRsv & token stream zero-alloc parser（5/5 PASS，文档直写 `doc/lib/fdt.md`）
新增 cap:fdt 纯 CP 零分配嵌入式设备树解析库（40B 大端头、Memory Reservation Map、4B 对齐结构块遍历与字符串表查找），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:udf` UDF (ECMA-167/ISO 13346) VRS, 16B Tag CRC-16, AVDP/PVD & OSTA CS0 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/udf.md`）
新增 cap:udf 纯 CP 零分配光盘与通用磁盘格式库（VRS BEA01/NSR03/TEA01、16B Tag 校验和与 CRC-ITU-T 0x1021、OSTA CS0 8/16-bit 字符串解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:zfs` OpenZFS Uberblock (LE/BE), 128B blkptr_t/DVA & Fletcher-4 256-bit zero-alloc parser（5/5 PASS，文档直写 `doc/lib/zfs.md`）
新增 cap:zfs 纯 CP 零分配企业级文件系统库（自动 LE/BE 字节序识别、Uberblock txg 选主、3x DVA 偏移换算与 Fletcher-4 256-bit），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:f2fs` Linux F2FS superblock, dual Checkpoint selection & f2fs_crc32 zero-alloc parser（5/5 PASS，文档直写 `doc/lib/f2fs.md`）
新增 cap:f2fs 纯 CP 零分配闪存文件系统库（超级块 UTF-16LE 卷名与 FEAT_SB_CHKSUM CRC32、双 Checkpoint 选主），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:tpm` TCG TPM 2.0 command/response header, TPMS_ATTEST/Quote, PCR Selection & TPMT_PUBLIC codec（5/5 PASS，文档直写 `doc/lib/tpm.md`）
新增 cap:tpm 纯 CP 零分配硬件可信计算库（10B 命令头编解码、0xFF544347 远程证明、PCR 位图迭代与公钥属性解析），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:apfs` Apple APFS Fletcher-64 checksum, Object Header, Container (NXSB) & Volume (APSB) parser（5/5 PASS，文档直写 `doc/lib/apfs.md`）
新增 cap:apfs 纯 CP 零分配 macOS/iOS 文件系统库（Fletcher-64 校验和、32B Object Header、4KB NXSB/APSB 超级块解析），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:erofs` Linux EROFS 128B superblock, Compact/Extended Inode, Dirent & CRC32C zero-alloc parser（5/5 PASS，文档直写 `doc/lib/erofs.md`）
新增 cap:erofs 纯 CP 零分配只读文件系统库（128B 超级块 CRC32C、统一 Inode 视图与目录块零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:luks` Linux LUKS1 (592B/8 keyslots) & LUKS2 (4096B dual header & JSON area) zero-alloc parser（5/5 PASS，文档直写 `doc/lib/luks.md`）
新增 cap:luks 纯 CP 零分配磁盘加密元数据库（LUKS1 密钥槽统计、LUKS2 主备头 seqid 选主与零拷贝 JSON 提取），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:vhd` Microsoft VHD 1.0 Footer, Dynamic Header, BAT lookup & one's-complement checksum parser（5/5 PASS，文档直写 `doc/lib/vhd.md`）
新增 cap:vhd 纯 CP 零分配虚拟硬盘镜像库（512B Footer、1024B Dynamic Header、一补码校验和与大端 BAT 查表），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:xfs` Linux XFS v4/v5 superblock, CRC32C, Inode Core & 128-bit BMBT Extent zero-alloc codec（5/5 PASS，文档直写 `doc/lib/xfs.md`）
新增 cap:xfs 纯 CP 零分配高性能日志文件系统库（v4/v5 大端超级块 CRC32C、v2/v3 Inode Core 与 128-bit BMBT Extent 跨 64-bit 位域编解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:exfat` Microsoft exFAT boot sector, 11-sector Boot Checksum & 32B directory entry set parser（5/5 PASS，文档直写 `doc/lib/exfat.md`）
新增 cap:exfat 纯 CP 零分配大闪存文件系统库（512B 引导扇区、右旋 Boot/EntrySet Checksum 与 32B 目录项零拷贝迭代），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:vdi` VirtualBox VDI 1.1 virtual disk header & Block Allocation Map zero-alloc parser（5/5 PASS，文档直写 `doc/lib/vdi.md`）
新增 cap:vdi 纯 CP 零分配虚拟磁盘镜像库（512B VDI 1.1 报头、Comment/UUID 与 >4GiB 块映射查表），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:btrfs` Linux Btrfs 4KB superblock, CRC32C, B-Tree 101B header & sys_chunk_array zero-alloc parser（5/5 PASS，文档直写 `doc/lib/btrfs.md`）
新增 cap:btrfs 纯 CP 零分配现代写时复制文件系统库（4KB 超级块 CRC32C、sys_chunk_array 多 Stripe 零拷贝迭代与 101B B-Tree 头），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:vmdk` VMware VMDK SparseExtentHeader 512B binary & text Descriptor/Extent zero-alloc parser（5/5 PASS，文档直写 `doc/lib/vmdk.md`）
新增 cap:vmdk 纯 CP 零分配虚拟磁盘镜像库（KDMV 稀疏头换行检测、streamOptimized 标志、文本 Descriptor DDB 与 Extent 零拷贝迭代），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:vhdx` Microsoft VHDX 1.0 FileId, dual Header, Region Table, BAT & CRC32C zero-alloc parser（5/5 PASS，文档直写 `doc/lib/vhdx.md`）
新增 cap:vhdx 纯 CP 零分配虚拟硬盘镜像库（Castagnoli CRC32C、4KB Header 选主、64KB Region Table GUID 查找与 64-bit BAT 条目解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:qcow2` QEMU QCOW2 v2/v3 virtual disk header, Header Extensions & L1/L2 entry zero-alloc parser（5/5 PASS，文档直写 `doc/lib/qcow2.md`）
新增 cap:qcow2 纯 CP 零分配虚拟磁盘镜像库（72B/104B 报头、Backing File 切片、8B 对齐扩展头迭代与 64-bit L1/L2 簇表解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:scsi` SCSI (SPC-5/SBC-4) CDB codec, INQUIRY, READ CAPACITY(10/16) & Sense Data zero-alloc parser（5/5 PASS，文档直写 `doc/lib/scsi.md`）
新增 cap:scsi 纯 CP 零分配存储协议库（6/10/12/16B CDB 查表与编码、36B INQUIRY、128-bit 容量计算与双格式 Sense Data），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ntfs` NTFS VBR, MFT FILE record, Resident/Non-Resident attributes & Data Runlist parser（5/5 PASS，文档直写 `doc/lib/ntfs.md`）
新增 cap:ntfs 纯 CP 零分配 Windows 文件系统库（VBR 负 MFT 簇数换算、常驻/非常驻属性与变长有符号增量 LCN Runlist 迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:squashfs` SquashFS v4.0 96B superblock, Metadata/Data block header, Fragment & InodeRef parser（5/5 PASS，文档直写 `doc/lib/squashfs.md`）
新增 cap:squashfs 纯 CP 零分配只读压缩文件系统库（96B 超级块校验、48/13-bit InodeRef 编解码、Metadata/Data/Fragment 解析），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ext4` Linux ext2/ext3/ext4 superblock, 64-bit Inode, Extent tree & dir zero-alloc parser（5/5 PASS，文档直写 `doc/lib/ext4.md`）
新增 cap:ext4 纯 CP 零分配 Linux 文件系统库（1024B 超级块、64-bit 块/文件大小、48-bit 物理块 Extent 迭代与线性目录空洞跳过），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:fat` FAT12/16/32 BPB, 32B directory entry, 8.3 name, DOS time & LFN checksum zero-alloc parser（5/5 PASS，文档直写 `doc/lib/fat.md`）
新增 cap:fat 纯 CP 零分配文件系统解析库（FAT12/16/32 精确簇数分类、clusterToLba、8.3 文件名格式化与 LFN 校验和），通过 cpc check 与 5/5 cap test

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

