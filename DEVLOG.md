## 2026-11-22: 新建 `cap:x3d` 零分配 Web3D X3D 3.x/4.0 XML 3D 场景树与坐标/索引数组解析器（5/5 PASS，文档直写 `doc/lib/x3d.md`）
实现零分配 ISO/IEC 19776-1 X3D XML 解析器，支持 DEF/USE 提取与 -1 面分隔符索引解码。

## 2026-11-22: 新建 `cap:it` 零分配 Impulse Tracker 2.14 (.it) 跟踪器音乐头、采样与 Pattern 解析器（5/5 PASS，文档直写 `doc/lib/it.md`）
实现零分配 Impulse Tracker IT 解析器，覆盖 4 张偏移表防越界校验与 16-bit/Loop 采样标志。

## 2026-11-22: 新建 `cap:spc` 零分配 SNES SPC700 (.spc) 芯片音乐头、ID666 与 xid6 扩展块解析器（5/5 PASS，文档直写 `doc/lib/spc.md`）
实现零分配 SNES SPC700 芯片音乐解析器，覆盖 CPU 寄存器快照与 xid6 扩展标签。

## 2026-11-22: 新建 `cap:s3m` 零分配 Scream Tracker 3 (.s3m) 跟踪器音乐头、Parapointer 与采样解析器（5/5 PASS，文档直写 `doc/lib/s3m.md`）
实现零分配 Scream Tracker 3 S3M 解析器，支持 24-bit memseg 偏移、SCRS 签名与立体声/16-bit 标志。

## 2026-11-22: 新建 `cap:xm` 零分配 FastTracker II (.xm) 跟踪器音乐头、Pattern 与 Instrument 解析器（5/5 PASS，文档直写 `doc/lib/xm.md`）
实现零分配 FastTracker II XM 解析器，完整支持 Linear/Amiga 频率表标志与严格边界防溢出。

## 2026-11-22: 新建 `cap:eot` 零分配 Microsoft Embedded OpenType (.eot) Web 字体容器与 UTF-16LE 名称解析器（5/5 PASS，文档直写 `doc/lib/eot.md`）
实现零分配 Microsoft EOT 容器解析器，严格校验 0x504C 魔数、Reserved/Padding 全零与字体边界。

## 2026-11-22: 新建 `cap:nsf` 零分配 NES Sound Format (.nsf/.nsfe) 芯片音乐头与分块容器解析器（5/5 PASS，文档直写 `doc/lib/nsf.md`）
实现零分配 NES Sound Format (.nsf/.nsfe) 解析器，支持 NTSC/PAL 帧率计算与 NSFe INFO 块解析。

## 2026-11-22: 新建 `cap:abc` 零分配 ABC Music Notation v2.1 乐谱头与 MIDI 音符/和弦流解析器（5/5 PASS，文档直写 `doc/lib/abc.md`）
实现零分配 ABC v2.1 乐谱解析器，支持 % 注释、吉他和弦文本与装饰符跳过。

## 2026-11-22: 新建 `cap:pcf` 零分配 X11 PCF 二进制点阵字体 TOC、度量、编码矩阵与属性解析器（5/5 PASS，文档直写 `doc/lib/pcf.md`）
实现零分配 X11 PCF 点阵字体解析器，支持表级大小端自适应与 4B 对齐字符串池切片。

## 2026-11-22: 新建 `cap:vgm` 零分配 Video Game Music (.vgm) 芯片音乐头、GD3 UTF-16LE 标签与命令流解析器（5/5 PASS，文档直写 `doc/lib/vgm.md`）
实现零分配 VGM 芯片音乐解析器，严格校验相对偏移、0x67 Data Block 跳过与 0x66 结束帧。

## 2026-11-22: 新建 `cap:sf2` 零分配 SoundFont 2.0 (.sf2) RIFF sfbk 音色库与 Hydra 表解析器（5/5 PASS，文档直写 `doc/lib/sf2.md`）
实现零分配 SoundFont 2.0 解析器，自动剥离末尾 EOP/EOI/EOS 哨兵并防越界样本访问。

