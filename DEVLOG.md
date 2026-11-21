## 2026-11-22: 新建 `cap:nntp` NNTP (RFC 3977) status/GROUP/STAT/OVER parser, dot-unstuffing iter & cmd encoder（5/5 PASS，文档直写 `doc/lib/nntp.md`）
新增 cap:nntp 纯 CP 零分配网络新闻传输协议库（3 位状态码分类、GROUP/STAT/8 字段 OVER 概览、NntpMultiLineIter 解填充与命令防注入编码），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:whois` WHOIS (RFC 3912) query encoder, key/value field iter & multi-registry summary parser（5/5 PASS，文档直写 `doc/lib/whois.md`）
新增 cap:whois 纯 CP 零分配域名注册信息查询协议库（查询防注入编码、IANA refer 提取、ICANN/RIPE/ARIN 字段归一），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:pop3` POP3 (RFC 1939/2449) +OK/-ERR, [RESP-CODE], STAT/LIST/UIDL & dot-unstuffing codec（5/5 PASS，文档直写 `doc/lib/pop3.md`）
新增 cap:pop3 纯 CP 零分配邮局协议库（+OK/-ERR、[RESP-CODE]、STAT/LIST/UIDL、Pop3MultiLineIter 解填充与 dot-stuffing），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:irc` IRC (RFC 1459/2812/IRCv3) Message Tags, prefix/param zero-alloc parser & IrcWriter（5/5 PASS，文档直写 `doc/lib/irc.md`）
新增 cap:irc 纯 CP 零分配实时聊天协议库（IRCv3 @tags 零拷贝迭代与转义、nick!user@host 前缀、3 位数字回复与 IrcWriter），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:ftp` FTP (RFC 959/2428/3659) control commands, replies, PASV/EPSV & MLSD zero-alloc codec（5/5 PASS，文档直写 `doc/lib/ftp.md`）
新增 cap:ftp 纯 CP 零分配文件传输控制协议库（单/多行回复、PASV/EPSV 解析、MLSD Fact 零拷贝迭代与 PORT/EPRT 编码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:sflow` sFlow v5 datagram, Flow/Expanded Sample, RawPacketHeader & ExtendedSwitch zero-alloc parser（5/5 PASS，文档直写 `doc/lib/sflow.md`）
新增 cap:sflow 纯 CP 零分配网络采样遥测协议库（IPv4/IPv6 Agent 报头、Sample/Record 嵌套零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ipfix` IPFIX v10 (RFC 7011) Message/Set/Enterprise IE Template & varlen zero-alloc parser（5/5 PASS，文档直写 `doc/lib/ipfix.md`）
新增 cap:ipfix 纯 CP 零分配 IP 流信息导出协议库（16B 报头、Set/Template/Enterprise IE 零拷贝迭代与 §7 变长字段解码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:netflow` NetFlow v5 & v9 (RFC 3954) flow records, FlowSets & Template zero-alloc parser（5/5 PASS，文档直写 `doc/lib/netflow.md`）
新增 cap:netflow 纯 CP 零分配网络流遥测协议库（v5 48B 固定流记录与 v9 Template/Options/Data FlowSet 零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:rtcp` RTCP (RFC 3550) compound packet, SR/RR/SDES/BYE/APP & 24-bit signed loss zero-alloc parser（5/5 PASS，文档直写 `doc/lib/rtcp.md`）
新增 cap:rtcp 纯 CP 零分配实时传输控制协议库（复合包零拷贝迭代、24B ReportBlock 与 24-bit 有符号丢包数扩展、Padding 剥离），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:tftp` TFTP (RFC 1350/2347-2349) RRQ/WRQ/DATA/ACK/ERROR/OACK & option zero-alloc codec（5/5 PASS，文档直写 `doc/lib/tftp.md`）
新增 cap:tftp 纯 CP 零分配简单文件传输协议库（RRQ/WRQ/DATA/ACK/ERROR/OACK 解析、NUL 分隔选项零拷贝迭代与 TftpOptWriter），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:coap` CoAP (RFC 7252) Delta-encoded Option TLV, CoapWriter & payload zero-alloc codec（5/5 PASS，文档直写 `doc/lib/coap.md`）
新增 cap:coap 纯 CP 零分配物联网轻量 REST 协议库（4B 报头/Token/Delta 0..12/13/14 扩展编码与 CoapWriter 往返），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:ptp` PTPv2/v2.1 (IEEE 1588) 34B header, 48-bit timestamp, Announce & Delay_Resp zero-alloc codec（5/5 PASS，文档直写 `doc/lib/ptp.md`）
新增 cap:ptp 纯 CP 零分配精密时间协议库（34B 报头、带符号缩放纳秒 correction_field、10B 时间戳、Announce/Delay_Resp 与编码往返），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ntp` NTPv4/SNTP (RFC 5905) 48B codec, Unix/NTP epoch conversion & offset/RTT calculator（5/5 PASS，文档直写 `doc/lib/ntp.md`）
新增 cap:ntp 纯 CP 零分配网络时间协议库（48B 报文解析/编码往返、64-bit 固定点时间戳与 RFC 5905 时钟偏移/RTT 计算），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:dhcp` DHCPv4 (RFC 2131/2132) 240B header, Option TLV iter & DhcpWriter zero-alloc codec（5/5 PASS，文档直写 `doc/lib/dhcp.md`）
新增 cap:dhcp 纯 CP 零分配动态主机配置协议库（Magic Cookie 校验、PAD/END Option 零拷贝迭代与 DhcpWriter 编码往返），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:isis` IS-IS (ISO 10589 / RFC 1195) common/LSP header, TLV iter & Fletcher-16 checksum parser（5/5 PASS，文档直写 `doc/lib/isis.md`）
新增 cap:isis 纯 CP 零分配链路状态路由协议库（8B 通用头、27B LSP 报头、2B+len TLV 零拷贝迭代与 ISO 8473 Fletcher-16 校验和），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:pim` PIM-SM v2 (RFC 7761) Hello TLV, Register, Writer & IPv4/IPv6 checksum zero-alloc codec（5/5 PASS，文档直写 `doc/lib/pim.md`）
新增 cap:pim 纯 CP 零分配组播路由协议库（PIMv2 Hello 选项零拷贝迭代、Register 8B 校验和特例与 IPv4/IPv6 编码往返），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:l2tp` L2TPv2/v3 (RFC 2661/3931) Control/Data header & AVP TLV zero-alloc parser（5/5 PASS，文档直写 `doc/lib/l2tp.md`）
新增 cap:l2tp 纯 CP 零分配二层隧道协议解析库（L2TPv2 Control/Data 可选字段与 AVP 零拷贝迭代、L2TPv3 Control 报头），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:rip` RIPv1/v2 (RFC 2453) & RIPng (RFC 2080) RTE/Auth zero-alloc parser & encoder（5/5 PASS，文档直写 `doc/lib/rip.md`）
新增 cap:rip 纯 CP 零分配距离矢量路由协议库（RIPv1/v2 RTE/Simple Auth 与 RIPng IPv6 RTE 零拷贝迭代及编码往返），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:pppoe` PPPoE (RFC 2516) Discovery TLV & Session framing zero-alloc parser & writer（5/5 PASS，文档直写 `doc/lib/pppoe.md`）
新增 cap:pppoe 纯 CP 零分配以太网点对点隧道协议库（PADI/PADO/PADR/PADS/PADT TLV 零拷贝迭代、Session 封装与 PppoeWriter），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:eap` EAP (RFC 3748) & IEEE 802.1X EAPOL framing + Expanded Vendor TLV zero-alloc parser（5/5 PASS，文档直写 `doc/lib/eap.md`）
新增 cap:eap 纯 CP 零分配扩展认证协议解析库（EAPOL 帧、EAP Request/Response/Success/Failure 与 24-bit Vendor Expanded TLV），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:lacp` LACP (IEEE 802.1AX/802.3ad) 110B/124B LACPDU zero-alloc parser & encoder（5/5 PASS，文档直写 `doc/lib/lacp.md`）
新增 cap:lacp 纯 CP 零分配链路聚合控制协议库（Actor/Partner/Collector/Terminator TLV 解析、8 位状态判定与标准 110B/124B 编码往返），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:cdp` CDP v1/v2 (Cisco Discovery Protocol) TLV parser, writer & RFC 1071 checksum（5/5 PASS，文档直写 `doc/lib/cdp.md`）
新增 cap:cdp 纯 CP 零分配网络拓扑发现协议库（CDPv1/v2 报头、奇偶字节校验和、TLV 零拷贝迭代与 CdpWriter 编码），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:tacacs` TACACS+ (RFC 8907) 12B header, Authen/Author/Acct body & zero-copy arg iter parser（5/5 PASS，文档直写 `doc/lib/tacacs.md`）
新增 cap:tacacs 纯 CP 零分配 AAA 协议解析库（12B 报头/标志位与 6 类明文 Body/参数零拷贝迭代），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:vrrp` VRRPv2/v3 (RFC 3768/5798) header, IPv4/IPv6 virtual IP iter & checksum zero-alloc parser（5/5 PASS，文档直写 `doc/lib/vrrp.md`）
新增 cap:vrrp 纯 CP 零分配虚拟路由冗余协议解析库（VRRPv2/v3 报头、IPv4/IPv6 虚拟地址零拷贝迭代与伪首部校验和），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:mpls` MPLS (RFC 3032/4385/5586) label stack, PW Control Word & G-ACh zero-alloc codec（5/5 PASS，文档直写 `doc/lib/mpls.md`）
新增 cap:mpls 纯 CP 零分配多协议标签交换线协议库（20-bit 标签栈迭代/编码、特殊标签、G-ACh/PW-CW/IPv4/IPv6 嗅探），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:bfd` BFD (RFC 5880) 24B control packet & Simple/MD5/SHA1 Auth TLV zero-alloc parser（5/5 PASS，文档直写 `doc/lib/bfd.md`）
新增 cap:bfd 纯 CP 零分配双向转发检测协议解析库（24B 报头/标志位/诊断码/状态机与 3 类认证 TLV 零拷贝解析），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:ipsec` IPsec (RFC 4302 AH / RFC 4303 ESP / RFC 7296 IKEv2) zero-alloc wire parser（5/5 PASS，文档直写 `doc/lib/ipsec.md`）
新增 cap:ipsec 纯 CP 零分配安全隧道协议解析库（ESP/Trailer、AH ICV、IKEv2 28B 报头与链式 Payload 零拷贝迭代），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:ospf` OSPFv2 (RFC 2328) 24B header/checksum/Hello/LSA/LSU/LSAck zero-alloc parser（5/5 PASS，文档直写 `doc/lib/ospf.md`）
新增 cap:ospf 纯 CP 零分配网络路由协议解析库（OSPFv2 24B 报头、跳过 AuType/Auth 校验和、Hello/LSA/LSU/LSAck 零拷贝迭代），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:stp` 零分配 IEEE 802.1D/802.1w/802.1t STP & RSTP BPDU 编解码器（5/5 PASS，文档直写 `doc/lib/stp.md`）
实现零分配 STP/RSTP BPDU 编解码器，支持 Port Role 判定与 Proposal/Agreement/Forwarding 标志。

