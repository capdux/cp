## 2026-11-22: 新建 `cap:ocsp` 零分配 RFC 6960 OCSP 请求与响应 ASN.1 DER 解析器（5/5 PASS，文档直写 `doc/lib/ocsp.md`）
实现零分配 RFC 6960 OCSP 解析器，兼容 byName/byKey ResponderID 并零拷贝暴露 tbs_raw_der。

## 2026-11-22: 新建 `cap:trx` 零分配 Visual Studio / .NET (.trx) 测试结果 XML 解析与汇总器（5/5 PASS，文档直写 `doc/lib/trx.md`）
实现零分配 Visual Studio .trx 测试报告解析器，缺失 Counters 时自动回退单遍实算。

## 2026-11-22: 新建 `cap:clf` 零分配 NCSA Common & Apache/Nginx Combined HTTP 访问日志解析与统计器（5/5 PASS，文档直写 `doc/lib/clf.md`）
实现零分配 HTTP 访问日志解析器，支持转义引号、缺失字节数 - 与畸形行安全跳过计数。

## 2026-11-22: 新建 `cap:crl` 零分配 RFC 5280 X.509 CRL 证书吊销列表 ASN.1 DER 解析器（5/5 PASS，文档直写 `doc/lib/crl.md`）
实现零分配 X.509 CRL 解析器，零拷贝暴露 tbs_raw_der 并兼容 UTCTime/GeneralizedTime。

## 2026-11-22: 新建 `cap:sarif` 零分配 OASIS SARIF 2.1.0 静态代码分析结果 JSON 解析与汇总器（5/5 PASS，文档直写 `doc/lib/sarif.md`）
实现零分配 SARIF 2.1.0 解析器，采用无递归 JSON 扫描内核安全跳过深嵌套 codeFlows。

## 2026-11-22: 新建 `cap:junit` 零分配 JUnit XML CI 测试报告套件/用例解析与汇总器（5/5 PASS，文档直写 `doc/lib/junit.md`）
实现零分配 JUnit XML 解析器，兼容 Maven/pytest/Jest/Cargo 报告格式与自闭合用例。

## 2026-11-22: 新建 `cap:csr` 零分配 RFC 2986 / PKCS#10 证书签发请求 (CSR) ASN.1 DER 解析器（5/5 PASS，文档直写 `doc/lib/csr.md`）
实现零分配 PKCS#10 CSR 解析器，严格校验 version==0、unused_bits==0 并零拷贝暴露 cri_raw_der。

## 2026-11-22: 新建 `cap:tap` 零分配 Test Anything Protocol (TAP v12/13/14) 测试流解析与汇总器（5/5 PASS，文档直写 `doc/lib/tap.md`）
实现零分配 TAP 测试结果流解析器，支持 \# 转义区分、尾部计划与 1..0 全跳过计划。

## 2026-11-22: 新建 `cap:pkcs7` 零分配 RFC 5652 CMS / PKCS#7 ASN.1 DER 签名容器解析器（5/5 PASS，文档直写 `doc/lib/pkcs7.md`）
实现零分配 CMS/PKCS#7 解析器，严格拒绝非最小 DER 长度与尾随垃圾字节。

## 2026-11-22: 新建 `cap:har` 零分配 W3C HTTP Archive (.har 1.2) 浏览器抓包日志与性能统计器（5/5 PASS，文档直写 `doc/lib/har.md`）
实现零分配 HAR 1.2 解析器，采用迭代式 JSON 值跳过安全处理嵌套 headers 与转义字符串。

## 2026-11-22: 新建 `cap:pgp` 零分配 OpenPGP RFC 4880/9580 Armor CRC-24 与 Old/New Packet 解析器（5/5 PASS，文档直写 `doc/lib/pgp.md`）
实现零分配 OpenPGP 解析器，内置流式 Base64 解码与 =xxxx CRC-24 强制校验。

## 2026-11-22: 新建 `cap:marc` 零分配 ISO 2709 / MARC 21 图书馆编目二进制记录解析器（5/5 PASS，文档直写 `doc/lib/marc.md`）
实现零分配 ISO 2709 MARC 21 书目记录解析器，支持多记录 .mrc 拼接流步进与严格边界校验。

## 2026-11-22: 新建 `cap:gedcom` 零分配 GEDCOM 5.5.1/7.0 家谱与谱系数据流式解析器（5/5 PASS，文档直写 `doc/lib/gedcom.md`）
实现零分配 GEDCOM 5.5.1/7.0 解析器，支持指针值检测与个人/家庭实体迭代。

## 2026-11-22: 新建 `cap:sshsig` 零分配 OpenSSH SSHSIG 二进制与 PEM Armor 签名解析与待签载荷构造器（5/5 PASS，文档直写 `doc/lib/sshsig.md`）
实现零分配 OpenSSH PROTOCOL.sshsig 解析器，严格校验非空 namespace 与无尾随垃圾字节。

## 2026-11-22: 新建 `cap:minisign` 零分配 Minisign & OpenBSD Signify Ed25519 签名与公钥编解码器（5/5 PASS，文档直写 `doc/lib/minisign.md`）
实现零分配 Minisign/Signify 编解码器，内置严格 RFC 4648 Base64 校验与 ED/Ed 预哈希区分。

## 2026-11-22: 新建 `cap:ris` 零分配 Research Information Systems (.ris) 学术文献引文解析与统计器（5/5 PASS，文档直写 `doc/lib/ris.md`）
实现零分配 RIS 学术引文解析器，兼容 UTF-8 BOM、CRLF 与自定义标签查询。

## 2026-11-22: 新建 `cap:leef` 零分配 IBM QRadar LEEF 1.0/2.0 SIEM 日志解析与格式化器（5/5 PASS，文档直写 `doc/lib/leef.md`）
实现零分配 IBM QRadar LEEF 1.0/2.0 解析器，兼容 Syslog 前缀与值内嵌等号。

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

