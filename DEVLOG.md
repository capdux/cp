## 2026-11-22: 新建 `cap:age` 零分配 age v1 现代文件加密头、Stanza 与 Bech32 密钥校验器（5/5 PASS，文档直写 `doc/lib/age.md`）
实现零分配 age v1 加密容器解析器，提取 HMAC-SHA256 签名输入切片与 16B STREAM Nonce。

## 2026-11-22: 新建 `cap:ldif` 零分配 RFC 2849 LDAP LDIF 折行展开、Base64 属性与变更记录解析器（5/5 PASS，文档直写 `doc/lib/ldif.md`）
实现零分配 RFC 2849 LDIF 解析器，支持大小写不敏感 LDAP 属性查找与单遍变更统计。

## 2026-11-22: 新建 `cap:cef` 零分配 ArcSight CEF SIEM 日志解析、Extension 切分与转义编解码器（5/5 PASS，文档直写 `doc/lib/cef.md`）
实现零分配 ArcSight CEF Rev 25 日志解析器，准确区分 \| 与 \\| 转义并统一映射数字/文本 Severity。

## 2026-11-22: 新建 `cap:der` 零分配 ITU-T X.690 ASN.1 DER 严格规范 TLV/OID 编解码器（5/5 PASS，文档直写 `doc/lib/der.md`）
实现零分配 ASN.1 DER 规范编解码器，拒绝不定长 0x80、非最小长度、冗余整型填充与非法布尔。

## 2026-11-22: 新建 `cap:warc` 零分配 ISO 28500 WARC 1.0/1.1 网页归档与 Common Crawl 语料解析器（5/5 PASS，文档直写 `doc/lib/warc.md`）
实现零分配 ISO 28500 WARC 归档解析器，严格校验双空行分隔符与大小写不敏感头查找。

## 2026-11-22: 新建 `cap:inchi` 零分配 IUPAC InChI v1.06 分层结构、Hill 分子式与 27 字符 InChIKey 解析器（5/5 PASS，文档直写 `doc/lib/inchi.md`）
实现零分配 IUPAC InChI 与 InChIKey 解析器，严格校验 27 字符块结构与全部子层前缀。

## 2026-11-22: 新建 `cap:smi` 零分配 Daylight SMILES / OpenSMILES 分子词法与环配平校验器（5/5 PASS，文档直写 `doc/lib/smi.md`）
实现零分配 Daylight SMILES 解析器，采用栈上 100 位环表校验成对闭合与分支平衡。

## 2026-11-22: 新建 `cap:cif` 零分配 IUCr CIF / mmCIF 晶体学与生物大分子结构词法与 loop_ 表解析器（5/5 PASS，文档直写 `doc/lib/cif.md`）
实现零分配 IUCr CIF 1.1 / mmCIF 解析器，支持单双引号空白闭合与缺失/未知哨兵区分。

## 2026-11-22: 新建 `cap:mol2` 零分配 Tripos SYBYL (.mol2) 3D 分子结构与力场原子类型解析器（5/5 PASS，文档直写 `doc/lib/mol2.md`）
实现零分配 Tripos MOL2 分子结构解析器，兼容可选子结构/电荷列缺失与 # 注释。

## 2026-11-22: 新建 `cap:segy` 零分配 SEG-Y Rev 1/2 地球物理地震反射与 IBM 浮点转换解析器（5/5 PASS，文档直写 `doc/lib/segy.md`）
实现零分配 SEG-Y Rev 1/2 解析器，支持 6 种采样格式统一解码与坐标标量换算。



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

