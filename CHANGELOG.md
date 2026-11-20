# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:avro` Apache Avro Schema 驱动二进制编解码与 OCF 容器引擎（5/5 PASS，自动动态挂载 `cap.avro`）
- **新增标准库**：`lib/avro/{avro,index}.cp`，实现全类型 `AvroSchema`（含 `record`/`union`/`enum`/`fixed`/`array`/`map`）、ZigZag + Varint `encodeDatum`/`decodeDatum` 以及 OCF `"Obj\x01"` 容器 `writeContainer`/`readContainer`（含 16 字节 `sync_marker` 校验）。
- **新增文档与测试**：`doc/lib/avro.md` 扁平文档与 `cap/test/test_avro_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:tiff` Baseline TIFF 多页双端序与多压缩算法编解码引擎（5/5 PASS，自动动态挂载 `cap.tiff`）
- **新增标准库**：`lib/tiff/{tiff,index}.cp`，复用 `cap:lzw`（`MsbFirst`）与 `cap:deflate` 实现 `"II"`/`"MM"` 双端序多页 TIFF 编解码，支持 `None`/`PackBits`/`Lzw`/`Deflate` 压缩、`Predictor = 2` 水平差分还原、多 Strip 拼接及循环 IFD/`ImageTooLarge`/`TooManyPages` 防护。
- **新增文档与测试**：`doc/lib/tiff.md` 扁平文档与 `cap/test/test_tiff_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:gif` 纯 CP GIF87a/GIF89a 多帧动画编解码引擎（5/5 PASS，自动动态挂载 `cap.gif`）
- **新增标准库**：`lib/gif/{gif,index}.cp`，复用 `cap:lzw` 实现 GIF87a/89a 编解码，支持全局/局部调色板、Netscape 2.0 循环扩展、GCE 透明色/延时/Disposal、4-pass 隔行扫描及 `ImageTooLarge`/`TooManyFrames` 防护。
- **新增文档与测试**：`doc/lib/gif.md` 扁平文档与 `cap/test/test_gif_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:wkt` OGC WKT 与 WKB 空间几何双向编解码引擎（5/5 PASS，自动动态挂载 `cap.wkt`）
- **新增标准库**：`lib/wkt/{wkt,index}.cp`，实现大小写不敏感 `parseWkt`/`formatWkt`（支持 2D/Z/M/ZM、`EMPTY` 及两种 `MULTIPOINT` 写法）与 ISO WKB 1..7 大小端双向 `encodeWkb`/`decodeWkb`，内置 Polygon 环闭合与 32 层 `MaxDepthExceeded` 防护。
- **新增文档与测试**：`doc/lib/wkt.md` 扁平文档与 `cap/test/test_wkt_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:lzw` 可变位宽 LZW 字典无损压缩与解压引擎（5/5 PASS，自动动态挂载 `cap.lzw`）
- **新增标准库**：`lib/lzw/{lzw,index}.cp`，实现支持 `LsbFirst`（GIF）与 `MsbFirst`（TIFF/PDF）双位序的 2..12-bit 动态位宽升档、`ClearCode` 自动重置、`KwKwK` 解码及 `OutputTooLarge` 防炸弹上限。
- **新增文档与测试**：`doc/lib/lzw.md` 扁平文档与 `cap/test/test_lzw_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:stomp` 与 `cap:geojson` 纯 CP 标准库（均 5/5 PASS，自动动态挂载）
- **新增 `cap:stomp`**：`lib/stomp/{stomp,index}.cp`、`doc/lib/stomp.md`、`cap/test/test_stomp_production_deep.cp`（5/5 PASS），实现 STOMP 1.2 帧编解码、头部转义反转义、心跳 EOL 跳过、`content-length` 内嵌 `\x00` 载荷及 `BodyTooLarge` 防护。
- **新增 `cap:geojson`**：`lib/geojson/{geojson,index}.cp`、`doc/lib/geojson.md`、`cap/test/test_geojson_production_deep.cp`（5/5 PASS），实现 RFC 7946 GeoJSON 校验编解码、`computeBBox`、支持内环孔洞排除的 `pointInPolygon` 及迭代式 `simplifyDouglasPeucker` 抽稀。

