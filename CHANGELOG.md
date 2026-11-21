# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:eot` 零分配 Microsoft Embedded OpenType (.eot) Web 字体容器与 UTF-16LE 名称解析器（5/5 PASS，自动动态挂载 `cap.eot`）
新增 `lib/eot/{eot,index}.cp`、`doc/lib/eot.md` 与 `cap/test/test_eot_production_deep.cp`（5/5 PASS），支持 EOT v1.0/v2.1/v2.2 头、root_string 白名单、代理对 UTF-16LE 转 UTF-8 与内嵌字体切片。

## [2026-11-22] - 新建 `cap:nsf` 零分配 NES Sound Format (.nsf/.nsfe) 芯片音乐头与分块容器解析器（5/5 PASS，自动动态挂载 `cap.nsf`）
新增 `lib/nsf/{nsf,index}.cp`、`doc/lib/nsf.md` 与 `cap/test/test_nsf_production_deep.cp`（5/5 PASS），支持 128B NSF1/2 头、6 种扩展芯片位掩码、Bankswitching 检测与 NSFe 分块迭代。

## [2026-11-22] - 新建 `cap:abc` 零分配 ABC Music Notation v2.1 乐谱头与 MIDI 音符/和弦流解析器（5/5 PASS，自动动态挂载 `cap.abc`）
新增 `lib/abc/{abc,index}.cp`、`doc/lib/abc.md` 与 `cap/test/test_abc_production_deep.cp`（5/5 PASS），支持 X/T/M/L/Q/K 头解析、升降号/八度 MIDI 音高换算、分数时值与小节线/和弦流。

## [2026-11-22] - 新建 `cap:pcf` 零分配 X11 PCF 二进制点阵字体 TOC、度量、编码矩阵与属性解析器（5/5 PASS，自动动态挂载 `cap.pcf`）
新增 `lib/pcf/{pcf,index}.cp`、`doc/lib/pcf.md` 与 `cap/test/test_pcf_production_deep.cp`（5/5 PASS），支持 9 类表 TOC 迭代、5B 压缩/12B LE/BE 度量、2D 编码矩阵 O(1) 查表与属性迭代。

## [2026-11-22] - 新建 `cap:vgm` 零分配 Video Game Music (.vgm) 芯片音乐头、GD3 UTF-16LE 标签与命令流解析器（5/5 PASS，自动动态挂载 `cap.vgm`）
新增 `lib/vgm/{vgm,index}.cp`、`doc/lib/vgm.md` 与 `cap/test/test_vgm_production_deep.cp`（5/5 PASS），支持 v1.00-v1.71 BCD 版本头、GD3 11 字段 UTF-16LE 转 UTF-8 与命令流采样累计。

## [2026-11-22] - 新建 `cap:sf2` 零分配 SoundFont 2.0 (.sf2) RIFF sfbk 音色库与 Hydra 表解析器（5/5 PASS，自动动态挂载 `cap.sf2`）
新增 `lib/sf2/{sf2,index}.cp`、`doc/lib/sf2.md` 与 `cap/test/test_sf2_production_deep.cp`（5/5 PASS），支持 INFO/sdta/pdta 解析、Preset/Instrument/Sample 迭代与 16-bit PCM 零拷贝切片。

## [2026-11-22] - 新建 `cap:psf` 零分配 Linux 控制台 PSF1/PSF2 点阵字体、像素与 Unicode 映射表解析器（5/5 PASS，自动动态挂载 `cap.psf`）
新增 `lib/psf/{psf,index}.cp`、`doc/lib/psf.md` 与 `cap/test/test_psf_production_deep.cp`（5/5 PASS），支持 PSF1/PSF2 统一头解析、MSB-first 像素解码及 UCS-2/UTF-8 Unicode 表查找。

## [2026-11-22] - 新建 `cap:bdf` 零分配 Adobe BDF 2.1/2.2 点阵字体头、属性表与字形位图解析器（5/5 PASS，自动动态挂载 `cap.bdf`）
新增 `lib/bdf/{bdf,index}.cp`、`doc/lib/bdf.md` 与 `cap/test/test_bdf_production_deep.cp`（5/5 PASS），支持 BDF 全局头、PROPERTIES 引号剥离、STARTCHAR/BBX/BITMAP 迭代与 MSB-first 像素解码。

