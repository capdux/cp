## 2026-11-22: 新建 `cap:marshal` 零分配 Ruby Marshal 4.8 二进制对象与 Fixnum 压缩整数解析器（5/5 PASS，文档直写 `doc/lib/marshal.md`）
实现零分配 Ruby Marshal 4.8 解析器，完整支持裸字符串/IVAR 字符串、Symbol/Symlink 与嵌套容器。

## 2026-11-22: 新建 `cap:smile` 零分配 FasterXML Jackson Smile 1.0 二进制 JSON 编解码与 Token 流解析器（5/5 PASS，文档直写 `doc/lib/smile.md`）
实现零分配 Jackson Smile 1.0 流式 Token 解析器，覆盖 5B/10B 7-bit 浮点编码与完整对象键状态机。

## 2026-11-22: 新建 `cap:ion` 零分配 Amazon Ion 1.0 二进制格式 IVM、VarUInt/VarInt 与结构化值解析器（5/5 PASS，文档直写 `doc/lib/ion.md`）
实现零分配 Amazon Ion 1.0 二进制解析器，支持全部 16 种 Type Code、typed null、内联 bool 与扩展长度编码。

## 2026-11-22: 新建 `cap:pickle` 零分配 Python Pickle Protocol 2-5 指令流解析与静态安全审计器（5/5 PASS，文档直写 `doc/lib/pickle.md`）
实现零分配 Python Pickle 指令流解析与模型权重安全审计器，覆盖 FRAME/LONG1/BINUNICODE8/BINFLOAT 与防越界校验。

## 2026-11-22: 新建 `cap:jfr` 零分配 JVM Java Flight Recorder v2.0 (.jfr) Chunk 头与事件帧解析器（5/5 PASS，文档直写 `doc/lib/jfr.md`）
实现零分配 JFR v2.0 录制文件解析器，支持 Metadata/Checkpoint/Custom 事件扫描与总时长统计。

## 2026-11-22: 新建 `cap:flexbuf` 零分配 Google FlatBuffers FlexBuffers 无 Schema 零拷贝二进制解析器（5/5 PASS，文档直写 `doc/lib/flexbuf.md`）
实现零分配 FlexBuffers 解析器，完整支持 1/2/4/8B 多态位宽与相对偏移安全解引用。

## 2026-11-22: 新建 `cap:bincode` 零分配 Rust bincode v1 (FixedInt) / v2 (Varint + ZigZag) 二进制编解码器（5/5 PASS，文档直写 `doc/lib/bincode.md`）
实现零分配 Rust bincode v1/v2 编解码器，严格校验非法 varint 标记与目标整数位宽上界。

## 2026-11-22: 新建 `cap:hprof` 零分配 JVM HPROF 1.0.1/1.0.2 二进制堆转储头与记录流解析器（5/5 PASS，文档直写 `doc/lib/hprof.md`）
实现零分配 JVM HPROF 堆转储解析器，覆盖全部 14 个顶层 Record Tag 与 10 个 Heap Dump 子记录常量。

## 2026-11-22: 新建 `cap:aof` 零分配 Redis AOF 命令日志、#TS: 时间戳与混合 RDB 前导解析器（5/5 PASS，文档直写 `doc/lib/aof.md`）
实现零分配 Redis AOF 流式解析器，支持混合 RDB 前导探测与断点续读/截断尾部恢复。

## 2026-11-22: 新建 `cap:ubjson` 零分配 Universal Binary JSON (UBJSON Draft 12) 编解码器（5/5 PASS，文档直写 `doc/lib/ubjson.md`）
实现零分配 UBJSON 编解码器，支持 writeCompactInt 自动最小编码与定宽强类型数组 O(1) 跳过。

## 2026-11-22: 新建 `cap:vcd` 零分配 IEEE 1364 VCD 硬件/RTL 仿真波形头与值变流解析器（5/5 PASS，文档直写 `doc/lib/vcd.md`）
实现零分配 IEEE 1364 VCD 波形解析器，支持 1/10/100 全部 6 种时间单位与 Verilator/Icarus 仿真流。