## 2026-11-22: 新建 `cap:psf` 零分配 Linux 控制台 PSF1/PSF2 点阵字体、像素与 Unicode 映射表解析器（5/5 PASS，文档直写 `doc/lib/psf.md`）
实现零分配 Linux PSF1/PSF2 控制台字体解析器，支持 STARTSEQ 组合序列跳过与防溢出校验。

## 2026-11-22: 新建 `cap:bdf` 零分配 Adobe BDF 2.1/2.2 点阵字体头、属性表与字形位图解析器（5/5 PASS，文档直写 `doc/lib/bdf.md`）
实现零分配 Adobe BDF 点阵字体解析器，支持多字节宽字形与按码点查找。

## 2026-11-22: 新建 `cap:dae` 零分配 Khronos COLLADA 1.4/1.5 (.dae) 3D 资产与顶点数组解析器（5/5 PASS，文档直写 `doc/lib/dae.md`）
实现零分配 COLLADA 1.4/1.5 XML 解析器，支持前序嵌套场景节点遍历与资产汇总。

## 2026-11-22: 新建 `cap:vrml` 零分配 VRML97 / Classic X3D 3D 场景词法、节点树与字段解析器（5/5 PASS，文档直写 `doc/lib/vrml.md`）
实现零分配 VRML97/Classic X3D 场景解析器，支持嵌套 body_slice 递归查找与字符串闭合校验。

## 2026-11-22: 新建 `cap:usd` 零分配 Pixar OpenUSD (.usdc/.usda/.usdz) Crate TOC 与阶段元数据解析器（5/5 PASS，文档直写 `doc/lib/usd.md`）
实现零分配 Pixar OpenUSD 解析器，覆盖 usdc/usda/usdz 三形态与 TOC 防溢出校验。

## 2026-11-22: 新建 `cap:igc` 零分配 FAI IGC 滑翔机/航空飞行记录仪 A/H/B 记录解析器（5/5 PASS，文档直写 `doc/lib/igc.md`）
实现零分配 FAI IGC 飞行记录解析器，兼容新旧 HFDTE 日期格式并严控经纬度/UTC 范围。

## 2026-11-22: 新建 `cap:ais` 零分配 ITU-R M.1371 海事船舶 AIS 6-bit 装甲比特流与位置报文解码器（5/5 PASS，文档直写 `doc/lib/ais.md`）
实现零分配海事 AIS 报文解码器，支持二补码经纬度符号扩展与 6-bit 船名字符串解码。

## 2026-11-22: 新建 `cap:fbx` 零分配 Autodesk FBX Binary (7100-7700) 3D 场景树与属性解析器（5/5 PASS，文档直写 `doc/lib/fbx.md`）
实现零分配 Autodesk FBX Binary 解析器，严格校验 23B 魔数、end_offset 与 props_end 边界。

## 2026-11-22: 新建 `cap:bvh` 零分配 Biovision Hierarchy (.bvh) 3D 骨骼动作捕捉与帧数据解析器（5/5 PASS，文档直写 `doc/lib/bvh.md`）
实现零分配 Biovision BVH 解析器，支持科学计数法浮点帧解码、FPS 与总时长计算。

## 2026-11-22: 新建 `cap:fit` 零分配 Garmin / ANT+ FIT 1.0/2.0 二进制遥测文件与 CRC-16 解析器（5/5 PASS，文档直写 `doc/lib/fit.md`）
实现零分配 ANT+ FIT 遥测解析器，维护 16 槽 Local Message 定义表并支持完整头尾 CRC 校验。

## 2026-11-22: 新建 `cap:srec` 零分配 Motorola S-Record (S0-S9) 嵌入式固件编解码与校验器（5/5 PASS，文档直写 `doc/lib/srec.md`）
实现零分配 Motorola S-Record 固件编解码器，覆盖 16/24/32-bit 地址记录与入口终止帧。