## [2026-11-22] - 新建 `cap:dae` 零分配 Khronos COLLADA 1.4/1.5 (.dae) 3D 资产与顶点数组解析器（5/5 PASS，自动动态挂载 `cap.dae`）
新增 `lib/dae/{dae,index}.cp`、`doc/lib/dae.md` 与 `cap/test/test_dae_production_deep.cp`（5/5 PASS），支持 <asset> 元数据、几何体与科学计数法 <float_array> 迭代及场景节点引用提取。

## [2026-11-22] - 新建 `cap:vrml` 零分配 VRML97 / Classic X3D 3D 场景词法、节点树与字段解析器（5/5 PASS，自动动态挂载 `cap.vrml`）
新增 `lib/vrml/{vrml,index}.cp`、`doc/lib/vrml.md` 与 `cap/test/test_vrml_production_deep.cp`（5/5 PASS），支持 VRML97/X3D 头解析、逗号空白词法、DEF/USE/ROUTE 迭代与括号平衡统计。

## [2026-11-22] - 新建 `cap:usd` 零分配 Pixar OpenUSD (.usdc/.usda/.usdz) Crate TOC 与阶段元数据解析器（5/5 PASS，自动动态挂载 `cap.usd`）
新增 `lib/usd/{usd,index}.cp`、`doc/lib/usd.md` 与 `cap/test/test_usd_production_deep.cp`（5/5 PASS），支持 88B Crate Bootstrap、32B TOC 区段零拷贝迭代、.usda 阶段元数据与 .usdz 未压缩首资产探测。

## [2026-11-22] - 新建 `cap:igc` 零分配 FAI IGC 滑翔机/航空飞行记录仪 A/H/B 记录解析器（5/5 PASS，自动动态挂载 `cap.igc`）
新增 `lib/igc/{igc,index}.cp`、`doc/lib/igc.md` 与 `cap/test/test_igc_production_deep.cp`（5/5 PASS），支持 A/H 头记录、B 定宽 35B 定位记录、负海拔与跨午夜 UTC 飞行时长统计。

## [2026-11-22] - 新建 `cap:ais` 零分配 ITU-R M.1371 海事船舶 AIS 6-bit 装甲比特流与位置报文解码器（5/5 PASS，自动动态挂载 `cap.ais`）
新增 `lib/ais/{ais,index}.cp`、`doc/lib/ais.md` 与 `cap/test/test_ais_production_deep.cp`（5/5 PASS），支持 !AIVDM/!AIVDO XOR 校验、6-bit 比特流读取、Class A (1/2/3) 与 Class B (18) 报文解码。

## [2026-11-22] - 新建 `cap:fbx` 零分配 Autodesk FBX Binary (7100-7700) 3D 场景树与属性解析器（5/5 PASS，自动动态挂载 `cap.fbx`）
新增 `lib/fbx/{fbx,index}.cp`、`doc/lib/fbx.md` 与 `cap/test/test_fbx_production_deep.cp`（5/5 PASS），支持 32/64-bit 节点头、NULL sentinel 终止与全标量/字符串/数组属性零拷贝解码。

## [2026-11-22] - 新建 `cap:bvh` 零分配 Biovision Hierarchy (.bvh) 3D 骨骼动作捕捉与帧数据解析器（5/5 PASS，自动动态挂载 `cap.bvh`）
新增 `lib/bvh/{bvh,index}.cp`、`doc/lib/bvh.md` 与 `cap/test/test_bvh_production_deep.cp`（5/5 PASS），支持 ROOT/JOINT/End Site 骨骼树遍历、channel_offset 累加与 MOTION 浮点帧流迭代。

## [2026-11-22] - 新建 `cap:fit` 零分配 Garmin / ANT+ FIT 1.0/2.0 二进制遥测文件与 CRC-16 解析器（5/5 PASS，自动动态挂载 `cap.fit`）
新增 `lib/fit/{fit,index}.cp`、`doc/lib/fit.md` 与 `cap/test/test_fit_production_deep.cp`（5/5 PASS），支持 nibble CRC-16、LE/BE Definition/Data 帧、Developer Fields 与压缩时间戳。

