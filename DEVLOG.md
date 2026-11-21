

## 2026-11-22: 新建 `cap:mseed` 零分配 IRIS MiniSEED v2.4 地震学波形头与 Blockette 1000/1001 解析器（5/5 PASS，文档直写 `doc/lib/mseed.md`）
实现零分配 FDSN MiniSEED v2.4 解析器，支持 Steim1/Steim2/IEEE 编码识别与多记录流式步进。

## 2026-11-22: 新建 `cap:sdf` 零分配 MDL Molfile V2000 & SDF 多分子原子/键连接表解析器（5/5 PASS，文档直写 `doc/lib/sdf.md`）
实现零分配 MDL Molfile V2000/SDF 化学结构解析器，严格校验键原子索引 1..atom_count 与截断边界。

## 2026-11-22: 新建 `cap:edf` 零分配 European Data Format (EDF/EDF+) 生物医学脑电/心电信号解析器（5/5 PASS，文档直写 `doc/lib/edf.md`）
实现零分配 EDF/EDF+ 生物医学信号解析器，兼容 EDF+C 实时流（num_records=-1）与除零安全防护。

## 2026-11-22: 新建 `cap:dicom` 零分配医学影像 DICOM PS3.10 Explicit/Implicit VR 数据元素解析器（5/5 PASS，文档直写 `doc/lib/dicom.md`）
实现零分配 DICOM PS3.10 医学影像解析器，支持单遍提取患者/模态/图像尺寸与像素数据标志。

## 2026-11-22: 新建 `cap:pdb` 零分配 RCSB Protein Data Bank (.pdb) v3.3 生物大分子 3D 结构解析器（5/5 PASS，文档直写 `doc/lib/pdb.md`）
实现零分配 RCSB PDB v3.3 解析器，支持 54-80 列兼容、栈上 256B 链去重与包围盒质心单遍计算。

## 2026-11-22: 新建 `cap:gff` 零分配生物信息学 GFF3 / GTF2 基因特征注释与属性键值解析器（5/5 PASS，文档直写 `doc/lib/gff.md`）
实现零分配 GFF3/GTF2 注释解析器，严格校验 1-based 闭区间 start<=end 与 phase 范围。

## 2026-11-22: 新建 `cap:sam` 零分配生物信息学 SAM v1.6 序列比对 FLAG 与 CIGAR 解析器（5/5 PASS，文档直写 `doc/lib/sam.md`）
实现零分配 SAM v1.6 序列比对解析器，支持可选 TAG 查找与比对汇总统计。

## 2026-11-22: 新建 `cap:vcf` 零分配生物信息学 VCF 4.2/4.3 基因变异记录、INFO 与 Ti/Tv 解析器（5/5 PASS，文档直写 `doc/lib/vcf.md`）
实现零分配 VCF 4.2/4.3 基因变异解析器，支持布尔 Flag 与 KEY=VAL 混合 INFO 零拷贝查询。

