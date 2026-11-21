# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:odt` 零分配 OASIS OpenDocument (.odt/.ods/.odp) 元数据与文本提取器（5/5 PASS，自动动态挂载 `cap.odt`）
新增 `lib/odt/{odt,index}.cp`、`doc/lib/odt.md` 与 `cap/test/test_odt_production_deep.cp`（5/5 PASS），支持 OCF mimetype 校验、meta.xml 统计、manifest.xml 与 content.xml 文本提取。

## [2026-11-22] - 新建 `cap:docx` 零分配 OOXML DOCX WordprocessingML 段落文本提取与 CoreProps 解析器（5/5 PASS，自动动态挂载 `cap.docx`）
新增 `lib/docx/{docx,index}.cp`、`doc/lib/docx.md` 与 `cap/test/test_docx_production_deep.cp`（6/6 PASS），支持 Heading/段落迭代、实体与数字实体 UTF-8 解码、tab/br 换行及 core.xml 元数据解析。

## [2026-11-22] - 新建 `cap:epub` 零分配 EPUB 2/3 电子书 OCF 校验、container.xml 与 OPF 元数据/书脊解析器（5/5 PASS，自动动态挂载 `cap.epub`）
新增 `lib/epub/{epub,index}.cp`、`doc/lib/epub.md` 与 `cap/test/test_epub_production_deep.cp`（5/5 PASS），支持未压缩 mimetype OCF 校验、container.xml 与 OPF metadata/manifest/spine 解析。

## [2026-11-22] - 新建 `cap:topojson` 零分配 TopoJSON v1.0 量化差分 Arc 解码与拓扑对象解析器（5/5 PASS，自动动态挂载 `cap.topojson`）
新增 `lib/topojson/{topojson,index}.cp`、`doc/lib/topojson.md` 与 `cap/test/test_topojson_production_deep.cp`（5/5 PASS），支持 transform 仿射量化还原、差分 Arc 迭代、负索引反向引用与命名 Objects 迭代。

## [2026-11-22] - 新建 `cap:mobi` 零分配 Mobipocket / Kindle (.mobi/.azw) PalmDB/MOBI/EXTH 解析器（5/5 PASS，自动动态挂载 `cap.mobi`）
新增 `lib/mobi/{mobi,index}.cp`、`doc/lib/mobi.md` 与 `cap/test/test_mobi_production_deep.cp`（5/5 PASS），支持 78B PalmDB 记录切片、PalmDOC/MOBI 头与 EXTH 元数据迭代。

## [2026-11-22] - 新建 `cap:gpx` 零分配 GPX 1.0/1.1 航点/轨迹 XML 解析与 Haversine 距离统计器（5/5 PASS，自动动态挂载 `cap.gpx`）
新增 `lib/gpx/{gpx,index}.cp`、`doc/lib/gpx.md` 与 `cap/test/test_gpx_production_deep.cp`（5/5 PASS），支持航点/轨迹点迭代、WGS-84 Haversine 距离、跨段隔离累计与爬升/下降统计。

## [2026-11-22] - 新建 `cap:kml` 零分配 OGC KML 2.2 地理标记 Placemark 与 coordinates 解析器（5/5 PASS，自动动态挂载 `cap.kml`）
新增 `lib/kml/{kml,index}.cp`、`doc/lib/kml.md` 与 `cap/test/test_kml_production_deep.cp`（5/5 PASS），支持 BOM/注释/CDATA 剥离、lon,lat[,alt] 坐标迭代与 BBox 单遍统计。

## [2026-11-22] - 新建 `cap:rtf` 零分配 Rich Text Format (RTF 1.9) 词法流、Unicode 转义与纯文本提取器（5/5 PASS，自动动态挂载 `cap.rtf`）
新增 `lib/rtf/{rtf,index}.cp`、`doc/lib/rtf.md` 与 `cap/test/test_rtf_production_deep.cp`（5/5 PASS），支持 64 层组栈状态机、忽略目的地跳过、\uN UTF-8 解码与零堆分配纯文本提取。

