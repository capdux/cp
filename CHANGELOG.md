# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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