## 2026-11-22: 新建 `cap:lcov` 零分配 LCOV (.info) 代码覆盖率报告流式解析与聚合统计器（5/5 PASS，文档直写 `doc/lib/lcov.md`）
实现零分配 LCOV 覆盖率解析器，兼容 CRLF 与含逗号函数名，提供按文件与全局统计视图。

## 2026-11-22: 新建 `cap:postcard` 零分配 Postcard 嵌入式紧凑二进制（LEB128/ZigZag + COBS）编解码器（5/5 PASS，文档直写 `doc/lib/postcard.md`）
实现零分配 Postcard 编解码器，严格校验 VarU16/32/64 位宽上限与 COBS 非法零字节。

## 2026-11-22: 新建 `cap:rdb` 零分配 Redis RDB 二进制快照头、变长/整数编码与流式事件解析器（5/5 PASS，文档直写 `doc/lib/rdb.md`）
实现零分配 Redis RDB 解析器，自动关联 EXPIRETIME/EXPIRETIME_MS 至后续 KV 并支持 EOF 8B CRC64 校验尾。

## 2026-11-22: 新建 `cap:sstable` 零分配 LevelDB / RocksDB SSTable Footer、BlockHandle 与前缀压缩块解析器（5/5 PASS，文档直写 `doc/lib/sstable.md`）
实现零分配 LevelDB/RocksDB SSTable 解析器，严格校验 handle 偏移与 restarts_offset 边界。

## 2026-11-22: 新建 `cap:ssz` 零分配 Ethereum SSZ 确定性小端编解码、Bitlist/Bitvector 与 Offset 校验器（5/5 PASS，文档直写 `doc/lib/ssz.md`）
实现零分配 Ethereum SSZ 编解码器，严格校验非递减 Offset 表、哨兵比特与 32B Chunk 叶子数计算。

## 2026-11-22: 新建 `cap:netcdf` 零分配 Unidata NetCDF CDF-1/CDF-2/CDF-5 科学多维数组容器解析器（5/5 PASS，文档直写 `doc/lib/netcdf.md`）
实现零分配 NetCDF 容器解析器，兼容 32-bit 与 64-bit 偏移/长度编码及 UNLIMITED 维度。

## 2026-11-22: 新建 `cap:scale` 零分配 Parity SCALE 确定性小端编解码器与 Compact<u64> 规范校验（5/5 PASS，文档直写 `doc/lib/scale.md`）
实现零分配 Parity SCALE 编解码器，严格拒绝非最小 Compact 编码与大整数尾零。

## 2026-11-22: 新建 `cap:borsh` 零分配 Borsh 确定性二进制小端编解码器（拒绝 NaN 与非 0/1 布尔）（5/5 PASS，文档直写 `doc/lib/borsh.md`）
实现零分配 Borsh 编解码器，严格执行 NaN 拒绝与 0/1 布尔合法性检查。

## 2026-11-22: 新建 `cap:rlp` 零分配 Ethereum RLP 确定性二进制规范编解码器（5/5 PASS，文档直写 `doc/lib/rlp.md`）
实现零分配 Ethereum RLP 编解码器，严格遵循 Yellow Paper 规范性规则与原子缓冲区溢出保护。

## 2026-11-22: 新建 `cap:nbt` 零分配 Named Binary Tag (NBT) Java BE / Bedrock LE 解析器（5/5 PASS，文档直写 `doc/lib/nbt.md`）
实现零分配 NBT 二进制格式解析器，完整覆盖 13 种 Tag 类型与数组乘法溢出防护。

## 2026-11-22: 新建 `cap:hdf5` 零分配 HDF5 科学数据容器 Superblock v0-v3 与 OHDR v2 对象头解析器（5/5 PASS，文档直写 `doc/lib/hdf5.md`）
实现零分配 HDF5 容器头解析器，严格校验 2/4/8B 地址/长度宽度、Root Group 地址与 chunk0 边界。