## [2026-11-21] - 新建 `cap:imap` RFC 3501/9051 IMAP4 协议命令与 Literal 解析引擎（5/5 PASS，自动动态挂载 `cap.imap`）
- **新增标准库**：`lib/imap/{imap,index}.cp`，实现递增标签 `ImapCommandCodec`（防 `\r`/`\n`/`\0` 注入）、跨 Chunk `{N}`/`{N+}` Literal 增量解析器 `ImapStreamParser` 及 `parseCapability`/`parseSearchUids`/`parseExistsCount`/`parseFetchFlags` 语义辅助。
- **新增文档与测试**：`doc/lib/imap.md` 扁平文档与 `cap/test/test_imap_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:nats` NATS Wire 协议与 HPUB/HMSG 增量流式解析编码引擎（5/5 PASS，自动动态挂载 `cap.nats`）
- **新增标准库**：`lib/nats/{nats,index}.cp`，实现 `NatsEncoder`（`CONNECT`/`PUB`/`HPUB`/`SUB`/`UNSUB`/`PING`/`PONG`，拦截空白主题与 CRLF 注入）及跨 Chunk 半包黏包 `NatsParser`（支持 `INFO`/`MSG`/`HMSG` `NATS/1.0` 多值头与 `PayloadTooLarge` 防护）。
- **新增文档与测试**：`doc/lib/nats.md` 扁平文档与 `cap/test/test_nats_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ldap` RFC 4515 LDAP 搜索过滤器与 RFC 4514 DN 引擎（5/5 PASS，自动动态挂载 `cap.ldap`）
- **新增标准库**：`lib/ldap/{ldap,index}.cp`，实现 RFC 4515 过滤器 AST 解析（含十六进制 `\xx` 转义、`*` 通配符、`MaxDepthExceeded` 64 层深度防护）、防注入 `escapeFilterValue`、内存属性求值 `matchesEntry` 及 RFC 4514 `parseDn`。
- **新增文档与测试**：`doc/lib/ldap.md` 扁平文档与 `cap/test/test_ldap_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:amqp` AMQP 0-9-1 二进制协议帧与 FieldTable 引擎（5/5 PASS，自动动态挂载 `cap.amqp`）
- **新增标准库**：`lib/amqp/{amqp,index}.cp`，实现 AMQP 0-9-1 协议头与 `Method`/`Header`/`Body`/`Heartbeat` 帧编解码、嵌套 `FieldTable`（32 层 `MaxDepthExceeded` 防护）及半包/黏包增量拆帧器 `AmqpStreamDecoder`。
- **新增文档与测试**：`doc/lib/amqp.md` 扁平文档与 `cap/test/test_amqp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:http3` RFC 9114 HTTP/3 帧与 RFC 9204 QPACK 静态表引擎（5/5 PASS，自动动态挂载 `cap.http3`）
- **新增标准库**：`lib/http3/{http3,index}.cp`，实现 HTTP/3 全帧类型编解码（含 GREASE 未知帧透传、`FrameTooLarge` 上限与 HTTP/2 保留帧/保留设置拦截）及完整 99 项 RFC 9204 QPACK 静态表头部编解码。
- **新增文档与测试**：`doc/lib/http3.md` 扁平文档与 `cap/test/test_http3_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bmp` 纯 CP BMP 位图图像编解码引擎（5/5 PASS，自动动态挂载 `cap.bmp`）
- **新增标准库**：`lib/bmp/{bmp,index}.cp`，实现 24-bit RGB / 32-bit RGBA（4 字节行填充自动对齐）编解码，支持 `bottom-up`/`top-down` 负高度与 8-bit 调色板展开及 `ImageTooLarge` 尺寸上限防护。
- **新增文档与测试**：`doc/lib/bmp.md` 扁平文档与 `cap/test/test_bmp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:midi` Standard MIDI File 多轨音序编解码引擎（5/5 PASS，自动动态挂载 `cap.midi`）
- **新增标准库**：`lib/midi/{midi,index}.cp`，实现 SMF Format 0/1/2 多轨解析与编码，完整支持 1..4 字节 7-bit VLQ 变长时间戳、MIDI Running Status 复用/清除、`meta_tempo`/`sysex` 与 `ticksToMs` 换算。
- **新增文档与测试**：`doc/lib/midi.md` 扁平文档与 `cap/test/test_midi_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:quic` RFC 9000 QUIC 变长整数、包头与核心帧编解码引擎（5/5 PASS，自动动态挂载 `cap.quic`）
- **新增标准库**：`lib/quic/{quic,index}.cp`，实现 RFC 9000 §16 62-bit 变长整数、§17 `LongHeader`/`ShortHeader`（校验 DCID/SCID `<= 20B` 与 `0x40` 固定位）及 §19 `PADDING`/`PING`/`ACK`/`CRYPTO`/`STREAM`/`MAX_DATA`/`CONNECTION_CLOSE` 帧编解码。
- **新增文档与测试**：`doc/lib/quic.md` 扁平文档与 `cap/test/test_quic_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:wav` 纯 CP RIFF/WAVE PCM 音频容器编解码与重采样引擎（5/5 PASS，自动动态挂载 `cap.wav`）
- **新增标准库**：`lib/wav/{wav,index}.cp`，实现 8/16/24/32-bit PCM 与 IEEE Float32 归一化 `decode`/`encode`（支持未知块跳过与奇数字节 Chunk 对齐），以及 `toMono`、`resampleLinear`、`peakAmplitude`/`normalizePeak` 音频原语。
- **新增文档与测试**：`doc/lib/wav.md` 扁平文档与 `cap/test/test_wav_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:sip` RFC 3261 SIP 会话初始协议报文解析与构建引擎（5/5 PASS，自动动态挂载 `cap.sip`）
- **新增标准库**：`lib/sip/{sip,index}.cp`，实现 SIP/2.0 Request/Response 解析与格式化、RFC 3261 §7.3.3 紧凑头名映射（`v`/`f`/`t`/`i`/`m`/`c`/`l`/`s`/`k`）、多行折叠头、`Content-Length` 截断校验及防 `\r`/`\n` 头注入 `SipBuilder`。
- **新增文档与测试**：`doc/lib/sip.md` 扁平文档与 `cap/test/test_sip_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:png` 纯 CP PNG 图像无损编解码与 Paeth 滤波引擎（5/5 PASS，自动动态挂载 `cap.png`）
- **新增标准库**：`lib/png/{png,index}.cp`，复用 `cap:deflate` 实现 PNG 签名、逐块 CRC-32、5 种扫描行滤波编解码（含最小 SAD 自适应滤波选择）、多 `IDAT` 拼接与 `PLTE`+`tRNS` 调色板展开及 `ImageTooLarge` 尺寸上限防护。
- **新增文档与测试**：`doc/lib/png.md` 扁平文档与 `cap/test/test_png_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:http2` RFC 9113 HTTP/2 二进制帧编解码与流控引擎（5/5 PASS，自动动态挂载 `cap.http2`）
- **新增标准库**：`lib/http2/{http2,index}.cp`，实现全帧类型编解码（`DATA`/`HEADERS`/`SETTINGS`/`PING`/`GOAWAY`/`RST_STREAM`/`WINDOW_UPDATE` 等）、跨分块半包/黏包 `FrameStreamDecoder`、`Http2Stream` 状态机与 `FlowWindow` 双向流控窗口。
- **新增文档与测试**：`doc/lib/http2.md` 扁平文档与 `cap/test/test_http2_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:rtree` 2D 轴对齐边界框（AABB）R-Tree 空间索引引擎（5/5 PASS，自动动态挂载 `cap.rtree`）
- **新增标准库**：`lib/rtree/{rtree,index}.cp`，实现动态平衡 `RTree<V>`（Guttman `Quadratic Split`、MBR 维护与收紧）、`searchOverlap`、`searchPoint` 与 Best-First 分支界定 `kNearestPoint`。
- **新增文档与测试**：`doc/lib/rtree.md` 扁平文档与 `cap/test/test_rtree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:rtp` RFC 3550 RTP/RTCP 实时传输包编解码与 Jitter Buffer 引擎（5/5 PASS，自动动态挂载 `cap.rtp`）
- **新增标准库**：`lib/rtp/{rtp,index}.cp`，实现 RFC 3550 `RtpPacket`（支持 CSRC、Extension Header 与 Padding 对齐剥离）、`RtcpSenderReport`/`RtcpBye` 编解码，以及基于 `u16` 回绕差值 `seqDiff` 的乱序重排 `JitterBuffer`。
- **新增文档与测试**：`doc/lib/rtp.md` 扁平文档与 `cap/test/test_rtp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:sdp` RFC 8866 WebRTC SDP 会话描述解析与构建引擎（5/5 PASS，自动动态挂载 `cap.sdp`）
- **新增标准库**：`lib/sdp/{sdp,index}.cp`，实现 `SdpSession` 零拷贝解析与格式化、多 `MediaSection`、`rtpmaps()`、`candidates()`（ICE `host`/`srflx`/`relay`）及 `direction()` 提取。
- **新增文档与测试**：`doc/lib/sdp.md` 扁平文档与 `cap/test/test_sdp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:smtp` RFC 5321/5322 SMTP 协议编解码与 MIME 邮件构建引擎（5/5 PASS，自动动态挂载 `cap.smtp`）
- **新增标准库**：`lib/smtp/{smtp,index}.cp`，实现多行 SMTP 响应与增强状态码解析 `parseReply`、`SmtpCommandCodec`（`EHLO`/`AUTH PLAIN`/`MAIL FROM`/`RCPT TO`）、RFC 5321 `dotStuff`/`dotUnstuff` 以及支持 RFC 2047 主题与 Base64 附件的 `MailBuilder`（拦截 CR/LF 头注入）。
- **新增文档与测试**：`doc/lib/smtp.md` 扁平文档与 `cap/test/test_smtp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:stun` RFC 5389/8489 STUN NAT 穿透消息与完整性校验引擎（5/5 PASS，自动动态挂载 `cap.stun`）
- **新增标准库**：`lib/stun/{stun,index}.cp`，复用 `cap:deflate` `crc32` 实现 STUN 20 字节头与 4 字节对齐属性编解码、`XOR-MAPPED-ADDRESS`（IPv4/IPv6）、`ERROR-CODE`、`MESSAGE-INTEGRITY`（HMAC-SHA1）及 `FINGERPRINT`（CRC32 `^ 0x5354554e`）。
- **新增文档与测试**：`doc/lib/stun.md` 扁平文档与 `cap/test/test_stun_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bson` MongoDB BSON 1.1 二进制文档与 ObjectId 编解码引擎（5/5 PASS，自动动态挂载 `cap.bson`）
- **新增标准库**：`lib/bson/{bson,index}.cp`，实现完整小端序 BSON 1.1 标量/嵌套文档/数组编码解码、12 字节 `ObjectId`（`fromBytes`/`fromHex`/`toHex`/`generate`）、UTF-8 与 CString 校验及 64 层 `MaxDepthExceeded` 防护。
- **新增文档与测试**：`doc/lib/bson.md` 扁平文档与 `cap/test/test_bson_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:prometheus` Prometheus / OpenMetrics 指标注册表与文本编解码引擎（5/5 PASS，自动动态挂载 `cap.prometheus`）
- **新增标准库**：`lib/prometheus/{prometheus,index}.cp`，实现 `PromRegistry`（`Counter` / `Gauge` / `Histogram` 累计分桶 + `+Inf` + `_sum` + `_count` 与 `exportText`）及 `parseExposition` 文本反向解析，严格校验指标/标签名规则并支持 `\\`, `\"`, `\n` 转义往返。
- **新增文档与测试**：`doc/lib/prometheus.md` 扁平文档与 `cap/test/test_prometheus_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:socks5` RFC 1928/1929 SOCKS5 代理协议 Wire 编解码引擎（5/5 PASS，自动动态挂载 `cap.socks5`）
- **新增标准库**：`lib/socks5/{socks5,index}.cp`，实现客户端 Greeting / MethodChoice、RFC 1929 账密认证、IPv4/Domain/IPv6 请求回复及 UDP Associate 转发报文编解码与完整越界校验。
- **新增文档与测试**：`doc/lib/socks5.md` 扁平文档与 `cap/test/test_socks5_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:x509` 纯 CP X.509 v3 DER/PEM 证书解析与 SAN 校验引擎（5/5 PASS，自动动态挂载 `cap.x509`）
- **新增标准库**：`lib/x509/{x509,index}.cp`，复用 `cap:asn1` 与 `cap:pem` 实现 DER/PEM 证书解析、UTCTime/GeneralizedTime 换算、`isValidAt` 有效期校验、RFC 6125 通配符/IP SAN `matchesHostname`、`isIssuedBy` CA 链校验及 SHA-256 指纹计算。
- **新增文档与测试**：`doc/lib/x509.md` 扁平文档与 `cap/test/test_x509_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:zip` 纯 CP PKZIP 归档打包与安全解包引擎（5/5 PASS，自动动态挂载 `cap.zip`）
- **新增标准库**：`lib/zip/{zip,index}.cp`，复用 `cap:deflate` 实现 `ZipBuilder`（混合 `Stored`/`Deflated` 与完整 LFH/CD/EOCD 结构）及 `readArchive`/`readArchiveWithLimits`（校验 CRC-32 并拦截 `..`/前导 `/`/`\` 路径穿越与 `TooManyEntries`/`EntryTooLarge`）。
- **新增文档与测试**：`doc/lib/zip.md` 扁平文档与 `cap/test/test_zip_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:pem` RFC 7468 PEM 证书链与密钥块编解码引擎（5/5 PASS，自动动态挂载 `cap.pem`）
- **新增标准库**：`lib/pem/{pem,index}.cp`，实现单块/多块证书链解析（支持块间注释、RFC 1421 `Key: Value` 头字段、标签一致性与 `TooManyBlocks` 防护）以及 64 列规范 Base64 折行编码。
- **新增文档与测试**：`doc/lib/pem.md` 扁平文档与 `cap/test/test_pem_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:deflate` 纯 CP RFC 1951/1952/1950 DEFLATE/GZIP/ZLIB 无损压缩解压引擎（5/5 PASS，自动动态挂载 `cap.deflate`）
- **新增标准库**：`lib/deflate/{deflate,index}.cp`，纯 CP 实现 32KB 窗口 LZ77 + Fixed/Dynamic Huffman 解码、`crc32` / `adler32` 校验、Raw DEFLATE / GZIP（含 `FEXTRA`/`FNAME`/`FCOMMENT`/`FHCRC`）与 ZLIB 编解码及 `OutputTooLarge` 防炸弹上限。
- **新增文档与测试**：`doc/lib/deflate.md` 扁平文档与 `cap/test/test_deflate_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:multipart` RFC 7578 `multipart/form-data` 解析与安全构建引擎（5/5 PASS，自动动态挂载 `cap.multipart`）
- **新增标准库**：`lib/multipart/{multipart,index}.cp`，实现 `extractBoundary`、`parse`/`parseWithLimits`（支持二进制载荷、`Content-Disposition` 引号转义还原、缺失结束边界与 `TooManyParts`/`PartTooLarge` 防护）及防 CR/LF 头注入的 `MultipartBuilder`。
- **新增文档与测试**：`doc/lib/multipart.md` 扁平文档与 `cap/test/test_multipart_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:sse` W3C Server-Sent Events 增量流式解析与编码引擎（5/5 PASS，自动动态挂载 `cap.sse`）
- **新增标准库**：`lib/sse/{sse,index}.cp`，严格遵循 WHATWG/W3C `text/event-stream` 规范实现跨 Chunk 任意字节 `SseParser`（含 BOM 剥离、`\r`/`\n`/`\r\n` 识别、多行 `data:` 拼接、`Last-Event-ID` 追踪与 `EventBufferOverflow` 防护）及 `SseEncoder`。
- **新增文档与测试**：`doc/lib/sse.md` 扁平文档与 `cap/test/test_sse_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:kdtree` 多维空间 K-D Tree 最近邻与范围检索引擎（5/5 PASS，自动动态挂载 `cap.kdtree`）
- **新增标准库**：`lib/kdtree/{kdtree,index}.cp`，实现任意 `dims >= 1` 维度方差中位数平衡建树 `KdTree<V>` 与动态 `insert`，支持剪枝精确 `nearest` / `kNearest`、球形 `withinRadius` 与 AABB 超矩形 `rangeSearch`。
- **新增文档与测试**：`doc/lib/kdtree.md` 扁平文档与 `cap/test/test_kdtree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ndjson` NDJSON / JSON Lines 增量流式分帧与校验引擎（5/5 PASS，自动动态挂载 `cap.ndjson`）
- **新增标准库**：`lib/ndjson/{ndjson,index}.cp`，实现跨 Chunk 增量分帧解析器 `NdjsonStreamParser`（支持 `\n`/`\r\n`、空白行跳过、单行长度上限防护与错误行号定位）、`NdjsonWriter`（拦截裸 `\n`/`\r` 破坏行边界）及 `parseAll`/`stringifyAll`。
- **新增文档与测试**：`doc/lib/ndjson.md` 扁平文档与 `cap/test/test_ndjson_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:jsonpatch` RFC 6901 JSON Pointer 与 RFC 6902 JSON Patch 原子事务引擎（5/5 PASS，自动动态挂载 `cap.jsonpatch`）
- **新增标准库**：`lib/jsonpatch/{jsonpatch,index}.cp`，实现 RFC 6901 `~0`/`~1` 转义与前导零校验的 `resolvePointer`、隔离深拷贝 Arena 上的 RFC 6902 六操作原子 `applyPatch`（含 `CycleMove` 检测）以及最小差分 `createDiffPatch`。
- **新增文档与测试**：`doc/lib/jsonpatch.md` 扁平文档与 `cap/test/test_jsonpatch_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bitstream` MSB/LSB 位流与 ULEB128/SLEB128/Exp-Golomb 编码引擎（5/5 PASS，自动动态挂载 `cap.bitstream`）
- **新增标准库**：`lib/bitstream/{bitstream,index}.cp`，实现支持 `MsbFirst`/`LsbFirst` 双位序的 `BitWriter`/`BitReader`（含 `1..64` 位非对齐读写、`peekBits`、H.264/HEVC `Exp-Golomb` `ue(v)`/`se(v)`）及 `Leb128`（严格拦截 >10 字节溢出）。
- **新增文档与测试**：`doc/lib/bitstream.md` 扁平文档与 `cap/test/test_bitstream_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:cidr` IPv4/IPv6 CIDR 子网划分、超网聚合与 LPM 路由表引擎（5/5 PASS，自动动态挂载 `cap.cidr`）
- **新增标准库**：`lib/cidr/{cidr,index}.cp`，实现 `Ipv4Addr`/`Ipv6Addr`（RFC 5952 规范格式化）、`Cidr4`/`Cidr6`（主机位归一化、`splitSubnets` 与递归兄弟合并 `summarize`）及基于二进制基数树的 `LpmTable4<V>`。
- **新增文档与测试**：`doc/lib/cidr.md` 扁平文档与 `cap/test/test_cidr_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:suffixarray` 后缀数组、Kasai LCP 数组与全文本二分索引引擎（5/5 PASS，自动动态挂载 `cap.suffixarray`）
- **新增标准库**：`lib/suffixarray/{suffixarray,index}.cp`，实现 $O(N \log N)$ 倍增基数排序构建 `sa` 与 Kasai $O(N)$ `lcp` 数组，支持 $O(|P| \log N)$ 子串检索、`longestRepeatedSubstring` 与 `distinctSubstringCount`（含 `\x00` 二进制载荷）。
- **新增文档与测试**：`doc/lib/suffixarray.md` 扁平文档与 `cap/test/test_suffixarray_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:base85` Z85 / Adobe Ascii85 / RFC 1924 紧凑编码引擎（5/5 PASS，自动动态挂载 `cap.base85`）
- **新增标准库**：`lib/base85/{base85,index}.cp`，实现 ZeroMQ `Z85`、Adobe `Ascii85`（含全零 `'z'` 压缩与 `<~...~>` 帧）及 `Rfc1924`，严格拦截 32-bit `ValueOverflow`、非法字符与帧畸形。
- **新增文档与测试**：`doc/lib/base85.md` 扁平文档与 `cap/test/test_base85_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:diff3` 三方文本合并与冲突检测引擎（5/5 PASS，自动动态挂载 `cap.diff3`）
- **新增标准库**：`lib/diff3/{diff3,index}.cp`，实现 LCS/Myers 对齐三方自动融合、双边相同修改消歧、`Standard`/`Diff3`（含 `||||||| base`）冲突标记输出及 `\n`/`\r\n` 换行自动还原。
- **新增文档与测试**：`doc/lib/diff3.md` 扁平文档与 `cap/test/test_diff3_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:segtree` Fenwick 树状数组与惰性传播线段树范围聚合引擎（5/5 PASS，自动动态挂载 `cap.segtree`）
- **新增标准库**：`lib/segtree/{segtree,index}.cp`，实现 $O(N)$ 建树 `FenwickTree`（含树上倍增 `lowerBoundPrefix`）与支持 `rangeAdd`/`rangeAssign` 惰性标记合成及 `sum`/`min`/`max` 查询的 `LazySegmentTree`。
- **新增文档与测试**：`doc/lib/segtree.md` 扁平文档与 `cap/test/test_segtree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:dsu` 路径压缩、带权差分约束与可回溯撤销并查集引擎（5/5 PASS，自动动态挂载 `cap.dsu`）
- **新增标准库**：`lib/dsu/{dsu,index}.cp`，实现迭代两遍路径压缩 `DisjointSet`、差分约束 `WeightedDsu`（支持传递推导与 `ConflictConstraint` 拦截）以及支持 `snapshot`/`rollbackTo`/`undo` 的 `RollbackDsu`。
- **新增文档与测试**：`doc/lib/dsu.md` 扁平文档与 `cap/test/test_dsu_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:fuzzy` 编辑距离、Jaro-Winkler 与 fzf 风格模糊评分高亮引擎（5/5 PASS，自动动态挂载 `cap.fuzzy`）
- **新增标准库**：`lib/fuzzy/{fuzzy,index}.cp`，实现 Unicode 码点 `levenshtein` / `damerauLevenshtein`、`jaro` / `jaroWinkler`（`prefix_scale` 校验）、以及基于 DP 回溯的 `fuzzyScore` / `rankCandidates` 模糊打分与精确命中下标提取。
- **新增文档与测试**：`doc/lib/fuzzy.md` 扁平文档与 `cap/test/test_fuzzy_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:pqueue` 索引优先队列、流式 Top-K 与双堆实时中位数引擎（5/5 PASS，自动动态挂载 `cap.pqueue`）
- **新增标准库**：`lib/pqueue/{pqueue,index}.cp`，实现代际防 ABA `u32` 句柄 `IndexedMinHeap<V>`（支持 $O(\log N)$ 原地 `updatePriority` 与 `removeByHandle`）、$O(K)$ 内存 `BoundedTopK<V>` 与双堆 `StreamingMedian`。
- **新增文档与测试**：`doc/lib/pqueue.md` 扁平文档与 `cap/test/test_pqueue_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:interval` 区间集合代数与 AVL 增强区间树重叠检索引擎（5/5 PASS，自动动态挂载 `cap.interval`）
- **新增标准库**：`lib/interval/{interval,index}.cp`，实现半开区间 `[start, end)` 原语、`IntervalSet`（二分自动合并、`subtract` 挖洞分裂、`gaps` 空档提取、双指针 $O(M+N)$ `unionWith`/`intersectWith`/`differenceWith`）与 AVL 平衡增强 `IntervalTree<V>`。
- **新增文档与测试**：`doc/lib/interval.md` 扁平文档与 `cap/test/test_interval_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:jsonschema` JSON Schema 结构化校验与 `$ref` 路径诊断引擎（5/5 PASS，自动动态挂载 `cap.jsonschema`）
- **新增标准库**：`lib/jsonschema/{jsonschema,index}.cp`，实现 `SchemaValidator`（覆盖 `type` 严格 `integer`/`number` 区分、数值/UTF-8 码点长度/数组 `uniqueItems`/对象 `additionalProperties`/`enum`/`const`/`allOf`/`anyOf`/`oneOf`/`not`、本地 `#/$defs/...` `$ref` 解析与 64 层循环引用 `MaxDepthExceeded` 防护）。
- **新增文档与测试**：`doc/lib/jsonschema.md` 扁平文档与 `cap/test/test_jsonschema_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:skiplist` 有序跳表（SkipList）内存索引与范围扫描引擎（5/5 PASS，自动动态挂载 `cap.skiplist`）
- **新增标准库**：`lib/skiplist/{skiplist,index}.cp`，基于连续节点池与空闲槽回收实现泛型 `SkipList<V>`（支持 1..32 层高、`upsert`、`removeKey`、`lowerBound`、$O(1)$ `first`/`last` 与严格字典序 `rangeScan`）。
- **新增文档与测试**：`doc/lib/skiplist.md` 扁平文档与 `cap/test/test_skiplist_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:roaring` Roaring Bitmap 自适应压缩位图集合引擎（5/5 PASS，自动动态挂载 `cap.roaring`）
- **新增标准库**：`lib/roaring/{roaring,index}.cp`，实现高 16 位分块与 `ArrayContainer`（`<= 4096`）/ `BitsetContainer`（`> 4096`）双向自适应升降级、`addRange`、`rank`、完整集合代数运算及 `"ROAR"` 二进制序列化。
- **新增文档与测试**：`doc/lib/roaring.md` 扁平文档与 `cap/test/test_roaring_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:aho` Aho-Corasick 多模式自动机与敏感词替换引擎（5/5 PASS，自动动态挂载 `cap.aho`）
- **新增标准库**：`lib/aho/{aho,index}.cp`，实现 Trie + BFS Failure Link + Dictionary Suffix Link + 完整 DFA 状态转移表，支持重叠匹配 `findAll`、非重叠消歧 `LeftmostFirst`/`LeftmostLongest`、零分配短路 `containsAny`、`replaceAll` 与大小写不敏感模式。
- **新增文档与测试**：`doc/lib/aho.md` 扁平文档与 `cap/test/test_aho_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 升级加固 `cap:cbor` RFC 8949 CBOR 确定性编码与安全解码引擎（5/5 PASS，自动动态挂载 `cap.cbor`）
- **新增与重构**：在 `lib/cbor/{value,cbor,index}.cp` 实现标准 `CborValue`（Major Type 0..7、`f16`/`f32`/`f64`、Tag）、RFC 8949 Section 4.2 `encodeCanonical` 确定性键排序编码与 `decode`/`decodeWithMaxDepth` 安全解码（严格 UTF-8、AI 28..30、Break、`TrailingBytes` 与 `MaxDepthExceeded` 防护），同时保留 `Bin*` 兼容导出。
- **更新文档与测试**：`doc/lib/cbor.md` 扁平文档与 `cap/test/test_cbor_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:otp` RFC 4226 HOTP / RFC 6238 TOTP 与 `otpauth://` URI 认证引擎（5/5 PASS，自动动态挂载 `cap.otp`）
- **新增标准库**：`lib/otp/{otp,index}.cp`，实现 RFC 4648 Base32 编解码、HOTP/TOTP（支持 `SHA1`/`SHA256`/`SHA512` 与 `6..8` 位前导零格式化）、恒定时间 `verifyTotp` 窗口漂移校验与防重放 `matched_step`，以及 `otpauth://` URI 格式化/解析。
- **新增文档与测试**：`doc/lib/otp.md` 扁平文档与 `cap/test/test_otp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:lru` O(1) LRU 缓存淘汰、TTL 过期与字节权重配额引擎（5/5 PASS，自动动态挂载 `cap.lru`）
- **新增标准库**：`lib/lru/{lru,index}.cp`，基于哈希表与连续槽位侵入式双向链表实现严格 $O(1)$ `LruCache<V>`，支持条目数与 `max_weight` 双上限淘汰、确定性 `now_ms` TTL 过期、`purgeExpired`、`resize` 与 `hitRate` 统计。
- **新增文档与测试**：`doc/lib/lru.md` 扁平文档与 `cap/test/test_lru_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 升级加固 `cap:protobuf` Protocol Buffers Wire 格式动态编解码引擎（5/5 PASS，自动动态挂载 `cap.protobuf`）
- **新增与重构**：`lib/protobuf/protobuf.cp` 实现 `ProtoWriter` / `ProtoReader`（Varint、ZigZag `sint32`/`sint64`、Fixed32/64、Packed Repeated、零拷贝切片访问、`skipField` 及 64 层嵌套 `MaxDepthExceeded` 防护），并与 `grpc.cp` 经 `index.cp` 统一导出。
- **更新文档与测试**：`doc/lib/protobuf.md` 扁平文档与 `cap/test/test_protobuf_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:snappy` Snappy Block Format 高速无损压缩与解压引擎（5/5 PASS，自动动态挂载 `cap.snappy`）
- **新增标准库**：`lib/snappy/{snappy,index}.cp`，实现 Snappy Block Format 14-bit 哈希表压缩、Literal/Copy-1/Copy-2/Copy-4 解压与 RLE 重叠拷贝，并内置 `InvalidOffset` 与 `decompressWithMaxSize` 资源耗尽防护。
- **新增文档与测试**：`doc/lib/snappy.md` 扁平文档与 `cap/test/test_snappy_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ringhash` 一致性哈希环、Jump Hash 与 Rendezvous 负载均衡引擎（5/5 PASS，自动动态挂载 `cap.ringhash`）
- **新增标准库**：`lib/ringhash/{ringhash,index}.cp`，实现加权虚拟节点二分查找 `ConsistentRing`（含物理节点严格去重 `getReplicas`）、$O(\ln N)$ 无状态 `jumpConsistentHash` 及 `RendezvousHasher`（HRW）。
- **新增文档与测试**：`doc/lib/ringhash.md` 扁平文档与 `cap/test/test_ringhash_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:base58` Base58 / Base58Check 与 Bech32/Bech32m 编码校验引擎（5/5 PASS，自动动态挂载 `cap.base58`）
- **新增标准库**：`lib/base58/{base58,index}.cp`，实现 Bitcoin 字母表 `Base58`（保留前导 `0x00` <-> `'1'`）、Double-SHA256 `encodeCheck`/`decodeCheck` 以及 BIP-173 Bech32 / BIP-350 Bech32m 多态检错编解码与 `convertBits`。
- **新增文档与测试**：`doc/lib/base58.md` 扁平文档与 `cap/test/test_base58_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 升级加固 `cap:expr` 安全业务规则与公式表达式求值引擎（5/5 PASS，自动动态挂载 `cap.expr`）
- **新增与重构**：`lib/expr/expr.cp` 替代旧 `value.cp`/`eval.cp`，实现完整词法与递归下降 AST 解析器、短路 `&&`/`||`、惰性三元条件、`ExprContext` 变量绑定、10 个纯内置函数及默认 64 层递归深度防护（`MaxDepthExceeded`）。
- **更新文档与测试**：`doc/lib/expr.md` 扁平文档与 `cap/test/test_expr_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 升级加固 `cap:fsm` 确定性有限状态机 DFA、守卫回调与 GraphViz DOT 引擎（5/5 PASS，自动动态挂载 `cap.fsm`）
- **新增与重构**：`lib/fsm/fsm.cp` 替代旧 `state_machine.cp`，实现 `StateMachine`（`GuardFn` 守卫、`onEnter`/`onExit`/`onTransition` 生命周期钩子、`history` 轨迹、BFS `reachableStates`/`findDeadEndStates` 死胡同检测与 `toDot` GraphViz 导出），并与 `circuit_breaker.cp`/`dag.cp` 经 `index.cp` 统一导出。
- **更新文档与测试**：`doc/lib/fsm.md` 扁平文档与 `cap/test/test_fsm_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:wal` CRC32C 校验预写日志（WAL）帧编解码与崩溃恢复引擎（5/5 PASS，自动动态挂载 `cap.wal`）
- **新增标准库**：`lib/wal/{wal,index}.cp`，实现固定 24 字节小端序 `"WAL1"` 帧头与 Castagnoli CRC32C 校验、`recoverStrict` / `recoverWithTailTruncation` 掉电尾部截断恢复以及 `compactBefore` 日志压缩。
- **新增文档与测试**：`doc/lib/wal.md` 扁平文档与 `cap/test/test_wal_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:hll` HyperLogLog 亚千字节超大规模基数近似估算引擎（5/5 PASS，自动动态挂载 `cap.hll`）
- **新增标准库**：`lib/hll/{hll,index}.cp`，支持精度 `4..16`、LinearCounting 小基数修正与 64-bit 大基数修正、无损分片 `merge` 以及 `"HLL1"` 魔数头 `serialize`/`deserialize` 持久化。
- **新增文档与测试**：`doc/lib/hll.md` 扁平文档与 `cap/test/test_hll_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:msgpack` MessagePack 二进制紧凑编解码与深度安全防护引擎（5/5 PASS，自动动态挂载 `cap.msgpack`）
- **新增标准库**：`lib/msgpack/{msgpack,index}.cp`，实现全规格 MessagePack 自动最小编码 `MsgPackWriter` 与零拷贝切片解码 `MsgPackReader`，支持 `fixint`/`int`/`uint`/`float`/`str`/`bin`/`array`/`map`/`ext` 并严格防御保留标签 `0xc1`、`TrailingBytes` 与超深嵌套。
- **新增文档与测试**：`doc/lib/msgpack.md` 扁平文档与 `cap/test/test_msgpack_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ratelimit` 多算法确定性限流与配额控制引擎（5/5 PASS，自动动态挂载 `cap.ratelimit`）
- **新增标准库**：`lib/ratelimit/{ratelimit,index}.cp`，实现 `TokenBucket`、`SlidingWindowCounter`（双窗平滑加权防边界毛刺）、`GcraLimiter`（O(1) 单 `tat_ms` 状态）及 `KeyedRateLimiter`（多租户隔离与 `evictIdle` 过期淘汰）。
- **新增文档与测试**：`doc/lib/ratelimit.md` 扁平文档与 `cap/test/test_ratelimit_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:nanoid` NanoID / 单调 ULID / RFC 9562 UUID v7 分布式短 ID 引擎（5/5 PASS，自动动态挂载 `cap.nanoid`）
- **新增标准库**：`lib/nanoid/{nanoid,index}.cp`，实现无模数偏差拒绝采样 `NanoId`、Crockford Base32 编码及同毫秒 80-bit 单调递增 `MonotonicUlidGenerator`、以及 RFC 9562 位布局 `UuidV7` 生成与校验。
- **新增文档与测试**：`doc/lib/nanoid.md` 扁平文档与 `cap/test/test_nanoid_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:tar` POSIX ustar 内存归档打包与安全解包引擎（5/5 PASS，自动动态挂载 `cap.tar`）
- **新增标准库**：`lib/tar/{tar,index}.cp`，实现 POSIX.1-1988 ustar 512 字节头部、自动 155+100 长路径 `prefix` 拆分、八进制校验和与双零 EOF 块，`TarReader` 默认拦截 `..` 与绝对路径穿越。
- **新增文档与测试**：`doc/lib/tar.md` 扁平文档与 `cap/test/test_tar_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:globset` 高性能多模式 Glob 路径匹配集合与 `.gitignore` 规则引擎（5/5 PASS，自动动态挂载 `cap.globset`）
- **新增标准库**：`lib/globset/{glob,globset,index}.cp`，实现线性 NFA `GlobMatcher`（支持 `*`/`**`/`a/**/b`/`?`/`[...]`/`[!...]`）、支持嵌套大括号展开的 `GlobSetBuilder`/`GlobSet` 与 `.gitignore` 语义 `IgnoreMatcher`。
- **新增文档与测试**：`doc/lib/globset.md` 扁平文档与 `cap/test/test_globset_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:cron` 5/6 字段 Cron 表达式解析与调度计算引擎（5/5 PASS，自动动态挂载 `cap.cron`）
- **新增标准库**：`lib/cron/{cron,index}.cp`，实现位掩码 `CronSchedule`、Howard Hinnant O(1) 纯 UTC 日历换算、5/6 字段与 `@hourly`/`@daily` 等宏别名解析、Vixie `dom`/`dow` OR/AND 语义及跳跃加速 `nextAfter`/`prevBefore`/`nextN`。
- **新增文档与测试**：`doc/lib/cron.md` 扁平文档与 `cap/test/test_cron_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:crdt` 分布式无冲突复制数据类型（CvRDT）引擎（5/5 PASS，自动动态挂载 `cap.crdt`）
- **新增标准库**：`lib/crdt/{vclock,counters,register,index}.cp`，实现 `VectorClock`、`GCounter`、`PnCounter`、`LwwRegister` 与 `OrSet`（并发 add-wins 与 remove 后重新 add），严格满足半格合并交换律、结合律与幂等律。
- **新增文档与测试**：`doc/lib/crdt.md` 扁平文档与 `cap/test/test_crdt_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:circuit` 分布式弹性断路器、退避抖动与舱壁限流引擎（5/5 PASS，自动动态挂载 `cap.circuit`）
- **新增标准库**：`lib/circuit/{breaker,policy,index}.cp`，实现环形滑动失败率窗口 `CircuitBreaker`、`BackoffPolicy`（`None`/`Full`/`Equal`/`Decorrelated` 确定性抖动）及 `Bulkhead` 并发舱壁限流器。
- **新增文档与测试**：`doc/lib/circuit.md` 扁平文档与 `cap/test/test_circuit_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:fx` 定点多币种金融货币运算与外汇三角换算引擎（5/5 PASS，自动动态挂载 `cap.fx`）
- **新增标准库**：`lib/fx/{money,rates,index}.cp`，实现 ISO-4217 货币小数位注册、256 位中间精度 `Money.mulScale`、Hamilton 最大余数零分币丢失 `allocate`、5 种舍入模式（含 `HalfEven`）以及 `FxBook` 直连/逆向/1 跳三角汇率路由兑换。
- **新增文档与测试**：`doc/lib/fx.md` 扁平文档与 `cap/test/test_fx_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:geohash` 零分配 Geohash 空间网格与 Haversine 距离引擎（5/5 PASS，自动动态挂载 `cap.geohash`）
- **新增标准库**：`lib/geohash/{geohash,index}.cp`，实现栈上定长 `GeohashCode`、`encode`/`decode`/`decodeBounds`、8 邻域 `neighbors`（含 `±180°` 国际日界线环绕与极点钳位）、64-bit Morton 整型 `encodeInt`/`decodeInt` 及 `haversineMeters`/`precisionForRadiusMeters`。
- **新增文档与测试**：`doc/lib/geohash.md` 扁平文档与 `cap/test/test_geohash_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:radix` 压缩基数树与高性能分段前缀路由引擎（5/5 PASS，自动动态挂载 `cap.radix`）
- **新增标准库**：`lib/radix/{radix,router,index}.cp`，实现泛型压缩基数树 `RadixTree<T>`（公共前缀分裂、删除单子节点自动压缩合并、`longestPrefix`、`scanPrefix`）与 `RadixRouter<T>`（静态/`:param`/`*wildcard` 优先级路由）。
- **新增文档与测试**：`doc/lib/radix.md` 扁平文档与 `cap/test/test_radix_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:xorfilter` 极省空间静态 Xor8 / Xor16 近似集合过滤器（5/5 PASS，自动动态挂载 `cap.xorfilter`）
- **新增标准库**：`lib/xorfilter/{xorfilter,index}.cp`，实现 `Xor8Filter`（~9.84 bits/key）与 `Xor16Filter`（~19.68 bits/key）3-peeling 构建、$O(1)$ 三次访存 `contains`、64-bit 哈希直通接口及 `serialize`/`deserialize` 二进制持久化。
- **新增文档与测试**：`doc/lib/xorfilter.md` 扁平文档与 `cap/test/test_xorfilter_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:hpack` RFC 7541 HTTP/2 HPACK 头部压缩编解码引擎（5/5 PASS，自动动态挂载 `cap.hpack`）
- **新增标准库**：`lib/hpack/{huffman,table,codec,index}.cp`，实现 RFC 7541 Appendix A 61 项静态表、O(1) 环形 FIFO 动态表、Appendix B 256+EOS 规范 Huffman 编解码、变长前缀整数以及 `HpackEncoder`/`HpackDecoder`。
- **新增文档与测试**：`doc/lib/hpack.md` 扁平文档与 `cap/test/test_hpack_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:syslog` RFC 5424 / RFC 3164 结构化 Syslog 解析与格式化引擎（5/5 PASS，自动动态挂载 `cap.syslog`）
- **新增标准库**：`lib/syslog/{types,parser,formatter,index}.cp`，实现 RFC 5424 与 RFC 3164 自动识别解析、PRI 0..191 校验、NILVALUE `-`、UTF-8 BOM 剥离及多 `SD-ELEMENT` 转义解析/序列化。
- **新增文档与测试**：`doc/lib/syslog.md` 扁平文档与 `cap/test/test_syslog_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:merkle` RFC 6962 域分离 Merkle 哈希树与双树差异定位引擎（5/5 PASS，自动动态挂载 `cap.merkle`）
- **新增标准库**：`lib/merkle/{merkle,index}.cp`，实现 RFC 6962 域分离 `0x00`/`0x01` SHA-256 哈希、非 2 次幂奇数节点提升、`generateProof`、`verifyProof` 与 $O(k \log N)$ 跨规模 `diffTrees` 差异定位。
- **新增文档与测试**：`doc/lib/merkle.md` 扁平文档与 `cap/test/test_merkle_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:cuckoo` 支持删除的布谷鸟过滤器（5/5 PASS，自动动态挂载 `cap.cuckoo`）
- **新增标准库**：`lib/cuckoo/{cuckoo,index}.cp`，实现 4 槽位/桶、16-bit 非零指纹、对合备选桶计算、有界踢出（`MAX_KICKS = 500`）及栈上逆向无损回滚，支持 `insert`、`insertUnique`、`contains`、`delete`、`clear`、`loadFactor`、`stats`。
- **新增文档与测试**：`doc/lib/cuckoo.md` 扁平文档与 `cap/test/test_cuckoo_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:mqtt` MQTT 3.1.1 二进制 Wire 报文编解码与流式拆包引擎（5/5 PASS，自动动态挂载 `cap.mqtt`）

- 新建 [`lib/mqtt/`](file:///data/dev/cp/lib/mqtt/index.cp)（`packet.cp`、`index.cp`）：`MqttCodec`（1..4 字节 RemainingLength Varint 含第 4 字节 `0x80` 拦截、全 14 类控制报文编解码及 `QoS > 0`/`PUBREL` 标志位严格校验）及 `MqttStreamDecoder`（半包等待、多报文黏包拆分与 `PacketTooLarge` 防护，零泄漏）；文档 [`doc/lib/mqtt.md`](file:///data/dev/cp/doc/lib/mqtt.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_mqtt_production_deep.cp`](file:///data/dev/cp/cap/test/test_mqtt_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:paseto` PASETO v4.local / v4.public 安全令牌与 PAE 认证引擎（5/5 PASS，自动动态挂载 `cap.paseto`）

- 新建 [`lib/paseto/`](file:///data/dev/cp/lib/paseto/index.cp)（`paseto.cp`、`index.cp`）：`paeEncode`（LE64 无歧义前置认证编码）及 `PasetoV4`（`v4.public` Ed25519 签名验签免疫 `alg:none`、`v4.local` BLAKE3 密钥派生 + XChaCha20-Poly1305 AEAD 加密、`Footer` & `Implicit Assertion` 及 `verifyClaims` `exp`/`nbf`/`iss`/`aud`/`sub` 校验，零泄漏）；文档 [`doc/lib/paseto.md`](file:///data/dev/cp/doc/lib/paseto.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_paseto_production_deep.cp`](file:///data/dev/cp/cap/test/test_paseto_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:lsm` LSM-Tree MemTable / SSTable 与多路归并压缩存储引擎（5/5 PASS，自动动态挂载 `cap.lsm`）

- 新建 [`lib/lsm/`](file:///data/dev/cp/lib/lsm/index.cp)（`lsm.cp`、`index.cp`）：`SsTable`（有序 Data Block、每 16 条采样稀疏 Index Block 与 `"CPS1"`+CRC32 篡改校验）及 `LsmEngine`（`MemTable` 阈值自动 `flush`、Tombstone 墓碑不穿透、`rangeScan` 多路最新 `seq` 归并与 `compact` 全量空间回收，零泄漏）；文档 [`doc/lib/lsm.md`](file:///data/dev/cp/doc/lib/lsm.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_lsm_production_deep.cp`](file:///data/dev/cp/cap/test/test_lsm_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:tlv` 二进制 TLV 编解码、流式分帧与 CRC32C 校验引擎（5/5 PASS，自动动态挂载 `cap.tlv`）

- 新建 [`lib/tlv/`](file:///data/dev/cp/lib/tlv/index.cp)（`tlv.cp`、`index.cp`）：`crc32c`（Castagnoli 查找表）、`TlvWriter`/`TlvReader`（`u16` Tag + `u32` Length 32 层嵌套容器）及 `LengthPrefixedFramer`（`U16Be`/`U32Be`/`U32Le` 半包黏包拆帧、DoS 上限防护与可选尾随 CRC32C 校验，零泄漏）；文档 [`doc/lib/tlv.md`](file:///data/dev/cp/doc/lib/tlv.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_tlv_production_deep.cp`](file:///data/dev/cp/cap/test/test_tlv_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:sheet`（5/5）与 `cap:tz`（5/5）双标准库落地（自动动态挂载 `cap.sheet` / `cap.tz`）

- **`cap:sheet`**（[`lib/sheet/`](file:///data/dev/cp/lib/sheet/index.cp)）：`CellCoord`（`A1`..`ZZ9999` 双向转换）与 `Spreadsheet`（四则/比较、`SUM`/`AVG`/`MIN`/`MAX`/`COUNT`/`IF`/`ROUND` 公式、Kahn 拓扑排序级联重算、`CycleDetected` 环路拦截与 RFC-4180 `toCsv`，零泄漏），文档 [`doc/lib/sheet.md`](file:///data/dev/cp/doc/lib/sheet.md)，`-m` 5/5 PASS；
- **`cap:tz`**（[`lib/tz/`](file:///data/dev/cp/lib/tz/index.cp)）：`Timezone`（Howard Hinnant $O(1)$ 历法算法、20+ 内置 IANA 时区、POSIX TZ 南北半球跨年 DST 规则解析、`toLocal`/`toUnix`/`formatRfc3339`），文档 [`doc/lib/tz.md`](file:///data/dev/cp/doc/lib/tz.md)，`-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-21] - 新建 `cap:ssh` OpenSSH 公钥 Wire 指纹与 `authorized_keys` 策略引擎（5/5 PASS，自动动态挂载 `cap.ssh`）

- 新建 [`lib/ssh/`](file:///data/dev/cp/lib/ssh/index.cp)（`keys.cp`、`index.cp`）：`SshPublicKey`（复用 `cap:crypto` & `cap:encoding`，支持 Ed25519/RSA/ECDSA-P256/SK-Ed25519、RFC 4251 长度前缀 Wire 校验、内外算法一致性校验与标准 `SHA256:<base64>` 指纹）及 `AuthorizedKeys`（多行选项解析含引号内逗号/转义引号与 `matchKey`，零泄漏）；文档 [`doc/lib/ssh.md`](file:///data/dev/cp/doc/lib/ssh.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_ssh_production_deep.cp`](file:///data/dev/cp/cap/test/test_ssh_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:rule` 声明式生产规则、优先级与互斥组决策引擎（5/5 PASS，自动动态挂载 `cap.rule`）

- 新建 [`lib/rule/`](file:///data/dev/cp/lib/rule/index.cp)（`rule.cp`、`index.cp`）：`FactMap`、`Condition`（`And`/`Or`/`Not`/`Compare`/`Exists`/`InList`，支持 `Int`/`Float` 跨类型比较与 32 层深度防护）及 `RuleEngine`（按 `salience` 降序稳定评估、`activation_group` 互斥短路与 `RuleEvalReport` 审计统计，零泄漏）；文档 [`doc/lib/rule.md`](file:///data/dev/cp/doc/lib/rule.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_rule_production_deep.cp`](file:///data/dev/cp/cap/test/test_rule_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 热修 `cpc` 保留 C ABI `export fn` / `export const` 关键字（`task-bun-20261121-export-fn-const`）

- 拆分 [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs) `Token::Export` 与 `Token::Pub`，为 [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) `VarDecl` 新增 `is_export` 并打通 [`parser/`](file:///data/dev/cp/cpc/src/parser/decl.rs) 与 [`codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)，使 `export fn` / `pub export fn` / `@c pub export fn` / `export const` 正确输出 `export`；已执行 `sync_cpc.sh release` 原子发布，经 [`repro_export_fn_const.cp`](file:///data/dev/cp/temp/repro_export_fn_const.cp) + `zig ast-check` 全绿验证并结单通知 `bun_agy`。

---

## [2026-11-21] - 新建 `cap:http_cache` RFC 9111 HTTP 缓存语义与强弱 `ETag` 条件协商引擎（5/5 PASS，自动动态挂载 `cap.http_cache`）

- 新建 [`lib/http_cache/`](file:///data/dev/cp/lib/http_cache/index.cp)（`cache_policy.cp`、`index.cp`）：`CacheControl`（全指令解析与往返 `toHeader`）、`EntityTag`（强弱比较、`If-None-Match` 弱比较 304 与 `If-Match` 强比较 412 防并发覆盖）及 `HttpCacheEvaluator`（`s-maxage`>`max-age` 新鲜度评估、`private`/`no-store` 与 `buildVaryKey`，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/http_cache.md`](file:///data/dev/cp/doc/lib/http_cache.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_http_cache_production_deep.cp`](file:///data/dev/cp/cap/test/test_http_cache_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:dedup` SimHash / MinHash / LSH 近重复文档去重引擎（5/5 PASS，自动动态挂载 `cap.dedup`）

- 新建 [`lib/dedup/`](file:///data/dev/cp/lib/dedup/index.cp)（`dedup.cp`、`index.cp`）：`SimHash`（FNV-1a+SplitMix64 64-bit 指纹与 `@popCount` 海明距离）、`SimHashIndex`（4x16-bit 抽屉分片桶 $k \le 3$ 鸽巢精确召回）、`MinHash` & `LshIndex`（$k$-shingle Jaccard 近似估算与分桶检索，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/dedup.md`](file:///data/dev/cp/doc/lib/dedup.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_dedup_production_deep.cp`](file:///data/dev/cp/cap/test/test_dedup_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:highlight` 八语言词法高亮与 ANSI/HTML 渲染引擎（5/5 PASS，自动动态挂载 `cap.highlight`）

- 新建 [`lib/highlight/`](file:///data/dev/cp/lib/highlight/index.cp)（`highlight.cp`、`index.cp`）：`Highlighter`（覆盖 CP/Zig/Rust/TypeScript/Python/JSON/SQL/Bash 八大语言无损 `tokenize`、`toAnsi` 终端着色与 `AnsiColorCodec.stripAnsi` 还原、`toHtml` 严格转义防 XSS，零泄漏）；文档 [`doc/lib/highlight.md`](file:///data/dev/cp/doc/lib/highlight.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_highlight_production_deep.cp`](file:///data/dev/cp/cap/test/test_highlight_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:vcard` RFC 6350/2426 vCard 3.0 & 4.0 电子名片解析与生成引擎（5/5 PASS，自动动态挂载 `cap.vcard`）

- 新建 [`lib/vcard/`](file:///data/dev/cp/lib/vcard/index.cp)（`vcard.cp`、`index.cp`）：`VCardContact` & `VCardCodec`（75 字节行折行展开与 UTF-8 安全折行生成、`group.` 属性组前缀剥离、结构化 `N`/`ADR` 转义分号切分、多值 `TEL`/`EMAIL` `TYPE=` 提取、`MissingFn`/`UnbalancedCard` 校验，零泄漏）；文档 [`doc/lib/vcard.md`](file:///data/dev/cp/doc/lib/vcard.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_vcard_production_deep.cp`](file:///data/dev/cp/cap/test/test_vcard_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:sat` DPLL 布尔 SAT 求解器与声明式约束建模引擎（5/5 PASS，自动动态挂载 `cap.sat`）

- 新建 [`lib/sat/`](file:///data/dev/cp/lib/sat/index.cp)（`solver.cp`、`index.cp`）：`SatSolver`（1-based 编号、DPLL 单位传播 + 纯文字消除 + MOM 启发式分支、标准 `parseDimacs` 与 100% 子句校验）及 `ConstraintModel`（`implies`/`conflicts`/`atMostOne`/`exactlyOne`/`solveNamed`，实测鸽巢 `PHP(4,3)` UNSAT、包依赖冲突与 4 皇后求解，零泄漏）；文档 [`doc/lib/sat.md`](file:///data/dev/cp/doc/lib/sat.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_sat_production_deep.cp`](file:///data/dev/cp/cap/test/test_sat_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:tween` 31 种缓动曲线、关键帧轨道与弹簧阻尼动画引擎（5/5 PASS，自动动态挂载 `cap.tween`）

- 新建 [`lib/tween/`](file:///data/dev/cp/lib/tween/index.cp)（`tween.cp`、`index.cp`）：`Easing`（全 31 种 Robert Penner 缓动曲线）、`Tween`（往返 `yoyo` 与重复计数）、`KeyframeTrack`（严格单调检验与二分分段插值）及 `SpringDamper`（稳定性受控自适应子步长半隐式欧拉物理系统）；文档 [`doc/lib/tween.md`](file:///data/dev/cp/doc/lib/tween.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_tween_production_deep.cp`](file:///data/dev/cp/cap/test/test_tween_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:noise` 确定性 Perlin/Worley/fBm 分形噪声与高度图引擎（5/5 PASS，自动动态挂载 `cap.noise`）

- 新建 [`lib/noise/`](file:///data/dev/cp/lib/noise/index.cp)（`noise.cp`、`index.cp`）：`NoiseGen`（SplitMix64 + Fisher-Yates 零堆 512 排列置换表、Quintic Perlin 2D/3D、`worley2` Voronoi 细胞噪声、归一化 `fbm2`/`ridged2` 分形及 `generateGrid2D` 批量 `[0.0, 1.0]` 高度图）；文档 [`doc/lib/noise.md`](file:///data/dev/cp/doc/lib/noise.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_noise_production_deep.cp`](file:///data/dev/cp/cap/test/test_noise_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:spell` SymSpell 对称删除、BK-Tree 与 Soundex 拼写纠错引擎（5/5 PASS，自动动态挂载 `cap.spell`）

- 新建 [`lib/spell/`](file:///data/dev/cp/lib/spell/index.cp)（`spell.cp`、`index.cp`）：`damerauLevenshtein`（OSA 相邻换位滚动三行）、`soundex` 四码、`SymSpellIndex`（对称删除倒排索引与 `(distance, frequency, term)` 排序）及 `BkTree`（三角不等式剪枝与迭代检索，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/spell.md`](file:///data/dev/cp/doc/lib/spell.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_spell_production_deep.cp`](file:///data/dev/cp/cap/test/test_spell_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:ecs` Sparse-Set 世代防 ABA 实体组件系统 ECS 引擎（5/5 PASS，自动动态挂载 `cap.ecs`）

- 新建 [`lib/ecs/`](file:///data/dev/cp/lib/ecs/index.cp)（`ecs.cp`、`index.cp`）：`EntityRegistry`（`free_list` 槽位复用 + `generation` 防 ABA）、`ComponentPool<T>`（Sparse-Set 稀疏映射 + `dense_entities`/`dense_data` 紧凑同步数组与 O(1) Swap-and-Pop 移除）及 `EcsQuery.collectJoined2`（小池驱动交集查询，经 `DebugAllocator` 零泄漏验证）；文档 [`doc/lib/ecs.md`](file:///data/dev/cp/doc/lib/ecs.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_ecs_production_deep.cp`](file:///data/dev/cp/cap/test/test_ecs_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:barcode` Code 128 Auto / EAN-13 / UPC-A 条码编码与 SVG 渲染引擎（5/5 PASS，自动动态挂载 `cap.barcode`）

- 新建 [`lib/barcode/`](file:///data/dev/cp/lib/barcode/index.cp)（`barcode.cp`、`index.cp`）：`BarcodeEncoder`（EAN-13/UPC-A Mod 10 校验与 95 模块编码、Code 128 Auto A/B/C 双数字压缩与 Mod 103 校验和）及 `BarcodeRenderer`（黑条合并 `<rect>` SVG 输出与 `cap:xml` 校验、4 行 UTF-8 ASCII 渲染）；文档 [`doc/lib/barcode.md`](file:///data/dev/cp/doc/lib/barcode.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_barcode_production_deep.cp`](file:///data/dev/cp/cap/test/test_barcode_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:pathfinding` 2D 加权网格 A* 防穿角寻路与视线平滑引擎（5/5 PASS，自动动态挂载 `cap.pathfinding`）

- 新建 [`lib/pathfinding/`](file:///data/dev/cp/lib/pathfinding/index.cp)（`grid.cp`、`index.cp`）：`GridMap`（可配地形代价权重、二叉最小堆 A* 支持 4/8 方向严格防穿墙角 Corner-Cutting Prevention、超覆盖 Bresenham `hasLineOfSight` 与 `smoothPath` 路径拉直、Dijkstra `computeDistanceField` 全图距离场，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/pathfinding.md`](file:///data/dev/cp/doc/lib/pathfinding.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_pathfinding_production_deep.cp`](file:///data/dev/cp/cap/test/test_pathfinding_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:billing` SaaS & AI API 阶梯计费、周期折算与发票引擎（5/5 PASS，自动动态挂载 `cap.billing`）

- 新建 [`lib/billing/`](file:///data/dev/cp/lib/billing/index.cp)（`billing.cp`、`index.cp`）：`BillingCalculator`（`Graduated` 累进 / `Volume` 统一档位 `priceMeteredUsage`、周期升降级对称折算 `calculateProration`、防负券封顶与基点税率 `buildInvoice`）及 RFC 8259 转义 `Invoice.toJson`；文档 [`doc/lib/billing.md`](file:///data/dev/cp/doc/lib/billing.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_billing_production_deep.cp`](file:///data/dev/cp/cap/test/test_billing_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:ledger` 复式记账双分录会计总账与试算平衡引擎（5/5 PASS，自动动态挂载 `cap.ledger`）

- 新建 [`lib/ledger/`](file:///data/dev/cp/lib/ledger/index.cp)（`ledger.cp`、`index.cp`）：`Ledger`（精确 `i128` 最小货币单位、借贷必相等原子过账与失败零污染回滚、同币种校验、透支拦截、`idempotency_key` 幂等防重入、`reverseTransaction` 红字冲正防二冲、`statementForAccount` 流水及 `verifyTrialBalance` 会计恒等式校验）；文档 [`doc/lib/ledger.md`](file:///data/dev/cp/doc/lib/ledger.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_ledger_production_deep.cp`](file:///data/dev/cp/cap/test/test_ledger_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:sitemap` RFC 9309 `robots.txt` 与 XML Sitemap 0.9 / Index 引擎（5/5 PASS，自动动态挂载 `cap.sitemap`）

- 新建 [`lib/sitemap/`](file:///data/dev/cp/lib/sitemap/index.cp)（`sitemap.cp`、`index.cp`）：`RobotsRules`（RFC 9309 多 `User-agent`、`*` 回退、最长模式匹配与等长 `Allow` 优先、`*` & `$` 通配符、`Crawl-delay` 与 `Sitemap:` 列表）及 `SitemapCodec`（`detectKind`/`parseUrlSet`/`parseIndex`/`buildUrlSet`/`buildIndex` 含 50,000 上限与 `&` 转义）；文档 [`doc/lib/sitemap.md`](file:///data/dev/cp/doc/lib/sitemap.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_sitemap_production_deep.cp`](file:///data/dev/cp/cap/test/test_sitemap_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:lz4` 纯 CP LZ4 Block & Frame 压缩解压与 XXH32 校验引擎（5/5 PASS，自动动态挂载 `cap.lz4`）

- 新建 [`lib/lz4/`](file:///data/dev/cp/lib/lz4/index.cp)（`lz4.cp`、`index.cp`）：`xxhash32`、`Lz4Block`（4096 槽位哈希压缩、重叠 RLE 匹配复制、`InvalidOffset`/`OutputTooLarge` 防膨胀）及 `Lz4Frame`（`0x184D2204` 魔数、64KB 分块回退、XXH32 头部与内容双重校验，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/lz4.md`](file:///data/dev/cp/doc/lib/lz4.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_lz4_production_deep.cp`](file:///data/dev/cp/cap/test/test_lz4_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:mermaid` 声明式 Mermaid 图表构建与安全转义引擎（5/5 PASS，自动动态挂载 `cap.mermaid`）

- 新建 [`lib/mermaid/`](file:///data/dev/cp/lib/mermaid/index.cp)（`diagram.cp`、`index.cp`）：`FlowchartBuilder`（多形状/子图 `subgraph`/特殊字符自动安全引号转义与 ````mermaid```` 围栏输出）、`SequenceBuilder`（`participant`/`actor`/消息箭头/激活条/`loop`/`alt`/`else`）、`ErBuilder`（`PK`/`FK`/`UK` 与基数关系）及 `StateBuilder`；文档 [`doc/lib/mermaid.md`](file:///data/dev/cp/doc/lib/mermaid.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_mermaid_production_deep.cp`](file:///data/dev/cp/cap/test/test_mermaid_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:bplus` B+ 树有序键值索引与叶子链表范围扫描引擎（5/5 PASS，自动动态挂载 `cap.bplus`）

- 新建 [`lib/bplus/`](file:///data/dev/cp/lib/bplus/index.cp)（`bplus.cp`、`index.cp`）：`BPlusTree`（索引池与双向叶子链表、1000+ 乱序多层分裂、大规模删除借位/合并与缩层、`lowerBound`/`rangeScan`/`prefixScan`/`verifyInvariants` 自检，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/bplus.md`](file:///data/dev/cp/doc/lib/bplus.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_bplus_production_deep.cp`](file:///data/dev/cp/cap/test/test_bplus_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:bencode` BitTorrent BEP-3 Bencode 编解码与 Torrent InfoHash 引擎（5/5 PASS，自动动态挂载 `cap.bencode`）

- 新建 [`lib/bencode/`](file:///data/dev/cp/lib/bencode/index.cp)（`bencode.cp`、`index.cp`）：`BencodeCodec`（严格拦截前导零 `i03e`/`02:ab`、负零 `i-0e`、字典乱序/重复键、尾部字节与 64 层深度限制，并支持字典自动排序编码）及 `TorrentParser`（定位原始 `"info"` 字典切片计算 20B/40-hex SHA-1 `info_hash` 并解析单/多文件元数据，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/bencode.md`](file:///data/dev/cp/doc/lib/bencode.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_bencode_production_deep.cp`](file:///data/dev/cp/cap/test/test_bencode_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:feed` RSS 2.0 / Atom 1.0 / JSON Feed 1.1 统一解析与生成引擎（5/5 PASS，自动动态挂载 `cap.feed`）

- 新建 [`lib/feed/`](file:///data/dev/cp/lib/feed/index.cp)（`feed.cp`、`index.cp`）：`Feed` & `FeedCodec`（复用 `cap:xml` 与 `cap:json` 实现格式自动探测 `detectFormat`、RSS 2.0 含 CDATA/`content:encoded`/多 `<category>`、Atom 1.0、JSON Feed 1.1 统一归一化解析与 `toRss2`/`toAtom1`/`toJsonFeed` 跨格式往返生成）；文档 [`doc/lib/feed.md`](file:///data/dev/cp/doc/lib/feed.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_feed_production_deep.cp`](file:///data/dev/cp/cap/test/test_feed_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:selector` CSS3 选择器编译器与 DOM 树查询引擎（5/5 PASS，自动动态挂载 `cap.selector`）

- 新建 [`lib/selector/`](file:///data/dev/cp/lib/selector/index.cp)（`selector.cp`、`index.cp`）：`CssSelector`（支持标签/`#id`/`.class`/`*`、6 种属性操作符、` ` / `>` / `+` / `~` 组合器、`:first-child`/`:last-child`/`:only-child`/`:empty`/`:nth-child(an+b)`/`:not(...)` 伪类、多选择器 `,` 文档序去重及 `selectXml` 一步抽取）；文档 [`doc/lib/selector.md`](file:///data/dev/cp/doc/lib/selector.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_selector_production_deep.cp`](file:///data/dev/cp/cap/test/test_selector_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:optimize` 数值求根、Nelder-Mead 与 Adam 多维优化引擎（5/5 PASS，自动动态挂载 `cap.optimize`）

- 新建 [`lib/optimize/`](file:///data/dev/cp/lib/optimize/index.cp)（`opt.cp`、`index.cp`）：`Optimizer`（`bisect`、超线性收敛 `brentRoot`、`goldenSectionMin` 区间极小化、中心差分 `numericalGradient`、无导数多维 `nelderMead` 单纯形法及解析/数值双模式 `adamMinimize`，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/optimize.md`](file:///data/dev/cp/doc/lib/optimize.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_optimize_production_deep.cp`](file:///data/dev/cp/cap/test/test_optimize_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:quadtree` 动态分裂合并 2D 空间四叉树索引与 KNN 引擎（5/5 PASS，自动动态挂载 `cap.quadtree`）

- 新建 [`lib/quadtree/`](file:///data/dev/cp/lib/quadtree/index.cp)（`quadtree.cp`、`index.cp`）：`QuadTree`（安全索引池 4 分裂下推、同坐标点防无限分裂、`remove`/`update` 动态向上合并回收节点、`queryRect`/`queryCircle` 及双堆包围盒下界剪枝 `nearestK`，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/quadtree.md`](file:///data/dev/cp/doc/lib/quadtree.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_quadtree_production_deep.cp`](file:///data/dev/cp/cap/test/test_quadtree_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:prob` 概率分布、确定性采样器与 A/B 显著性假设检验引擎（5/5 PASS，自动动态挂载 `cap.prob`）

- 新建 [`lib/prob/`](file:///data/dev/cp/lib/prob/index.cp)（`dist.cp`、`index.cp`）：`Rng`（SplitMix64 + xoshiro256**、Box-Muller 正态、Knuth/PTRS 泊松、`weightedChoice` 与 `shuffle`）、`NormalDist`（Acklam 高精度 `invCdf`）/`ExponentialDist`/`BinomialDist`（`lgammaApprox` 防大数溢出）/`PoissonDist` 及 `HypothesisTest`（`twoProportionZTest`/`twoSampleZTest` 含 p-value 与置信区间）；文档 [`doc/lib/prob.md`](file:///data/dev/cp/doc/lib/prob.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_prob_production_deep.cp`](file:///data/dev/cp/cap/test/test_prob_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:geom2d` 2D 计算几何、凸包与碰撞检测引擎（5/5 PASS，自动动态挂载 `cap.geom2d`）

- 新建 [`lib/geom2d/`](file:///data/dev/cp/lib/geom2d/index.cp)（`geom.cp`、`index.cp`）：`Point2`/`Segment2`/`Aabb2`/`Circle2` 及 `Geom2D`（`segmentsIntersect` & `segmentIntersectionPoint` 含 T 形与共线边界、`pointToSegmentDist`、Shoelace `polygonArea`/`polygonCentroid`、射线法 `pointInPolygon`、Andrew 单调链 $O(N \log N)$ `convexHull` 及 AABB/Circle 碰撞）；文档 [`doc/lib/geom2d.md`](file:///data/dev/cp/doc/lib/geom2d.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_geom2d_production_deep.cp`](file:///data/dev/cp/cap/test/test_geom2d_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:fft` Cooley-Tukey Radix-2 FFT/IFFT 与频域卷积引擎（5/5 PASS，自动动态挂载 `cap.fft`）

- 新建 [`lib/fft/`](file:///data/dev/cp/lib/fft/index.cp)（`fft.cp`、`index.cp`）：`Complex` & `Fft`（迭代位反转 Radix-2 `forward`/`inverse` 往返精度 `< 1e-9`、Hann/Hamming/Blackman 窗函数、Parseval 守恒 `rfftMagnitude`、`dominantFrequency` 频谱峰值提取及自动零填充 `convolve`）；文档 [`doc/lib/fft.md`](file:///data/dev/cp/doc/lib/fft.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_fft_production_deep.cp`](file:///data/dev/cp/cap/test/test_fft_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:linalg` 通用稠密矩阵与数值线性代数引擎（5/5 PASS，自动动态挂载 `cap.linalg`）

- 新建 [`lib/linalg/`](file:///data/dev/cp/lib/linalg/index.cp)（`matrix.cp`、`index.cp`）：`Matrix`（`zeros`/`identity`/`fromSlice`/`mul`/`transpose`/`trace`/`frobeniusNorm`、列主元部分选主元 LU `determinant`/`solve`/`inverse`、Householder QR `leastSquares` 超定最小二乘求解与向量范数，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/linalg.md`](file:///data/dev/cp/doc/lib/linalg.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_linalg_production_deep.cp`](file:///data/dev/cp/cap/test/test_linalg_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:bpe` Byte-Level BPE Tokenizer 训练与无损编解码引擎（5/5 PASS，自动动态挂载 `cap.bpe`）

- 新建 [`lib/bpe/`](file:///data/dev/cp/lib/bpe/index.cp)（`bpe.cp`、`index.cp`）：`BpeTokenizer`（256 字节无损基础词表、`addSpecialToken` 最长匹配隔离、`train` 统计高频字节对迭代合并、`addMerge` 手动规则加载、`encode`/`decode` 任意 UTF-8/Emoji 100% 无损往返）；文档 [`doc/lib/bpe.md`](file:///data/dev/cp/doc/lib/bpe.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_bpe_production_deep.cp`](file:///data/dev/cp/cap/test/test_bpe_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:regex` 抗 ReDoS 线性时间 Pike VM / Thompson NFA 正则表达式引擎（5/5 PASS，自动动态挂载 `cap.regex`）

- 新建 [`lib/regex/`](file:///data/dev/cp/lib/regex/index.cp)（`compiler.cp`、`vm.cp`、`index.cp`）：Thompson NFA 字节码编译器（字符类/否定类、`\d\w\s` 转义、捕获与非捕获组、分支 `|`、锚点 `^`/`$`、贪婪与惰性量词 `{m,n}?`）与稀疏去重 Pike VM（$O(N \cdot M)$ 抗 ReDoS 线性时间、`find`/`findAll`/`captures`/`replaceAll`/`split`）；文档 [`doc/lib/regex.md`](file:///data/dev/cp/doc/lib/regex.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_regex_production_deep.cp`](file:///data/dev/cp/cap/test/test_regex_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:svg` 矢量 SVG 画布与统计图表渲染引擎（5/5 PASS，自动动态挂载 `cap.svg`）

- 新建 [`lib/svg/`](file:///data/dev/cp/lib/svg/index.cp)（`canvas.cp`、`charts.cp`、`index.cp`）：`SvgPathBuilder` & `SvgCanvas`（基础图元、分组嵌套、有限数值校验与 XML XSS 安全转义）及 `SvgCharts`（Heckbert Nice Ticks `barChart`、多系列归一化 `lineChart`、实心/环形与 100% 扇区 `pieChart`，经 `cap:xml` 解析验证）；文档 [`doc/lib/svg.md`](file:///data/dev/cp/doc/lib/svg.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_svg_production_deep.cp`](file:///data/dev/cp/cap/test/test_svg_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:sketch` 流式概率素描引擎（HLL / T-Digest / ReservoirSampler，5/5 PASS）

- 新建 [`lib/sketch/`](file:///data/dev/cp/lib/sketch/index.cp)（`sketch.cp`、`index.cp`）：`HyperLogLog`（FNV-1a+SplitMix64 混洗、LinearCounting 修正与 `merge`）、`QuantileDigest`（T-Digest 质心局部密度合并估算 P50/P95/P99 与 `merge`）及 `ReservoirSampler<T>`（Algorithm R + Lemire 无偏蓄水池抽样）；文档 [`doc/lib/sketch.md`](file:///data/dev/cp/doc/lib/sketch.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_sketch_production_deep.cp`](file:///data/dev/cp/cap/test/test_sketch_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:intern` 指针稳定字符串驻留池与 1:1 双向映射表引擎（5/5 PASS，自动动态挂载 `cap.intern`）

- 新建 [`lib/intern/`](file:///data/dev/cp/lib/intern/index.cp)（`intern.cp`、`index.cp`）：`StringInterner`（4KB 指针稳定分块 Arena + `SymbolId` 去重驻留、`lookup`/`resolve`/`totalBytes`）与 `StringBiMap`（严格 1:1 双射 `insert`、`forcePut` 原子置换与双向查询删除，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/intern.md`](file:///data/dev/cp/doc/lib/intern.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_intern_production_deep.cp`](file:///data/dev/cp/cap/test/test_intern_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:ical` RFC 5545 iCalendar `.ics` 解析生成与 `RRULE` 重复规则展开引擎（5/5 PASS，自动动态挂载 `cap.ical`）

- 新建 [`lib/ical/`](file:///data/dev/cp/lib/ical/index.cp)（`ical.cp`、`index.cp`）：`IcalDateTime`（$O(1)$ Gregorian 闰年与世纪非闰年日历算术、Sakamoto `dayOfWeek`）、`RRule`（`DAILY`/`WEEKLY`+`BYDAY`/`MONTHLY`+`BYMONTHDAY`/`YEARLY`、`INTERVAL`/`COUNT`/`UNTIL`/`EXDATE` 展开与 `max_instances` 防护）及 `IcalCalendar`（75 字节行折行展开与 `toIcs` 往返）；文档 [`doc/lib/ical.md`](file:///data/dev/cp/doc/lib/ical.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_ical_production_deep.cp`](file:///data/dev/cp/cap/test/test_ical_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:zset` SkipList 跳表 + 哈希双索引有序集合引擎（5/5 PASS，自动动态挂载 `cap.zset`）

- 新建 [`lib/zset/`](file:///data/dev/cp/lib/zset/index.cp)（`zset.cp`、`index.cp`）：`SortedSet`（32 层 `span` 跨度 SkipList + `StringHashMapUnmanaged` 双索引、`(score, member)` 同分字典序决胜、`NaN` 拦截、$O(\log N)$ `zrank`/`zrevrank`、负数切片 `zrange`/`zrevrange`、`zrangeByScore`/`zremRangeByScore`、`zpopMin`/`zpopMax` 与 `Sum`/`Min`/`Max` `unionInto` 聚合，经 `std.testing.allocator` 零泄漏验证）；文档 [`doc/lib/zset.md`](file:///data/dev/cp/doc/lib/zset.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_zset_production_deep.cp`](file:///data/dev/cp/cap/test/test_zset_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:raft` 确定性 Raft 共识核心状态机（5/5 PASS，自动动态挂载 `cap.raft`）

- 新建 [`lib/raft/`](file:///data/dev/cp/lib/raft/index.cp)（`raft.cp`、`index.cp`）：`RaftNode`（Follower/Candidate/Leader 三态转换、`RequestVote` 日志新旧安全性校验、`AppendEntries` 前驱检查与冲突后缀截断、Figure 8 仅提交当前任期多数派推进 `commit_index`、`drainApplied` 与 `drainOutbound`），并同步将前期热日志滚动归档至 `CHANGELOG_H.md`/`DEVLOG_H.md`；文档 [`doc/lib/raft.md`](file:///data/dev/cp/doc/lib/raft.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_raft_production_deep.cp`](file:///data/dev/cp/cap/test/test_raft_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:agentmem` AI Agent 分层长期记忆与三因子联合召回引擎（5/5 PASS，自动动态挂载 `cap.agentmem`）

- 新建 [`lib/agentmem/`](file:///data/dev/cp/lib/agentmem/index.cp)（`memory.cp`、`index.cp`）：`AgentMemory`（`Working`/`Episodic`/`Semantic` 三层模型、`Working` LRU/FIFO 上限自动淘汰、Recency 指数衰减 + Importance + Relevance 关键词/标签相关性三因子联合召回评分、`consolidate` 晋升、`prune` 遗忘及 `exportJsonl`/`importJsonl` 持久化往返）；文档 [`doc/lib/agentmem.md`](file:///data/dev/cp/doc/lib/agentmem.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_agentmem_production_deep.cp`](file:///data/dev/cp/cap/test/test_agentmem_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:mime` RFC 6838 MediaType、`Accept` 内容协商与 `MimeDb` 引擎（5/5 PASS，自动动态挂载 `cap.mime`）

- 新建 [`lib/mime/`](file:///data/dev/cp/lib/mime/index.cp)（`mime.cp`、`db.cp`、`index.cp`）：`MediaType`（大小写归一化、引号参数、`+suffix` 匹配与 `toString`）、`ContentNegotiator`（RFC 9110 `Accept` `q=0..1` 权重与 specificity 精确排序）及 `MimeDb`（110+ 常见扩展名双向映射与 `isText`/`isCompressible`）；文档 [`doc/lib/mime.md`](file:///data/dev/cp/doc/lib/mime.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_mime_production_deep.cp`](file:///data/dev/cp/cap/test/test_mime_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:jsonpath` RFC 9535 JSONPath 编译器与过滤查询引擎（5/5 PASS，自动动态挂载 `cap.jsonpath`）

- 新建 [`lib/jsonpath/`](file:///data/dev/cp/lib/jsonpath/index.cp)（`parser.cp`、`query.cp`、`index.cp`）：`JsonPath`（预编译校验 + 32 层深度守卫、点分/引号/负下标/通配符 `*`/多选并集/正负步长切片 `[s:e:step]`/递归下降 `..`/复合过滤表达式 `[?(@.a && !@.b)]`、Normalized Path 生成与 `evalJson` 便捷入口）；文档 [`doc/lib/jsonpath.md`](file:///data/dev/cp/doc/lib/jsonpath.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_jsonpath_production_deep.cp`](file:///data/dev/cp/cap/test/test_jsonpath_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:lock` 逻辑读写锁、等待图死锁检测与 Fencing 租约锁引擎（5/5 PASS，自动动态挂载 `cap.lock`）

- 新建 [`lib/lock/`](file:///data/dev/cp/lib/lock/index.cp)（`wait_graph.cp`、`lock.cp`、`index.cp`）：`WaitForGraph`（3 色迭代 DFS 环路检测）、`NamedLockTable`（多读者共享/排他写/同属主重入、`DeadlockDetected` 实时拦截回滚、`isResourceLocked`/`NotOwner`）及 `LeaseLockManager`（单调 `fencing_token`、TTL 过期抢占、`renewLease`/`releaseLease` 与 `validateToken` 防脑裂）；文档 [`doc/lib/lock.md`](file:///data/dev/cp/doc/lib/lock.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_lock_production_deep.cp`](file:///data/dev/cp/cap/test/test_lock_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:health` 云原生健康探针汇聚与有序优雅停机协调器（5/5 PASS，自动动态挂载 `cap.health`）

- 新建 [`lib/health/`](file:///data/dev/cp/lib/health/index.cp)（`probe.cp`、`index.cp`）：`HealthRegistry`（Liveness/Readiness/Startup/All 探针聚合、`setDraining` 一键排空切 `Readiness` 为 503 同时保活 `Liveness`、RFC 8259 JSON 报告）及 `ShutdownCoordinator`（按 `priority` 降序严格幂等执行停机钩子）；文档 [`doc/lib/health.md`](file:///data/dev/cp/doc/lib/health.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_health_production_deep.cp`](file:///data/dev/cp/cap/test/test_health_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:pool` 通用有界资源与连接池引擎（5/5 PASS，自动动态挂载 `cap.pool`）

- 新建 [`lib/pool/`](file:///data/dev/cp/lib/pool/index.cp)（`pool.cp`、`index.cp`）：`ResourcePool`（`min_idle` 预热 `warmup`、`Lifo`/`Fifo` 借出策略、`idle_timeout_ms` 空闲驱逐与 `max_lifetime_ms` 寿命淘汰、`validate_fn` 失效透明重建、`checkin`/`discard` 及 `deinit` 零泄漏保障）；文档 [`doc/lib/pool.md`](file:///data/dev/cp/doc/lib/pool.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_pool_production_deep.cp`](file:///data/dev/cp/cap/test/test_pool_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:bitset` 64-bit 字级动态位图与零堆固定位图引擎（5/5 PASS，自动动态挂载 `cap.bitset`）

- 新建 [`lib/bitset/`](file:///data/dev/cp/lib/bitset/index.cp)（`bitset.cp`、`index.cp`）：`BitSet`（动态扩容 `u64` 字数组、`setRange`/`clearRange`、`@popCount`/`@ctz` 快速 `count`/`firstSetBit`/`nextSetBit`/`toIndices`、不等长安全并/交/差/对称差与 `jaccardSimilarity`、紧凑十六进制 `toHex`/`fromHex`）及零堆 `FixedBitSet(N)`；文档 [`doc/lib/bitset.md`](file:///data/dev/cp/doc/lib/bitset.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_bitset_production_deep.cp`](file:///data/dev/cp/cap/test/test_bitset_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:pipeline` 洋葱模型中间件与可观测多阶段流水线引擎（5/5 PASS，自动动态挂载 `cap.pipeline`）

- 新建 [`lib/pipeline/`](file:///data/dev/cp/lib/pipeline/index.cp)（`pipeline.cp`、`index.cp`）：`Pipeline` & `PipelineContext`（`before`/`after`/`onError` 洋葱模型钩子、短路 `halt` 仍按逆序清理已进入阶段、条件阶段 `useConditional`、动态启停及 `StageTrace` 耗时轨迹）；文档 [`doc/lib/pipeline.md`](file:///data/dev/cp/doc/lib/pipeline.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_pipeline_production_deep.cp`](file:///data/dev/cp/cap/test/test_pipeline_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:uritemplate` RFC 6570 Level 1~4 URI Template 解析与展开引擎（5/5 PASS，自动动态挂载 `cap.uritemplate`）

- 新建 [`lib/uritemplate/`](file:///data/dev/cp/lib/uritemplate/index.cp)（`template.cp`、`index.cp`）：`UriTemplate` & `TemplateVars`（全面支持 RFC 6570 8 种操作符、UTF-8 字符级前缀截取 `{var:N}`、列表/关联表爆炸 `{list*}`/`{keys*}`、精确百分号编码、未定义变量跳过与去重 `variables()` 提取）；文档 [`doc/lib/uritemplate.md`](file:///data/dev/cp/doc/lib/uritemplate.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_uritemplate_production_deep.cp`](file:///data/dev/cp/cap/test/test_uritemplate_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:ringbuf` 有界环形缓冲、字节流管道与双阈值微批聚合引擎（5/5 PASS，自动动态挂载 `cap.ringbuf`）

- 新建 [`lib/ringbuf/`](file:///data/dev/cp/lib/ringbuf/index.cp)（`ring.cp`、`index.cp`）：`RingBuffer<T>`（`Reject`/`OverwriteOldest`、`overwrittenCount`、跨回绕 `popBatch`）、`ByteRing`（跨尾两段 `@memcpy` 读写与 `readUntilByte` 增量帧切分）及 `MicroBatcher<T>`（条数/超时双触发与 `drain` 复位）；文档 [`doc/lib/ringbuf.md`](file:///data/dev/cp/doc/lib/ringbuf.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_ringbuf_production_deep.cp`](file:///data/dev/cp/cap/test/test_ringbuf_production_deep.cp) `-m` 5/5 PASS。

---

