# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:clf` 零分配 NCSA Common & Apache/Nginx Combined HTTP 访问日志解析与统计器（5/5 PASS，自动动态挂载 `cap.clf`）
新增 `lib/clf/{clf,index}.cp`、`doc/lib/clf.md` 与 `cap/test/test_clf_production_deep.cp`（5/5 PASS），支持 Common/Combined 双格式、IPv6、请求行自动拆分与状态码/流量汇总。

## [2026-11-22] - 新建 `cap:crl` 零分配 RFC 5280 X.509 CRL 证书吊销列表 ASN.1 DER 解析器（5/5 PASS，自动动态挂载 `cap.crl`）
新增 `lib/crl/{crl,index}.cp`、`doc/lib/crl.md` 与 `cap/test/test_crl_production_deep.cp`（5/5 PASS），支持 v1/v2 CRL DER 解析、吊销序列号规范化查验与 CRLReason 枚举提取。

## [2026-11-22] - 新建 `cap:sarif` 零分配 OASIS SARIF 2.1.0 静态代码分析结果 JSON 解析与汇总器（5/5 PASS，自动动态挂载 `cap.sarif`）
新增 `lib/sarif/{sarif,index}.cp`、`doc/lib/sarif.md` 与 `cap/test/test_sarif_production_deep.cp`（5/5 PASS），支持多 runs/results 零拷贝迭代、位置行列提取与单遍汇总。

## [2026-11-22] - 新建 `cap:junit` 零分配 JUnit XML CI 测试报告套件/用例解析与汇总器（5/5 PASS，自动动态挂载 `cap.junit`）
新增 `lib/junit/{junit,index}.cp`、`doc/lib/junit.md` 与 `cap/test/test_junit_production_deep.cp`（5/5 PASS），支持 <testsuites>/<testsuite>/<testcase> 四态解析、CDATA 堆栈提取与单遍汇总。

## [2026-11-22] - 新建 `cap:csr` 零分配 RFC 2986 / PKCS#10 证书签发请求 (CSR) ASN.1 DER 解析器（5/5 PASS，自动动态挂载 `cap.csr`）
新增 `lib/csr/{csr,index}.cp`、`doc/lib/csr.md` 与 `cap/test/test_csr_production_deep.cp`（5/5 PASS），支持 PKCS#10 DER 解析、Subject RDN 查询与 extensionRequest SAN 域名/IP 提取。

## [2026-11-22] - 新建 `cap:tap` 零分配 Test Anything Protocol (TAP v12/13/14) 测试流解析与汇总器（5/5 PASS，自动动态挂载 `cap.tap`）
新增 `lib/tap/{tap,index}.cp`、`doc/lib/tap.md` 与 `cap/test/test_tap_production_deep.cp`（5/5 PASS），支持 TAP 12-14、YAML 诊断块聚合、# SKIP/# TODO 指令与 Bail out! 汇总。

## [2026-11-22] - 新建 `cap:pkcs7` 零分配 RFC 5652 CMS / PKCS#7 ASN.1 DER 签名容器解析器（5/5 PASS，自动动态挂载 `cap.pkcs7`）
新增 `lib/pkcs7/{pkcs7,index}.cp`、`doc/lib/pkcs7.md` 与 `cap/test/test_pkcs7_production_deep.cp`（5/5 PASS），支持 ContentInfo/SignedData（含 detached）、证书集与 SignerInfo 零拷贝迭代。

## [2026-11-22] - 新建 `cap:har` 零分配 W3C HTTP Archive (.har 1.2) 浏览器抓包日志与性能统计器（5/5 PASS，自动动态挂载 `cap.har`）
新增 `lib/har/{har,index}.cp`、`doc/lib/har.md` 与 `cap/test/test_har_production_deep.cp`（5/5 PASS），支持 HAR 1.2 头/请求条目零拷贝迭代、HarTimings 与平均 TTFB 统计。

## [2026-11-22] - 新建 `cap:pgp` 零分配 OpenPGP RFC 4880/9580 Armor CRC-24 与 Old/New Packet 解析器（5/5 PASS，自动动态挂载 `cap.pgp`）
新增 `lib/pgp/{pgp,index}.cp`、`doc/lib/pgp.md` 与 `cap/test/test_pgp_production_deep.cp`（5/5 PASS），支持 Radix-64 CRC-24 校验、Old/New Packet 迭代与 PublicKey/Signature/LiteralData 解码。

## [2026-11-22] - 新建 `cap:marc` 零分配 ISO 2709 / MARC 21 图书馆编目二进制记录解析器（5/5 PASS，自动动态挂载 `cap.marc`）
新增 `lib/marc/{marc,index}.cp`、`doc/lib/marc.md` 与 `cap/test/test_marc_production_deep.cp`（5/5 PASS），支持 24B Leader、12B 目录表、控制/数据字段与 0x1F 子字段迭代。

## [2026-11-22] - 新建 `cap:gedcom` 零分配 GEDCOM 5.5.1/7.0 家谱与谱系数据流式解析器（5/5 PASS，自动动态挂载 `cap.gedcom`）
新增 `lib/gedcom/{gedcom,index}.cp`、`doc/lib/gedcom.md` 与 `cap/test/test_gedcom_production_deep.cp`（5/5 PASS），支持 HEAD/INDI/FAM/TRLR 解析、层级跳跃校验与单遍汇总。