## 2026-11-22: 新建 `cap:igmp` 零分配 RFC 1112/2236/3376 IGMP v1/v2/v3 组播协议与校验和编解码器（5/5 PASS，文档直写 `doc/lib/igmp.md`）
实现零分配 IGMP v1/v2/v3 组播成员管理编解码器，支持源地址列表与 Aux Data 跳过。

## 2026-11-22: 新建 `cap:icmp` 零分配 RFC 792/4443 ICMPv4 & ICMPv6 报头与 RFC 1071 校验和编解码器（5/5 PASS，文档直写 `doc/lib/icmp.md`）
实现零分配 ICMPv4/v6 编解码器，精确处理奇数字节补零与防溢出校验和折叠。

## 2026-11-22: 新建 `cap:arp` 零分配 RFC 826/5227 以太网 IPv4 ARP 与变长地址解析编解码器（5/5 PASS，文档直写 `doc/lib/arp.md`）
实现零分配 ARP 地址解析协议编解码器，覆盖 RFC 5227 Probe 与 Gratuitous 判定。

## 2026-11-22: 新建 `cap:gre` 零分配 RFC 2784/2890/2637 GRE 隧道报头与 RFC 1071 校验和编解码器（5/5 PASS，文档直写 `doc/lib/gre.md`）
实现零分配 GRE 隧道封装编解码器，支持自动校验和回填与严格标志位边界校验。