## 2026-11-22: 新建 `cap:deb` 零分配 Debian (.deb) 软件包容器与 RFC 822 DEBIAN/control 解析器（5/5 PASS，文档直写 `doc/lib/deb.md`）
实现零分配 Debian .deb 容器与 RFC 822 control 解析器，严格校验 ar 60B 头、2B 偶对齐与多行续行。

## 2026-11-22: 新建 `cap:mkv` 零分配 Matroska / WebM EBML 视频容器 Info 与多轨道解析器（5/5 PASS，文档直写 `doc/lib/mkv.md`）
实现零分配 Matroska/WebM EBML 容器解析器，支持视频宽高、音频采样率/声道数与流式无界 Segment 解析。

## 2026-11-22: 新建 `cap:dmg` 零分配 Apple UDIF (.dmg) 512B koly 尾部块与 mish/BLKX 条带解析器（5/5 PASS，文档直写 `doc/lib/dmg.md`）
实现零分配 Apple UDIF DMG 解析器，严格校验 koly/mish 魔数、版本号与防溢出 sector_count 字节换算。



## 2026-11-22: 新建 `cap:ts` 零分配 ISO/IEC 13818-1 MPEG-TS 188B/M2TS 传输流、PCR、PAT 与 PMT 解析器（5/5 PASS，文档直写 `doc/lib/ts.md`）
实现零分配 MPEG-TS 传输流解析器，自动跳过 PSI pointer_field 并严格校验 section_length 与 adaptation 边界。

## 2026-11-22: 新建 `cap:heif` 零分配 HEIF / HEIC ISO BMFF 图像容器与 hvcC/ispe/colr 解析器（5/5 PASS，文档直写 `doc/lib/heif.md`）
实现零分配 HEIF/HEIC ISO BMFF 元数据解析器，自动排除纯 AVIF 品牌并解析 Main/Main10 色彩与旋转信息。

## 2026-11-22: 新建 `cap:cab` 零分配 Microsoft Cabinet (.cab) CFHEADER/CFFOLDER/CFFILE 归档解析器（5/5 PASS，文档直写 `doc/lib/cab.md`）
实现零分配 Microsoft Cabinet 归档解析器，严格校验 MSCF 魔数、1.3 版本号、ab_reserve 扩展与未闭合文件名边界。

## 2026-11-22: 新建 `cap:mpeg` 零分配 MPEG-1/2/2.5 MP3 帧头、ID3v2 跳过与 Xing/Info/VBRI VBR 解析器（5/5 PASS，文档直写 `doc/lib/mpeg.md`）
实现零分配 MPEG 音频帧与 VBR 头解析器，支持连续帧同步字校验与 Side-Info 偏移自适应。

## 2026-11-22: 新建 `cap:avif` 零分配 AVIF / MIAF ISO BMFF 图像容器与 av1C/ispe/colr 元数据解析器（5/5 PASS，文档直写 `doc/lib/avif.md`）
实现零分配 AVIF ISO BMFF 元数据解析器，支持 HDR nclx 色彩、Alpha auxC 与动画序列检测。

## 2026-11-22: 新建 `cap:iso` 零分配 ISO 9660 光盘镜像 PVD 卷描述符与目录记录解析器（5/5 PASS，文档直写 `doc/lib/iso.md`）
实现零分配 ISO 9660 光盘镜像解析器，支持 ;1 版本号剥离、扇区末尾 0 填充跳过与防溢出 LBA 读取。

## 2026-11-22: 新建 `cap:opus` 零分配 RFC 7845 OpusHead/OpusTags 与 RFC 6716 Opus TOC 解析器（5/5 PASS，文档直写 `doc/lib/opus.md`）
实现零分配 Opus 音频头与 TOC 解析器，严格校验 120ms 上限、映射表索引与 VBR/Padding 帧计数。

## 2026-11-22: 新建 `cap:aac` 零分配 MPEG-2/4 AAC ADTS 音频帧流与 AudioSpecificConfig 解析器（5/5 PASS，文档直写 `doc/lib/aac.md`）
实现零分配 AAC ADTS 与 AudioSpecificConfig 解析器，支持 5-bit escape 对象类型与 24-bit 显式采样率。