## [2026-11-22] - 新建 `cap:xcf` 零分配 GIMP XCF v000-v014 分层图像头、属性表与图层偏移解析器（5/5 PASS，自动动态挂载 `cap.xcf`）
新增 `lib/xcf/{xcf,index}.cp`、`doc/lib/xcf.md` 与 `cap/test/test_xcf_production_deep.cp`（5/5 PASS），支持 v0-v14 版本头、v4+ 精度、v11+ 64-bit 偏移表与图层名提取。

## [2026-11-22] - 新建 `cap:psd` 零分配 Adobe Photoshop (.psd/.psb) 文件头、区段与 8BIM 资源块解析器（5/5 PASS，自动动态挂载 `cap.psd`）
新增 `lib/psd/{psd,index}.cp`、`doc/lib/psd.md` 与 `cap/test/test_psd_production_deep.cp`（5/5 PASS），支持 PSD 32-bit 与 PSB 64-bit Layer/Mask 区段、8BIM 2B 对齐资源块迭代与按 ID 查找。

## [2026-11-22] - 新建 `cap:las` 零分配 ASPRS LAS 1.2-1.4 LiDAR 3D 点云头、VLR 与点记录解析器（5/5 PASS，自动动态挂载 `cap.las`）
新增 `lib/las/{las,index}.cp`、`doc/lib/las.md` 与 `cap/test/test_las_production_deep.cp`（5/5 PASS），支持 LAS 1.2/1.3/1.4 头解析、LAZ 压缩标志识别、VLR 迭代与 Format 0-10 点坐标缩放换算。

## [2026-11-22] - 新建 `cap:shp` 零分配 ESRI Shapefile (.shp) 混合端序头、Bounding Box 与几何记录解析器（5/5 PASS，自动动态挂载 `cap.shp`）
新增 `lib/shp/{shp,index}.cp`、`doc/lib/shp.md` 与 `cap/test/test_shp_production_deep.cp`（5/5 PASS），支持 BE/LE 混合 100B 头、16-bit word 长度校验、Point/PolyLine/Polygon 零拷贝随机访问。

## [2026-11-22] - 新建 `cap:wkb` 零分配 OGC WKB / PostGIS EWKB 空间几何二进制编解码器（5/5 PASS，自动动态挂载 `cap.wkb`）
新增 `lib/wkb/{wkb,index}.cp`、`doc/lib/wkb.md` 与 `cap/test/test_wkb_production_deep.cp`（5/5 PASS），支持 XDR/NDR 双端序、ISO WKB Z/M/ZM 与 PostGIS EWKB SRID 编解码及多级嵌套迭代。

## [2026-11-22] - 新建 `cap:lucene` 零分配 Apache Lucene CodecUtil 索引头/尾帧与 VInt/VLong 解析器（5/5 PASS，自动动态挂载 `cap.lucene`）
新增 `lib/lucene/{lucene,index}.cp`、`doc/lib/lucene.md` 与 `cap/test/test_lucene_production_deep.cp`（5/5 PASS），支持 CodecHeader/IndexHeader、16B Footer CRC32 校验与 VInt/VLong/ZigZag 解码。

## [2026-11-22] - 新建 `cap:flatbuf` 零分配 Google FlatBuffers 二进制根表、VTable 字段与向量解析器（5/5 PASS，自动动态挂载 `cap.flatbuf`）
新增 `lib/flatbuf/{flatbuf,index}.cp`、`doc/lib/flatbuf.md` 与 `cap/test/test_flatbuf_production_deep.cp`（5/5 PASS），支持 File Identifier、尺寸前缀剥离、正负 soffset VTable 定位与全标量/字符串/嵌套表/向量读取。

## [2026-11-22] - 新建 `cap:marshal` 零分配 Ruby Marshal 4.8 二进制对象与 Fixnum 压缩整数解析器（5/5 PASS，自动动态挂载 `cap.marshal`）
新增 `lib/marshal/{marshal,index}.cp`、`doc/lib/marshal.md` 与 `cap/test/test_marshal_production_deep.cp`（5/5 PASS），支持 Marshal 4.8 版本头、压缩有符号整数、IVAR UTF-8 解包与深度受控 skipValue。