## [2026-11-22] - 新建 `cap:sshsig` 零分配 OpenSSH SSHSIG 二进制与 PEM Armor 签名解析与待签载荷构造器（5/5 PASS，自动动态挂载 `cap.sshsig`）
新增 `lib/sshsig/{sshsig,index}.cp`、`doc/lib/sshsig.md` 与 `cap/test/test_sshsig_production_deep.cp`（5/5 PASS），支持 SSHSIG v1 魔数、PEM Armor Base64 解码、嵌套 key_type/sig_type 提取与 SignedData 构造。

## [2026-11-22] - 新建 `cap:minisign` 零分配 Minisign & OpenBSD Signify Ed25519 签名与公钥编解码器（5/5 PASS，自动动态挂载 `cap.minisign`）
新增 `lib/minisign/{minisign,index}.cp`、`doc/lib/minisign.md` 与 `cap/test/test_minisign_production_deep.cp`（5/5 PASS），支持 2 行 .pub / 单行 RW 公钥、4 行 .minisig / 2 行 Signify 签名编解码与 Key ID 匹配。

## [2026-11-22] - 新建 `cap:ris` 零分配 Research Information Systems (.ris) 学术文献引文解析与统计器（5/5 PASS，自动动态挂载 `cap.ris`）
新增 `lib/ris/{ris,index}.cp`、`doc/lib/ris.md` 与 `cap/test/test_ris_production_deep.cp`（5/5 PASS），支持 TY/ER 文献流、多行续行摘要、多作者/关键词与单遍汇总。

## [2026-11-22] - 新建 `cap:leef` 零分配 IBM QRadar LEEF 1.0/2.0 SIEM 日志解析与格式化器（5/5 PASS，自动动态挂载 `cap.leef`）
新增 `lib/leef/{leef,index}.cp`、`doc/lib/leef.md` 与 `cap/test/test_leef_production_deep.cp`（5/5 PASS），支持 LEEF 1.0 Tab 与 2.0 自定义/十六进制分隔符、属性查找与零分配头部编码。

## [2026-11-22] - 新建 `cap:age` 零分配 age v1 现代文件加密头、Stanza 与 Bech32 密钥校验器（5/5 PASS，自动动态挂载 `cap.age`）
新增 `lib/age/{age,index}.cp`、`doc/lib/age.md` 与 `cap/test/test_age_production_deep.cp`（5/5 PASS），支持严格 LF v1 头、64 列折行 Stanza、43B MAC 与 age1/AGE-SECRET-KEY-1 密钥校验。

## [2026-11-22] - 新建 `cap:ldif` 零分配 RFC 2849 LDAP LDIF 折行展开、Base64 属性与变更记录解析器（5/5 PASS，自动动态挂载 `cap.ldif`）
新增 `lib/ldif/{ldif,index}.cp`、`doc/lib/ldif.md` 与 `cap/test/test_ldif_production_deep.cp`（5/5 PASS），支持 version: 1、RFC 2849 单空格续行展开、Base64/URL 属性与 changetype 解析。

## [2026-11-22] - 新建 `cap:cef` 零分配 ArcSight CEF SIEM 日志解析、Extension 切分与转义编解码器（5/5 PASS，自动动态挂载 `cap.cef`）
新增 `lib/cef/{cef,index}.cp`、`doc/lib/cef.md` 与 `cap/test/test_cef_production_deep.cp`（5/5 PASS），支持 Syslog 前缀保留、头部未转义 | 切分、含空格 Extension 解析与 formatCefHeader。

## [2026-11-22] - 新建 `cap:der` 零分配 ITU-T X.690 ASN.1 DER 严格规范 TLV/OID 编解码器（5/5 PASS，自动动态挂载 `cap.der`）
新增 `lib/der/{der,index}.cp`、`doc/lib/der.md` 与 `cap/test/test_der_production_deep.cp`（5/5 PASS），支持严格 DER TLV、OID 格式化、BitString 未用位零校验与原子 DerWriter。

## [2026-11-22] - 新建 `cap:warc` 零分配 ISO 28500 WARC 1.0/1.1 网页归档与 Common Crawl 语料解析器（5/5 PASS，自动动态挂载 `cap.warc`）
新增 `lib/warc/{warc,index}.cp`、`doc/lib/warc.md` 与 `cap/test/test_warc_production_deep.cp`（5/5 PASS），支持 WARC/1.0-1.1 头、Content-Length 精确二进制负载切片与多记录流统计。

## [2026-11-22] - 新建 `cap:inchi` 零分配 IUPAC InChI v1.06 分层结构、Hill 分子式与 27 字符 InChIKey 解析器（5/5 PASS，自动动态挂载 `cap.inchi`）
新增 `lib/inchi/{inchi,index}.cp`、`doc/lib/inchi.md` 与 `cap/test/test_inchi_production_deep.cp`（5/5 PASS），支持 1S/1 分层解析、Hill 多组分分子式迭代与 InChIKey 质子化偏移校验。

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