## 2026-11-22: 新建 `cap:gcode` 零分配 CNC / 3D 打印 G-Code 指令词法、校验和与刀路包围盒解析器（5/5 PASS，文档直写 `doc/lib/gcode.md`）
实现零分配 G-Code 解析器，支持 ; 与 (...) 注释剥离、3D 包围盒及累计挤出耗材量计算。

## 2026-11-22: 新建 `cap:ihex` 零分配 Intel HEX (I8HEX/I16HEX/I32HEX) 嵌入式固件编解码与 32-bit 扩展地址解析器（5/5 PASS，文档直写 `doc/lib/ihex.md`）
实现零分配 Intel HEX 固件编解码器，覆盖 STM32/ARM 32-bit 线性基址与地址范围汇总。

## 2026-11-22: 新建 `cap:uf2` 零分配 Microsoft UF2 512B 微控制器固件块编解码与连续性校验器（5/5 PASS，文档直写 `doc/lib/uf2.md`）
实现零分配 Microsoft UF2 固件块编解码器，严格校验 block_no < num_blocks 与 payload_size <= 476。

## 2026-11-22: 新建 `cap:nmea` 零分配 NMEA 0183 GPS/GNSS 语句校验与 GGA/RMC/VTG 解析器（5/5 PASS，文档直写 `doc/lib/nmea.md`）
实现零分配 NMEA 0183 解析器，严格保留空字段并支持 $GP/$GN 与 !AIVDM 帧流。

## 2026-11-22: 新建 `cap:apk` 零分配 Android APK ZIP EOCD 与 APK Signing Block v2/v3 签名块解析器（5/5 PASS，文档直写 `doc/lib/apk.md`）
实现零分配 Android APK 签名块解析器，严格防下溢/越界并汇总 v2/v3/Verity Padding 存在性。

## 2026-11-22: 新建 `cap:pptx` 零分配 OOXML PresentationML (.pptx) 幻灯片清单、EMU 尺寸与文本提取器（5/5 PASS，文档直写 `doc/lib/pptx.md`）
实现零分配 OOXML PPTX 解析器，精准区分 <a:p> 与 <a:pPr> 及 id 与 r:id 属性。

## 2026-11-22: 新建 `cap:xlsx` 零分配 OOXML SpreadsheetML (.xlsx) 工作簿、共享串与单元格解析器（5/5 PASS，文档直写 `doc/lib/xlsx.md`）
实现零分配 OOXML XLSX 解析器，支持富文本 <r><t> 合并、<rPh> 拼音跳过与 XML 实体反转义。

## 2026-11-22: 新建 `cap:odt` 零分配 OASIS OpenDocument (.odt/.ods/.odp) 元数据与文本提取器（5/5 PASS，文档直写 `doc/lib/odt.md`）
实现零分配 OASIS OpenDocument 解析器，支持 <text:s text:c=N/> 展开、tab/line-break 与 XML 实体反转义。

## 2026-11-22: 新建 `cap:docx` 零分配 OOXML DOCX WordprocessingML 段落文本提取与 CoreProps 解析器（5/5 PASS，文档直写 `doc/lib/docx.md`）
实现零分配 OOXML DOCX 解析器，精准区分 <w:p> 与 <w:pPr>/<w:pStyle> 并支持嵌套段落深度匹配。

## 2026-11-22: 新建 `cap:epub` 零分配 EPUB 2/3 电子书 OCF 校验、container.xml 与 OPF 元数据/书脊解析器（5/5 PASS，文档直写 `doc/lib/epub.md`）
实现零分配 EPUB 2/3 解析器，兼容 dc:/opf: 命名空间、CDATA 与 unique-identifier 精确匹配。

## 2026-11-22: 新建 `cap:topojson` 零分配 TopoJSON v1.0 量化差分 Arc 解码与拓扑对象解析器（5/5 PASS，文档直写 `doc/lib/topojson.md`）
实现零分配 TopoJSON v1.0 解析器，支持 4/6 元 bbox、3D 坐标兼容与无溢出 ~index 解码。