## [2026-11-22] - 新建 `cap:smile` 零分配 FasterXML Jackson Smile 1.0 二进制 JSON 编解码与 Token 流解析器（5/5 PASS，自动动态挂载 `cap.smile`）
新增 `lib/smile/{smile,index}.cp`、`doc/lib/smile.md` 与 `cap/test/test_smile_production_deep.cp`（5/5 PASS），支持 4B Smile 头、7+6-bit VInt/ZigZag、Tiny/Small/Long 字符串、共享引用与对象键解析。

## [2026-11-22] - 新建 `cap:ion` 零分配 Amazon Ion 1.0 二进制格式 IVM、VarUInt/VarInt 与结构化值解析器（5/5 PASS，自动动态挂载 `cap.ion`）
新增 `lib/ion/{ion,index}.cp`、`doc/lib/ion.md` 与 `cap/test/test_ion_production_deep.cp`（5/5 PASS），支持 IVM 1.0、大端 7-bit VarUInt/VarInt、NOP Pad 跳过、List 与 Struct 迭代。

## [2026-11-22] - 新建 `cap:pickle` 零分配 Python Pickle Protocol 2-5 指令流解析与静态安全审计器（5/5 PASS，自动动态挂载 `cap.pickle`）
新增 `lib/pickle/{pickle,index}.cp`、`doc/lib/pickle.md` 与 `cap/test/test_pickle_production_deep.cp`（5/5 PASS），支持 Protocol 2-5 全部核心指令解码与 GLOBAL/REDUCE/NEWOBJ 等危险操作码静态扫描。

## [2026-11-22] - 新建 `cap:jfr` 零分配 JVM Java Flight Recorder v2.0 (.jfr) Chunk 头与事件帧解析器（5/5 PASS，自动动态挂载 `cap.jfr`）
新增 `lib/jfr/{jfr,index}.cp`、`doc/lib/jfr.md` 与 `cap/test/test_jfr_production_deep.cp`（5/5 PASS），支持 68B Chunk 头、多 Chunk 迭代、JFR 9B 压缩整数与事件帧流解析。

## [2026-11-22] - 新建 `cap:flexbuf` 零分配 Google FlatBuffers FlexBuffers 无 Schema 零拷贝二进制解析器（5/5 PASS，自动动态挂载 `cap.flexbuf`）
新增 `lib/flexbuf/{flexbuf,index}.cp`、`doc/lib/flexbuf.md` 与 `cap/test/test_flexbuf_production_deep.cp`（5/5 PASS），支持全 28 种 FlexType、间接标量、定型/多态向量与有序 Map 二分查找。

## [2026-11-22] - 新建 `cap:bincode` 零分配 Rust bincode v1 (FixedInt) / v2 (Varint + ZigZag) 二进制编解码器（5/5 PASS，自动动态挂载 `cap.bincode`）
新增 `lib/bincode/{bincode,index}.cp`、`doc/lib/bincode.md` 与 `cap/test/test_bincode_production_deep.cp`（5/5 PASS），支持 Legacy 定宽 LE/BE 与 Standard Varint/ZigZag 双向编解码。

## [2026-11-22] - 新建 `cap:hprof` 零分配 JVM HPROF 1.0.1/1.0.2 二进制堆转储头与记录流解析器（5/5 PASS，自动动态挂载 `cap.hprof`）
新增 `lib/hprof/{hprof,index}.cp`、`doc/lib/hprof.md` 与 `cap/test/test_hprof_production_deep.cp`（5/5 PASS），支持 32/64-bit id_size、UTF8/LOAD_CLASS/HEAP_SUMMARY 解析与单遍统计。

## [2026-11-22] - 新建 `cap:aof` 零分配 Redis AOF 命令日志、#TS: 时间戳与混合 RDB 前导解析器（5/5 PASS，自动动态挂载 `cap.aof`）
新增 `lib/aof/{aof,index}.cp`、`doc/lib/aof.md` 与 `cap/test/test_aof_production_deep.cp`（5/5 PASS），支持二进制安全 RESP Multi-Bulk 流迭代、Redis 7.0 #TS: 时间戳关联与截断尾安全 consumedBytes。

