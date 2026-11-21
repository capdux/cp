# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:smi` 零分配 Daylight SMILES / OpenSMILES 分子词法与环配平校验器（5/5 PASS，自动动态挂载 `cap.smi`）
新增 `lib/smi/{smi,index}.cp`、`doc/lib/smi.md` 与 `cap/test/test_smi_production_deep.cp`（5/5 PASS），支持双字母卤素区分、括号原子全字段解析、%NN 双位环闭合配平与单遍图统计。

## [2026-11-22] - 新建 `cap:cif` 零分配 IUCr CIF / mmCIF 晶体学与生物大分子结构词法与 loop_ 表解析器（5/5 PASS，自动动态挂载 `cap.cif`）
新增 `lib/cif/{cif,index}.cp`、`doc/lib/cif.md` 与 `cap/test/test_cif_production_deep.cp`（5/5 PASS），支持 data_/save_/loop_ 词法、行首 ;...; 多行文本、顶层标签查询与 loop_ 表格按行迭代。

## [2026-11-22] - 新建 `cap:mol2` 零分配 Tripos SYBYL (.mol2) 3D 分子结构与力场原子类型解析器（5/5 PASS，自动动态挂载 `cap.mol2`）
新增 `lib/mol2/{mol2,index}.cp`、`doc/lib/mol2.md` 与 `cap/test/test_mol2_production_deep.cp`（5/5 PASS），支持 @<TRIPOS>MOLECULE/ATOM/BOND 区段、SYBYL 原子类型、多分子流与净电荷统计。

## [2026-11-22] - 新建 `cap:segy` 零分配 SEG-Y Rev 1/2 地球物理地震反射与 IBM 浮点转换解析器（5/5 PASS，自动动态挂载 `cap.segy`）
新增 `lib/segy/{segy,index}.cp`、`doc/lib/segy.md` 与 `cap/test/test_segy_production_deep.cp`（5/5 PASS），支持 3600B 卷头、扩展文本头、IBM S/360 浮点转 IEEE-754 与 240B 道迭代。

## [2026-11-22] - 新建 `cap:mseed` 零分配 IRIS MiniSEED v2.4 地震学波形头与 Blockette 1000/1001 解析器（5/5 PASS，自动动态挂载 `cap.mseed`）
新增 `lib/mseed/{mseed,index}.cp`、`doc/lib/mseed.md` 与 `cap/test/test_mseed_production_deep.cp`（5/5 PASS），支持 BTIME 大小端自动识别、Blockette 1000/1001 链式解析与四象限采样率计算。

## [2026-11-22] - 新建 `cap:sdf` 零分配 MDL Molfile V2000 & SDF 多分子原子/键连接表解析器（5/5 PASS，自动动态挂载 `cap.sdf`）
新增 `lib/sdf/{sdf,index}.cp`、`doc/lib/sdf.md` 与 `cap/test/test_sdf_production_deep.cp`（5/5 PASS），支持 V2000 原子/键连接表、电荷码映射、多行关联属性与 $$$$ 多分子流。

## [2026-11-22] - 新建 `cap:edf` 零分配 European Data Format (EDF/EDF+) 生物医学脑电/心电信号解析器（5/5 PASS，自动动态挂载 `cap.edf`）
新增 `lib/edf/{edf,index}.cp`、`doc/lib/edf.md` 与 `cap/test/test_edf_production_deep.cp`（6/6 PASS），支持 256B 定宽主头、列式通道头、物理量线性定标与交错 16-bit 样本随机读取。

## [2026-11-22] - 新建 `cap:dicom` 零分配医学影像 DICOM PS3.10 Explicit/Implicit VR 数据元素解析器（5/5 PASS，自动动态挂载 `cap.dicom`）
新增 `lib/dicom/{dicom,index}.cp`、`doc/lib/dicom.md` 与 `cap/test/test_dicom_production_deep.cp`（5/5 PASS），支持 Group 0002 Transfer Syntax 自动切换 Explicit/Implicit LE/BE、13 种长 VR 与未定义长度 SQ 跳过。

## [2026-11-22] - 新建 `cap:pdb` 零分配 RCSB Protein Data Bank (.pdb) v3.3 生物大分子 3D 结构解析器（5/5 PASS，自动动态挂载 `cap.pdb`）
新增 `lib/pdb/{pdb,index}.cp`、`doc/lib/pdb.md` 与 `cap/test/test_pdb_production_deep.cp`（6/6 PASS），支持 80 列 HEADER/ATOM/HETATM/MODEL/CONECT 解析、3D 欧氏距离与质心统计。

## [2026-11-22] - 新建 `cap:gff` 零分配生物信息学 GFF3 / GTF2 基因特征注释与属性键值解析器（5/5 PASS，自动动态挂载 `cap.gff`）
新增 `lib/gff/{gff,index}.cp`、`doc/lib/gff.md` 与 `cap/test/test_gff_production_deep.cp`（5/5 PASS），支持 9 列记录、GFF3/GTF2 双格式属性查找、##FASTA 截断停止与单遍统计。

## [2026-11-22] - 新建 `cap:sam` 零分配生物信息学 SAM v1.6 序列比对 FLAG 与 CIGAR 解析器（5/5 PASS，自动动态挂载 `cap.sam`）
新增 `lib/sam/{sam,index}.cp`、`doc/lib/sam.md` 与 `cap/test/test_sam_production_deep.cp`（5/5 PASS），支持 12 种 FLAG 位掩码、9 种 CIGAR 操作码跨度计算、@HD/@SQ 头与 11 列比对记录迭代。

## [2026-11-22] - 新建 `cap:vcf` 零分配生物信息学 VCF 4.2/4.3 基因变异记录、INFO 与 Ti/Tv 解析器（5/5 PASS，自动动态挂载 `cap.vcf`）
新增 `lib/vcf/{vcf,index}.cp`、`doc/lib/vcf.md` 与 `cap/test/test_vcf_production_deep.cp`（5/5 PASS），支持 VCFv4.x 头、8 必选列与样本切片、INFO 查找及 Ti/Tv 转换颠换比统计。