## [2026-11-22] - 新建 `cap:srec` 零分配 Motorola S-Record (S0-S9) 嵌入式固件编解码与校验器（5/5 PASS，自动动态挂载 `cap.srec`）
新增 `lib/srec/{srec,index}.cp`、`doc/lib/srec.md` 与 `cap/test/test_srec_production_deep.cp`（5/5 PASS），支持 S0-S9 编解码、一补码校验和与 S5/S6 计数核验。

## [2026-11-22] - 新建 `cap:gcode` 零分配 CNC / 3D 打印 G-Code 指令词法、校验和与刀路包围盒解析器（5/5 PASS，自动动态挂载 `cap.gcode`）
新增 `lib/gcode/{gcode,index}.cp`、`doc/lib/gcode.md` 与 `cap/test/test_gcode_production_deep.cp`（5/5 PASS），支持紧凑参数词法、RepRap XOR 校验和与 G90/G91/M82/M83/G92 状态机刀路统计。

## [2026-11-22] - 新建 `cap:ihex` 零分配 Intel HEX (I8HEX/I16HEX/I32HEX) 嵌入式固件编解码与 32-bit 扩展地址解析器（5/5 PASS，自动动态挂载 `cap.ihex`）
新增 `lib/ihex/{ihex,index}.cp`、`doc/lib/ihex.md` 与 `cap/test/test_ihex_production_deep.cp`（5/5 PASS），支持二补码校验和、02/04 扩展地址换算、05 入口点与零分配行编码。

## [2026-11-22] - 新建 `cap:uf2` 零分配 Microsoft UF2 512B 微控制器固件块编解码与连续性校验器（5/5 PASS，自动动态挂载 `cap.uf2`）
新增 `lib/uf2/{uf2,index}.cp`、`doc/lib/uf2.md` 与 `cap/test/test_uf2_production_deep.cp`（5/5 PASS），支持三重魔数校验、RP2040/ESP32-S3/STM32 Family ID、512B 编解码与 Flash 地址连续性汇总。

## [2026-11-22] - 新建 `cap:nmea` 零分配 NMEA 0183 GPS/GNSS 语句校验与 GGA/RMC/VTG 解析器（5/5 PASS，自动动态挂载 `cap.nmea`）
新增 `lib/nmea/{nmea,index}.cp`、`doc/lib/nmea.md` 与 `cap/test/test_nmea_production_deep.cp`（5/5 PASS），支持 XOR 校验和、ddmm.mmmm 经纬度换算、GGA/RMC/VTG 解码与多行串口流过滤。

## [2026-11-22] - 新建 `cap:apk` 零分配 Android APK ZIP EOCD 与 APK Signing Block v2/v3 签名块解析器（5/5 PASS，自动动态挂载 `cap.apk`）
新增 `lib/apk/{apk,index}.cp`、`doc/lib/apk.md` 与 `cap/test/test_apk_production_deep.cp`（5/5 PASS），支持 EOCD 反向定位、APK Sig Block 42 顶底长度一致性校验与 v2/v3/v3.1 Pair 迭代。

## [2026-11-22] - 新建 `cap:pptx` 零分配 OOXML PresentationML (.pptx) 幻灯片清单、EMU 尺寸与文本提取器（5/5 PASS，自动动态挂载 `cap.pptx`）
新增 `lib/pptx/{pptx,index}.cp`、`doc/lib/pptx.md` 与 `cap/test/test_pptx_production_deep.cp`（6/6 PASS），支持 16:9 EMU 尺寸换算、幻灯片/备注纯文本提取与 Shape/Table/Pic 统计。

## [2026-11-22] - 新建 `cap:xlsx` 零分配 OOXML SpreadsheetML (.xlsx) 工作簿、共享串与单元格解析器（5/5 PASS，自动动态挂载 `cap.xlsx`）
新增 `lib/xlsx/{xlsx,index}.cp`、`doc/lib/xlsx.md` 与 `cap/test/test_xlsx_production_deep.cp`（5/5 PASS），支持 A1..XFD1048576 引用编解码、workbook/sharedStrings 与 sheetData 单元格迭代。

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