## [2026-11-22] - 新建 `cap:ubjson` 零分配 Universal Binary JSON (UBJSON Draft 12) 编解码器（5/5 PASS，自动动态挂载 `cap.ubjson`）
新增 `lib/ubjson/{ubjson,index}.cp`、`doc/lib/ubjson.md` 与 `cap/test/test_ubjson_production_deep.cp`（5/5 PASS），支持全套 Draft 12 标记、No-Op 跳过、$type#count 优化容器与深度防护 skipValue。

## [2026-11-22] - 新建 `cap:vcd` 零分配 IEEE 1364 VCD 硬件/RTL 仿真波形头与值变流解析器（5/5 PASS，自动动态挂载 `cap.vcd`）
新增 `lib/vcd/{vcd,index}.cp`、`doc/lib/vcd.md` 与 `cap/test/test_vcd_production_deep.cp`（5/5 PASS），支持 $timescale/$scope/$var 头解析及时间戳/标量/向量/实数波形流迭代。

## [2026-11-22] - 新建 `cap:lcov` 零分配 LCOV (.info) 代码覆盖率报告流式解析与聚合统计器（5/5 PASS，自动动态挂载 `cap.lcov`）
新增 `lib/lcov/{lcov,index}.cp`、`doc/lib/lcov.md` 与 `cap/test/test_lcov_production_deep.cp`（5/5 PASS），支持 14 种 LCOV 记录、BRDA 未执行分支、缺失 LF/LH 自动回退实算与全局覆盖率汇总。

## [2026-11-22] - 新建 `cap:postcard` 零分配 Postcard 嵌入式紧凑二进制（LEB128/ZigZag + COBS）编解码器（5/5 PASS，自动动态挂载 `cap.postcard`）
新增 `lib/postcard/{postcard,index}.cp`、`doc/lib/postcard.md` 与 `cap/test/test_postcard_production_deep.cp`（5/5 PASS），支持全套 LEB128/ZigZag 变长整数、IEEE-754 浮点与 COBS 帧定界编解码。

## [2026-11-22] - 新建 `cap:rdb` 零分配 Redis RDB 二进制快照头、变长/整数编码与流式事件解析器（5/5 PASS，自动动态挂载 `cap.rdb`）
新增 `lib/rdb/{rdb,index}.cp`、`doc/lib/rdb.md` 与 `cap/test/test_rdb_production_deep.cp`（5/5 PASS），支持 REDIS 9B 头、4 种长度编码、int8/16/32/LZF 字符串与全套 Opcode 流式解析。

## [2026-11-22] - 新建 `cap:sstable` 零分配 LevelDB / RocksDB SSTable Footer、BlockHandle 与前缀压缩块解析器（5/5 PASS，自动动态挂载 `cap.sstable`）
新增 `lib/sstable/{sstable,index}.cp`、`doc/lib/sstable.md` 与 `cap/test/test_sstable_production_deep.cp`（5/5 PASS），支持 48B Footer、Varint64 BlockHandle、CRC32C mask/unmask 与 Restart Point 前缀压缩块迭代。

## [2026-11-22] - 新建 `cap:ssz` 零分配 Ethereum SSZ 确定性小端编解码、Bitlist/Bitvector 与 Offset 校验器（5/5 PASS，自动动态挂载 `cap.ssz`）
新增 `lib/ssz/{ssz,index}.cp`、`doc/lib/ssz.md` 与 `cap/test/test_ssz_production_deep.cp`（5/5 PASS），支持 u8-u128/bool 小端编解码、Bitlist 哨兵位/Bitvector 高零校验与变长列表 Offset 零拷贝迭代。

## [2026-11-22] - 新建 `cap:netcdf` 零分配 Unidata NetCDF CDF-1/CDF-2/CDF-5 科学多维数组容器解析器（5/5 PASS，自动动态挂载 `cap.netcdf`）
新增 `lib/netcdf/{netcdf,index}.cp`、`doc/lib/netcdf.md` 与 `cap/test/test_netcdf_production_deep.cp`（5/5 PASS），支持 CDF-1/2/5 4B 对齐头解析与维度/全局属性/变量零分配迭代。