## 2026-11-22: 新建 `cap:geneve` 零分配 RFC 8926 Geneve 通用网络虚拟化隧道与 TLV 选项编解码器（5/5 PASS，文档直写 `doc/lib/geneve.md`）
实现零分配 RFC 8926 Geneve 隧道编解码器，自动置位 C-flag 并严检保留位与 252B 选项上限。

## 2026-11-22: 新建 `cap:vxlan` 零分配 RFC 7348 VXLAN / VXLAN-GPE / VXLAN-GBP 隧道封装编解码器（5/5 PASS，文档直写 `doc/lib/vxlan.md`）
实现零分配 VXLAN/GPE/GBP 隧道封装编解码器，支持经典与扩展模式双向编码。

## 2026-11-22: 新建 `cap:lldp` 零分配 IEEE 802.1AB LLDP 链路层发现协议 TLV 编解码器（5/5 PASS，文档直写 `doc/lib/lldp.md`）
实现零分配 IEEE 802.1AB LLDP 编解码器，支持最长 511B TLV 与 End-of-LLDPDU 严格校验。

## 2026-11-22: 新建 `cap:snmp` 零分配 RFC 1157/3416 SNMP v1/v2c PDU 与 VarBind ASN.1 解析器（5/5 PASS，文档直写 `doc/lib/snmp.md`）
实现零分配 SNMP v1/v2c 解析器，严格拒绝不定长 BER、非最小 OID VLQ 与尾随垃圾。