## 2026-11-22: 新建 `cap:mobi` 零分配 Mobipocket / Kindle (.mobi/.azw) PalmDB/MOBI/EXTH 解析器（5/5 PASS，文档直写 `doc/lib/mobi.md`）
实现零分配 Kindle MOBI 电子书解析器，严格校验记录偏移单调递增与 EXTH 边界。

## 2026-11-22: 新建 `cap:gpx` 零分配 GPX 1.0/1.1 航点/轨迹 XML 解析与 Haversine 距离统计器（5/5 PASS，文档直写 `doc/lib/gpx.md`）
实现零分配 GPX 1.0/1.1 解析器，兼容单双引号属性、自闭合标签与严格经纬度校验。

## 2026-11-22: 新建 `cap:kml` 零分配 OGC KML 2.2 地理标记 Placemark 与 coordinates 解析器（5/5 PASS，文档直写 `doc/lib/kml.md`）
实现零分配 OGC KML 2.2 解析器，完整支持嵌套 Folder/Document、Point/LineString/Polygon 与经纬度范围校验。

## 2026-11-22: 新建 `cap:rtf` 零分配 Rich Text Format (RTF 1.9) 词法流、Unicode 转义与纯文本提取器（5/5 PASS，文档直写 `doc/lib/rtf.md`）
实现零分配 RTF 1.9 词法与文本提取器，完整遵循控制字单空格消费与 \ucN 替换跳过规范。

## 2026-11-22: 新建 `cap:xcf` 零分配 GIMP XCF v000-v014 分层图像头、属性表与图层偏移解析器（5/5 PASS，文档直写 `doc/lib/xcf.md`）
实现零分配 GIMP XCF 图像解析器，完整支持 32/64-bit 指针表防越界迭代与属性表解析。

## 2026-11-22: 新建 `cap:psd` 零分配 Adobe Photoshop (.psd/.psb) 文件头、区段与 8BIM 资源块解析器（5/5 PASS，文档直写 `doc/lib/psd.md`）
实现零分配 Photoshop PSD/PSB 容器解析器，覆盖全部 8 种色彩模式、4 种压缩与 ICC/XMP 资源定位。

## 2026-11-22: 新建 `cap:las` 零分配 ASPRS LAS 1.2-1.4 LiDAR 3D 点云头、VLR 与点记录解析器（5/5 PASS，文档直写 `doc/lib/las.md`）
实现零分配 ASPRS LAS LiDAR 点云解析器，兼容 32/64-bit 点数并严控 VLR 与点数据边界。

## 2026-11-22: 新建 `cap:shp` 零分配 ESRI Shapefile (.shp) 混合端序头、Bounding Box 与几何记录解析器（5/5 PASS，文档直写 `doc/lib/shp.md`）
实现零分配 ESRI Shapefile 解析器，完整覆盖 15 种 ShpType、NullShape 与 64B BBox。

## 2026-11-22: 新建 `cap:wkb` 零分配 OGC WKB / PostGIS EWKB 空间几何二进制编解码器（5/5 PASS，文档直写 `doc/lib/wkb.md`）
实现零分配 WKB/EWKB 空间几何编解码器，严控 32 层嵌套深度与点数乘法溢出边界。

## 2026-11-22: 新建 `cap:lucene` 零分配 Apache Lucene CodecUtil 索引头/尾帧与 VInt/VLong 解析器（5/5 PASS，文档直写 `doc/lib/lucene.md`）
实现零分配 Apache Lucene CodecUtil 解析器，严格校验魔数、algorithm_id==0 与高 32 位零校验和。

## 2026-11-22: 新建 `cap:flatbuf` 零分配 Google FlatBuffers 二进制根表、VTable 字段与向量解析器（5/5 PASS，文档直写 `doc/lib/flatbuf.md`）
实现零分配 Google FlatBuffers 解析器，全程严控 VTable 与字符串/向量下标越界。

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