## [2026-11-22] - 新建 `cap:scale` 零分配 Parity SCALE 确定性小端编解码器与 Compact<u64> 规范校验（5/5 PASS，自动动态挂载 `cap.scale`）
新增 `lib/scale/{scale,index}.cp`、`doc/lib/scale.md` 与 `cap/test/test_scale_production_deep.cp`（5/5 PASS），支持 Compact<u64> 全 4 模式规范校验、单字节三态 Option<bool> 与全套 LE 标量/容器编解码。

## [2026-11-22] - 新建 `cap:borsh` 零分配 Borsh 确定性二进制小端编解码器（拒绝 NaN 与非 0/1 布尔）（5/5 PASS，自动动态挂载 `cap.borsh`）
新增 `lib/borsh/{borsh,index}.cp`、`doc/lib/borsh.md` 与 `cap/test/test_borsh_production_deep.cp`（5/5 PASS），支持全套小端整数/浮点、Option/Enum/Seq 与原子溢出保护。

## [2026-11-22] - 新建 `cap:rlp` 零分配 Ethereum RLP 确定性二进制规范编解码器（5/5 PASS，自动动态挂载 `cap.rlp`）
新增 `lib/rlp/{rlp,index}.cp`、`doc/lib/rlp.md` 与 `cap/test/test_rlp_production_deep.cp`（5/5 PASS），支持严格规范 RLP 解码（拒绝非规范单字节/前导零/短长前缀）、列表迭代与零分配编码。

## [2026-11-22] - 新建 `cap:nbt` 零分配 Named Binary Tag (NBT) Java BE / Bedrock LE 解析器（5/5 PASS，自动动态挂载 `cap.nbt`）
新增 `lib/nbt/{nbt,index}.cp`、`doc/lib/nbt.md` 与 `cap/test/test_nbt_production_deep.cp`（5/5 PASS），支持双端序流式读取、64 层深度防护 skipPayload 与根 Compound 字段零拷贝定位。

## [2026-11-22] - 新建 `cap:hdf5` 零分配 HDF5 科学数据容器 Superblock v0-v3 与 OHDR v2 对象头解析器（5/5 PASS，自动动态挂载 `cap.hdf5`）
新增 `lib/hdf5/{hdf5,index}.cp`、`doc/lib/hdf5.md` 与 `cap/test/test_hdf5_production_deep.cp`（5/5 PASS），支持 2^n*512 User Block 跳搜、Superblock v0/v1/v2/v3 与 OHDR v2 对象头零拷贝解析。

## [2026-11-22] - 新建 `cap:deb` 零分配 Debian (.deb) 软件包容器与 RFC 822 DEBIAN/control 解析器（5/5 PASS，自动动态挂载 `cap.deb`）
新增 `lib/deb/{deb,index}.cp`、`doc/lib/deb.md` 与 `cap/test/test_deb_production_deep.cp`（5/5 PASS），支持 debian-binary/control.tar.*/data.tar.* 零拷贝提取与大小写不敏感 control 元数据解析。

## [2026-11-22] - 新建 `cap:mkv` 零分配 Matroska / WebM EBML 视频容器 Info 与多轨道解析器（5/5 PASS，自动动态挂载 `cap.mkv`）
新增 `lib/mkv/{mkv,index}.cp`、`doc/lib/mkv.md` 与 `cap/test/test_mkv_production_deep.cp`（5/5 PASS），支持 Matroska/WebM DocType、未知尺寸流式 Segment、32/64-bit 浮点 Duration 与多轨道迭代。

## [2026-11-22] - 新建 `cap:dmg` 零分配 Apple UDIF (.dmg) 512B koly 尾部块与 mish/BLKX 条带解析器（5/5 PASS，自动动态挂载 `cap.dmg`）
新增 `lib/dmg/{dmg,index}.cp`、`doc/lib/dmg.md` 与 `cap/test/test_dmg_production_deep.cp`（6/6 PASS），支持 512B koly 尾部校验、xmlSlice/dataForkSlice 零拷贝提取与 204B mish/40B BLKX 条带迭代。