## 2026-11-22: 新建 `cap:sctp` 零分配 RFC 4960 SCTP 报头、Castagnoli CRC32C 与 4B 对齐 Chunk 编解码器（5/5 PASS，文档直写 `doc/lib/sctp.md`）
实现零分配 RFC 4960 SCTP 编解码器，支持 4B 对齐 Chunk 迭代与自动 LE CRC32C 回填。

## 2026-11-22: 新建 `cap:diameter` 零分配 RFC 6733 Diameter AAA 协议报头与 4B 对齐 AVP 编解码器（5/5 PASS，文档直写 `doc/lib/diameter.md`）
实现零分配 RFC 6733 Diameter AAA 编解码器，严格校验版本、保留位与 4B 对齐边界。

## 2026-11-22: 新建 `cap:bgp` 零分配 RFC 4271 BGP-4 边界网关路由协议报头、NLRI 与路径属性解析器（5/5 PASS，文档直写 `doc/lib/bgp.md`）
实现零分配 BGP-4 路由协议解析器，严格校验 16B 全 0xFF Marker 与 /0..32 前缀压缩编码。

## 2026-11-22: 新建 `cap:pkcs12` 零分配 RFC 7292 PKCS#12 (.p12/.pfx) 容器、SafeBag 与 MacData DER 解析器（5/5 PASS，文档直写 `doc/lib/pkcs12.md`）
实现零分配 PKCS#12 PFX 容器解析器，严格校验 DER 规范并提取 localKeyId 与 friendlyName。

## 2026-11-22: 新建 `cap:radius` 零分配 RFC 2865/2866 RADIUS 认证计费协议 AVP 与 VSA 编解码器（5/5 PASS，文档直写 `doc/lib/radius.md`）
实现零分配 RADIUS 认证计费协议编解码器，覆盖 Cisco/Microsoft VSA 子 TLV 与严格长度校验。

## 2026-11-22: 新建 `cap:pkcs8` 零分配 RFC 5958 PKCS#8 私钥/加密私钥与 RFC 5280 SPKI 公钥 DER 解析器（5/5 PASS，文档直写 `doc/lib/pkcs8.md`）
实现零分配 PKCS#8/SPKI 容器解析器，支持剥离 Ed25519/X25519 内嵌 OCTET STRING 裸种子。

## 2026-11-22: 新建 `cap:jks` 零分配 Java KeyStore (.jks v1/v2) 密钥库与证书链解析器（5/5 PASS，文档直写 `doc/lib/jks.md`）
实现零分配 Java KeyStore 解析器，严格不越过尾部 20B SHA-1 摘要边界并支持按别名查找。

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