## [2026-11-22] - 新建 `cap:ts` 零分配 ISO/IEC 13818-1 MPEG-TS 188B/M2TS 传输流、PCR、PAT 与 PMT 解析器（5/5 PASS，自动动态挂载 `cap.ts`）
新增 `lib/ts/{ts,index}.cp`、`doc/lib/ts.md` 与 `cap/test/test_ts_production_deep.cp`（5/5 PASS），支持 188B TS/192B M2TS 同步、Adaptation Field 33-bit PCR 解码与 PAT/PMT 表迭代。

## [2026-11-22] - 新建 `cap:heif` 零分配 HEIF / HEIC ISO BMFF 图像容器与 hvcC/ispe/colr 解析器（5/5 PASS，自动动态挂载 `cap.heif`）
新增 `lib/heif/{heif,index}.cp`、`doc/lib/heif.md` 与 `cap/test/test_heif_production_deep.cp`（5/5 PASS），支持 heic/heix/mif1/msf1 品牌识别、23B+ hvcC HEVC 配置解析与 Alpha/Depth 辅助通道检测。

## [2026-11-22] - 新建 `cap:cab` 零分配 Microsoft Cabinet (.cab) CFHEADER/CFFOLDER/CFFILE 归档解析器（5/5 PASS，自动动态挂载 `cap.cab`）
新增 `lib/cab/{cab,index}.cp`、`doc/lib/cab.md` 与 `cap/test/test_cab_production_deep.cp`（6/6 PASS），支持 MSZIP/LZX/Quantum 压缩识别、RESERVE/PREV/NEXT 扩展偏移与文件夹/文件零拷贝迭代。

## [2026-11-22] - 新建 `cap:mpeg` 零分配 MPEG-1/2/2.5 MP3 帧头、ID3v2 跳过与 Xing/Info/VBRI VBR 解析器（5/5 PASS，自动动态挂载 `cap.mpeg`）
新增 `lib/mpeg/{mpeg,index}.cp`、`doc/lib/mpeg.md` 与 `cap/test/test_mpeg_production_deep.cp`（5/5 PASS），支持全版本/层级查表、精确帧字节计算、ID3v2 跳过与 Xing/Info/VBRI 时长汇总。

## [2026-11-22] - 新建 `cap:avif` 零分配 AVIF / MIAF ISO BMFF 图像容器与 av1C/ispe/colr 元数据解析器（5/5 PASS，自动动态挂载 `cap.avif`）
新增 `lib/avif/{avif,index}.cp`、`doc/lib/avif.md` 与 `cap/test/test_avif_production_deep.cp`（5/5 PASS），支持 avif/avis 品牌识别、av1C 8/10/12-bit 与子采样解析、ispe/pixi/colr/irot/auxC 提取。

## [2026-11-22] - 新建 `cap:iso` 零分配 ISO 9660 光盘镜像 PVD 卷描述符与目录记录解析器（5/5 PASS，自动动态挂载 `cap.iso`）
新增 `lib/iso/{iso,index}.cp`、`doc/lib/iso.md` 与 `cap/test/test_iso_production_deep.cp`（6/6 PASS），支持 16 扇区 PVD 校验、both-endian 一致性检查、跨扇区目录遍历与文件零拷贝切片。

## [2026-11-22] - 新建 `cap:opus` 零分配 RFC 7845 OpusHead/OpusTags 与 RFC 6716 Opus TOC 解析器（5/5 PASS，自动动态挂载 `cap.opus`）
新增 `lib/opus/{opus,index}.cp`、`doc/lib/opus.md` 与 `cap/test/test_opus_production_deep.cp`（5/5 PASS），支持单/多声道映射 OpusHead、OpusTags 注释迭代与全 0..31 config TOC 模式/带宽/时长解码。

## [2026-11-22] - 新建 `cap:aac` 零分配 MPEG-2/4 AAC ADTS 音频帧流与 AudioSpecificConfig 解析器（5/5 PASS，自动动态挂载 `cap.aac`）
新增 `lib/aac/{aac,index}.cp`、`doc/lib/aac.md` 与 `cap/test/test_aac_production_deep.cp`（5/5 PASS），支持 7B/9B ADTS 帧流迭代、13 档采样率/声道查表、AudioSpecificConfig 扩展解析与时长估算。

