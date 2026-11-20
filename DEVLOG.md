# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:avro` Apache Avro Schema 驱动二进制编解码与 OCF 容器引擎（5/5 PASS，文档直写 `doc/lib/avro.md`）
- **核心设计**：在 `AvroSchema` 内部使用 `ArenaAllocator` 管理复杂嵌套与命名类型引用，编解码器严格校验 `SchemaMismatch`、`InvalidUnionIndex`、Varint 溢出与 OCF 块 `sync_marker` 一致性，零泄漏。

## 2026-11-21: 新建 `cap:tiff` Baseline TIFF 多页双端序与多压缩算法编解码引擎（5/5 PASS，文档直写 `doc/lib/tiff.md`）
- **核心设计**：统一抽象 `"II"` 小端与 `"MM"` 大端 IFD 读写器并检测循环 IFD 链表，复用 `cap:lzw` 与 `cap:deflate` 结合内联 PackBits 与 `Predictor = 2` 水平差分还原，全量测试零泄漏。

## 2026-11-21: 新建 `cap:gif` 纯 CP GIF87a/GIF89a 多帧动画编解码引擎（5/5 PASS，文档直写 `doc/lib/gif.md`）
- **核心设计**：复用 `cap:lzw` (`LsbFirst`) 处理 255 字节最大数据子块流，支持 4-pass Interlaced 重排与局部调色板覆盖全局调色板，并在解码前严格校验宽高与帧数上限防炸弹。

## 2026-11-21: 新建 `cap:wkt` OGC WKT 与 WKB 空间几何双向编解码引擎（5/5 PASS，文档直写 `doc/lib/wkt.md`）
- **核心设计**：统一 `WktGeometry` AST 模型同时服务 WKT 文本与 ISO WKB 大小端二进制双向无损转换，并在解析与解码入口强制校验 Polygon 闭合环、WKB 字节序标记及 32 层 `GEOMETRYCOLLECTION` 递归上限。

## 2026-11-21: 新建 `cap:lzw` 可变位宽 LZW 字典无损压缩与解压引擎（5/5 PASS，文档直写 `doc/lib/lzw.md`）
- **核心设计**：统一支持 LSB/MSB 位流打包及 `2..8` 初始码宽，满 4096 字典自动发出 `ClearCode` 复位；解码端完整处理经典 `KwKwK` 码字并拦截缺失 `EOI`、非法码字与超限输出。

## 2026-11-21: 新建 `cap:stomp` 与 `cap:geojson`（均 5/5 PASS，扁平文档直写 `doc/lib/`）
- **`cap:stomp`**：`StompStreamParser` 支持 1..5 字节任意切分半包拆帧并正确处理 STOMP 1.2 `\r`/`\n`/`\c`/`\\` 头部转义及重复头取首个规范；拦截非转义命令、缺 `\x00` 终止符与超限载荷。
- **`cap:geojson`**：遵循 RFC 7946 校验 Polygon 环至少 4 点首尾闭合；Ray-Casting 射线法结合边界判定支持多内环孔洞排除，Douglas-Peucker 采用显式栈迭代防止深递归溢出。

## 2026-11-21: 新建 `cap:imap` RFC 3501/9051 IMAP4 协议命令与 Literal 解析引擎（5/5 PASS，文档直写 `doc/lib/imap.md`）
- **核心设计**：`ImapStreamParser` 支持任意分块边界解析同步/非同步 `{N}`/`{N+}` 二进制 Literal 块并通过 `markEof`/`finish` 精确检测截断；命令编码器自动转义引号与反斜杠并阻断 CR/LF 注入。

## 2026-11-21: 新建 `cap:nats` NATS Wire 协议与 HPUB/HMSG 增量流式解析编码引擎（5/5 PASS，文档直写 `doc/lib/nats.md`）
- **核心设计**：`NatsParser` 支持 `1..7` 字节任意切分流式解析及二进制 `\r\n\x00` 载荷零损耗提取，严格校验 `hdr_len <= total_len` 与 `max_payload_bytes` 上限；`NatsEncoder` 全面拦截主题空白与 CR/LF 命令注入。

## 2026-11-21: 新建 `cap:ldap` RFC 4515 LDAP 搜索过滤器与 RFC 4514 DN 引擎（5/5 PASS，文档直写 `doc/lib/ldap.md`）
- **核心设计**：过滤器解析器在编译期校验括号配对、十六进制转义及 64 层嵌套上限，`matchesEntry` 大小写不敏感支持多值属性、`ge`/`le` 及 `substring` 首中尾匹配，全程零泄漏。

## 2026-11-21: 新建 `cap:amqp` AMQP 0-9-1 二进制协议帧与 FieldTable 引擎（5/5 PASS，文档直写 `doc/lib/amqp.md`）
- **核心设计**：大端序封装 AMQP 0-9-1 帧头并严格校验 `0xCE` 帧尾与 `Heartbeat` channel=0；`AmqpStreamDecoder` 支持任意小分块拆帧并拦截超限负载与超深嵌套 `FieldTable`。

## 2026-11-21: 新建 `cap:http3` RFC 9114 HTTP/3 帧与 RFC 9204 QPACK 静态表引擎（5/5 PASS，文档直写 `doc/lib/http3.md`）
- **核心设计**：内置 RFC 9000 Varint 与 RFC 9204 Appendix A 全量 99 项静态表，严格拦截 HTTP/2 保留设置/帧及重复设置 ID；解码器采用零拷贝载荷切片并按 `max_frame_size` 防御超大帧。

## 2026-11-21: 新建 `cap:bmp` 纯 CP BMP 位图图像编解码引擎（5/5 PASS，文档直写 `doc/lib/bmp.md`）
- **核心设计**：统一将 `bottom-up` 与负高度 `top-down` BMP 归一为紧凑 top-down RGB/RGBA 缓冲区，编码端自动按 4 字节步长补零对齐，解码端严格拦截签名、`BI_RGB`/`BI_BITFIELDS` 之外的压缩及超限尺寸。

## 2026-11-21: 新建 `cap:midi` Standard MIDI File 多轨音序编解码引擎（5/5 PASS，文档直写 `doc/lib/midi.md`）
- **核心设计**：精确处理 MIDI Running Status 状态继承与 SysEx/Meta 事件清除边界；VLQ 限制最多 4 字节（`<= 0x0FFFFFFF`）并自动补齐 `End of Track` (`FF 2F 00`)，零泄漏。

## 2026-11-21: 新建 `cap:quic` RFC 9000 QUIC 变长整数、包头与核心帧编解码引擎（5/5 PASS，文档直写 `doc/lib/quic.md`）
- **核心设计**：按 RFC 9000 规范实现 1/2/4/8 字节大端序变长整数（严格拦截 `> 2^62 - 1` 溢出）及包头/帧载荷零泄漏解析，异常路径全链路 `errdefer` 自动回滚释放。

## 2026-11-21: 新建 `cap:wav` 纯 CP RIFF/WAVE PCM 音频容器编解码与重采样引擎（5/5 PASS，文档直写 `doc/lib/wav.md`）
- **核心设计**：统一将五种 PCM/Float 采样格式归一化为交错 `f32` 缓冲并在编码端执行饱和钳位，跳过 `JUNK`/`LIST`/`fact` 等非关键块并严格校验 RIFF/WAVE 头与截断边界。

## 2026-11-21: 新建 `cap:sip` RFC 3261 SIP 会话初始协议报文解析与构建引擎（5/5 PASS，文档直写 `doc/lib/sip.md`）
- **核心设计**：按 `SIP/2.0` 协议规范实现大小写不敏感与紧凑单字母头名无缝互查，`parse()` 严格校验状态码 `100..699` 与 `Content-Length` 载荷完整性，`SipBuilder` 自动维护 `Content-Length` 并阻断 CRLF 注入。

## 2026-11-21: 新建 `cap:png` 纯 CP PNG 图像无损编解码与 Paeth 滤波引擎（5/5 PASS，文档直写 `doc/lib/png.md`）
- **核心设计**：零 `libpng` 依赖，复用 `cap:deflate` 的 `compressZlib`/`decompressZlib`/`crc32` 完成完整 PNG 流式编解码，编码端自动按最小 SAD 选择最优扫描行滤波器，解码端严格拦截签名/CRC/滤波/缺失 `IEND` 及超限尺寸。

## 2026-11-21: 新建 `cap:http2` RFC 9113 HTTP/2 二进制帧编解码与流控引擎（5/5 PASS，文档直写 `doc/lib/http2.md`）
- **核心设计**：采用 9 字节大端序帧头编码并清除保留位 `R`；`FrameStreamDecoder` 支持 1..7 字节任意切分增量拆帧并在 `SETTINGS` 非 0 `stream_id` / 非 6 倍数及 `WINDOW_UPDATE` 增量溢出时严格拦截。

## 2026-11-21: 新建 `cap:rtree` 2D 轴对齐边界框（AABB）R-Tree 空间索引引擎（5/5 PASS，文档直写 `doc/lib/rtree.md`）
- **核心设计**：采用最小面积增量 `ChooseLeaf` 与 Guttman `Quadratic Split` 维持树平衡，`removeById` 清理空节点后向上收紧祖先 MBR；560 个重叠矩形与暴力全量对拍 100% 一致且零泄漏。

## 2026-11-21: 新建 `cap:rtp` RFC 3550 RTP/RTCP 实时传输包编解码与 Jitter Buffer 引擎（5/5 PASS，文档直写 `doc/lib/rtp.md`）
- **核心设计**：解码器严格校验 `V=2`、CSRC/Extension 截断与 Padding `1..remaining` 范围；`JitterBuffer` 采用模 `65536` 符号半空间序列号差值实现无歧义乱序重排、重复包去重与丢包统计。

## 2026-11-21: 新建 `cap:sdp` RFC 8866 WebRTC SDP 会话描述解析与构建引擎（5/5 PASS，文档直写 `doc/lib/sdp.md`）
- **核心设计**：按会话级与媒体流区块两层解析，零拷贝引用原始字段切片并通过 `errdefer` 完整回滚异常路径内存；严格校验 `v=0`、`o=`、`s=` 及 `m=` 端口与格式列表。

## 2026-11-21: 新建 `cap:smtp` RFC 5321/5322 SMTP 协议编解码与 MIME 邮件构建引擎（5/5 PASS，文档直写 `doc/lib/smtp.md`）
- **核心设计**：零网络系统调用实现无状态 SMTP 命令/多行响应编解码、RFC 5321 §4.5.2 行首点号透明转义以及复合 `multipart/alternative` + `multipart/mixed` 邮件组装，全程校验防头注入。

## 2026-11-21: 新建 `cap:stun` RFC 5389/8489 STUN NAT 穿透消息与完整性校验引擎（5/5 PASS，文档直写 `doc/lib/stun.md`）
- **核心设计**：`StunBuilder` 按 RFC 5389 规范自动对变长属性做 4 字节补零对齐并依次计算 `MESSAGE-INTEGRITY` 与 `FINGERPRINT`；`verifyMessageIntegrity` 采用恒定时间比较并严格校验 `MAGIC_COOKIE` 与头前 2 位 `00`。

## 2026-11-21: 新建 `cap:bson` MongoDB BSON 1.1 二进制文档与 ObjectId 编解码引擎（5/5 PASS，文档直写 `doc/lib/bson.md`）
- **核心设计**：严格遵循 BSON 1.1 规范校验 4 字节总长、EOO `0x00` 终止符、CString 无内嵌零字节、UTF-8 合法性及默认 64 层深度上限；`ObjectId` 按大端序时间戳+随机数+计数器封装并支持 24 位 Hex 互转。

## 2026-11-21: 新建 `cap:prometheus` Prometheus / OpenMetrics 指标注册表与文本编解码引擎（5/5 PASS，文档直写 `doc/lib/prometheus.md`）
- **核心设计**：在注册期严格校验指标名、标签名（拒绝 `__` 保留前缀）、Counter 非负增量与 Histogram 严格升序桶边界；`exportText` 与 `parseExposition` 完整支持标签值转义往返与 `+Inf`/`-Inf`/`NaN` 解析。

## 2026-11-21: 新建 `cap:socks5` RFC 1928/1929 SOCKS5 代理协议 Wire 编解码引擎（5/5 PASS，文档直写 `doc/lib/socks5.md`）
- **核心设计**：零网络系统调用实现完整 SOCKS5 握手、账密、请求/响应与 UDP 封装，严格拦截非法版本号、非预期 `ATYP`、超 255 字节域名及截断/尾随字节。

## 2026-11-21: 新建 `cap:x509` 纯 CP X.509 v3 DER/PEM 证书解析与 SAN 校验引擎（5/5 PASS，文档直写 `doc/lib/x509.md`）
- **核心设计**：复用 `cap:asn1` 与 `cap:pem` 并由独立 `ArenaAllocator` 统一托管解析期所有派生切片，严格遵循 RFC 6125 单级通配符主机名匹配与 CA 基本约束校验，全量测试零泄漏。

## 2026-11-21: 新建 `cap:zip` 纯 CP PKZIP 归档打包与安全解包引擎（5/5 PASS，文档直写 `doc/lib/zip.md`）
- **核心设计**：直接复用 `cap:deflate` 压缩与 `crc32` 校验，零外部 C/系统调用实现 PKZIP 打包解包；解包器在分配内存前先验证条目总数、未压缩大小上限及安全相对路径，彻底阻断路径穿越与超限膨胀。

## 2026-11-21: 新建 `cap:pem` RFC 7468 PEM 证书链与密钥块编解码引擎（5/5 PASS，文档直写 `doc/lib/pem.md`）
- **核心设计**：严格校验 `-----BEGIN <LABEL>-----` 与 `-----END <LABEL>-----` 匹配与 Base64 字符集完整性，支持 RFC 1421 `Proc-Type`/`DEK-Info` 头字段保留与 `TooManyBlocks` 资源上限防护，零泄漏。

## 2026-11-21: 新建 `cap:deflate` 纯 CP RFC 1951/1952/1950 DEFLATE/GZIP/ZLIB 无损压缩解压引擎（5/5 PASS，文档直写 `doc/lib/deflate.md`）
- **核心设计**：零外部 C/系统依赖完成 Stored / Fixed Huffman / Dynamic Huffman 全块型解码与 LZ77 压缩，严格校验非法块类型 `BTYPE=11`、越界距离、CRC32/Adler-32 与最大输出上限。

## 2026-11-21: 新建 `cap:multipart` RFC 7578 `multipart/form-data` 解析与安全构建引擎（5/5 PASS，文档直写 `doc/lib/multipart.md`）
- **核心设计**：兼容 `\r\n`/`\n` 与 preamble/epilogue 边界扫描，内嵌 `\x00` 二进制载荷零损耗提取；在构建与解析阶段同时拦截 CR/LF/NUL 注入、缺失 `--boundary--` 及分片数量/体积超限。

## 2026-11-21: 新建 `cap:sse` W3C Server-Sent Events 增量流式解析与编码引擎（5/5 PASS，文档直写 `doc/lib/sse.md`）
- **核心设计**：`SseParser` 逐字节状态机正确处理 `\r\n` 跨 Chunk 边界及流首 UTF-8 BOM，严格遵循冒号单前导空格剥离、空 `data` 不触发事件、`id:` 含 `\x00` 忽略以及 `max_event_bytes` 上限拦截。

## 2026-11-21: 新建 `cap:kdtree` 多维空间 K-D Tree 最近邻与范围检索引擎（5/5 PASS，文档直写 `doc/lib/kdtree.md`）
- **核心设计**：建树期按各维方差与中位数划分平衡超平面，`kNearest` 结合最大堆边界剪枝回溯，并在 2D/3D/5D 高维点集下与全量暴力欧氏距离对拍 100% 一致。

## 2026-11-21: 新建 `cap:ndjson` NDJSON / JSON Lines 增量流式分帧与校验引擎（5/5 PASS，文档直写 `doc/lib/ndjson.md`）
- **核心设计**：`NdjsonStreamParser` 维护跨分块行缓冲与行号计数器，严格校验每行完整 JSON 语法并在 `max_line_bytes` 超限时立即阻断；`NdjsonWriter` 禁止未转义换行符注入破坏帧边界。

## 2026-11-21: 新建 `cap:jsonpatch` RFC 6901 JSON Pointer 与 RFC 6902 JSON Patch 原子事务引擎（5/5 PASS，文档直写 `doc/lib/jsonpatch.md`）
- **核心设计**：`applyPatch` 在独立 `ArenaAllocator` 深拷贝树上顺序执行补丁操作，任意校验或路径异常立即回滚释放并保持原输入不变；`createDiffPatch` 递归对比 JSON 树生成可逆最小 Patch 数组。

## 2026-11-21: 新建 `cap:bitstream` MSB/LSB 位流与 ULEB128/SLEB128/Exp-Golomb 编码引擎（5/5 PASS，文档直写 `doc/lib/bitstream.md`）
- **核心设计**：统一 `BitWriter`/`BitReader` 支持 `MsbFirst`（网络/视频流）与 `LsbFirst`（压缩/容器流）双位序；`Leb128` 严格限制 64-bit 最多 10 字节编码并正确完成有符号补码扩展。

## 2026-11-21: 新建 `cap:cidr` IPv4/IPv6 CIDR 子网划分、超网聚合与 LPM 路由表引擎（5/5 PASS，文档直写 `doc/lib/cidr.md`）
- **核心设计**：`Cidr4.summarize` 先按网络地址与前缀长度排序去重被包含子网，再迭代合并相邻兄弟前缀；`LpmTable4<V>` 采用二进制基数树实现 $O(32)$ 最长前缀匹配（含 `/0` 兜底）。

## 2026-11-21: 新建 `cap:suffixarray` 后缀数组、Kasai LCP 数组与全文本二分索引引擎（5/5 PASS，文档直写 `doc/lib/suffixarray.md`）
- **核心设计**：采用倍增前缀排序结合计数/基数排序构建后缀数组 `sa`（$O(N \log N)$），再以 Kasai 线性算法构造 `lcp` 数组，使所有重复子串统计与二分检索在零正则引擎开销下稳定运行。

## 2026-11-21: 新建 `cap:base85` Z85 / Adobe Ascii85 / RFC 1924 紧凑编码引擎（5/5 PASS，文档直写 `doc/lib/base85.md`）
- **核心设计**：统一封装三种主流 Base85 变体，使用 `u64` 中间累加防 `u32` 溢出并严格检测 `"%%%%%"`/`"|NsC1"` 越界码字与块内 `'z'` 畸形帧，全量测试零泄漏。

## 2026-11-21: 新建 `cap:diff3` 三方文本合并与冲突检测引擎（5/5 PASS，文档直写 `doc/lib/diff3.md`）
- **核心设计**：对齐 `base -> ours` 与 `base -> theirs` 编辑区段，自动合并互不重叠修改与双边一致修改，并对真正冲突按 `Standard` 或 `Diff3` 样式生成精确冲突块，全程零泄漏。

## 2026-11-21: 新建 `cap:segtree` Fenwick 树状数组与惰性传播线段树范围聚合引擎（5/5 PASS，文档直写 `doc/lib/segtree.md`）
- **核心设计**：`LazySegmentTree` 严格区分 `has_assign` 与 `add_lazy` 标记优先级（`assign` 覆写旧 `add`，后续 `add` 直接叠加至 `assign_val`），经 120 轮交叉随机操作与暴力数组对拍 100% 一致。

## 2026-11-21: 新建 `cap:dsu` 路径压缩、带权差分约束与可回溯撤销并查集引擎（5/5 PASS，文档直写 `doc/lib/dsu.md`）
- **核心设计**：在同一包内分场景提供迭代两遍路径压缩 `DisjointSet`、带权偏置关系 `WeightedDsu` 与无路径压缩历史栈 `RollbackDsu`（$O(\log N)$ 查找与 $O(1)$ 精确回滚），全程零泄漏。

## 2026-11-21: 新建 `cap:fuzzy` 编辑距离、Jaro-Winkler 与 fzf 风格模糊评分高亮引擎（5/5 PASS，文档直写 `doc/lib/fuzzy.md`）
- **核心设计**：`levenshtein` 采用 $O(\min(M, N))$ 空间优化，`damerauLevenshtein` 采用 Lowrance-Wagner 无限制算法，`fuzzyScore` 采用 DP 连续/词边界/驼峰加分回溯（大矩阵退化贪心）输出精确命中 `positions`，零泄漏。

## 2026-11-21: 新建 `cap:pqueue` 索引优先队列、流式 Top-K 与双堆实时中位数引擎（5/5 PASS，文档直写 `doc/lib/pqueue.md`）
- **核心设计**：`IndexedMinHeap<V>` 采用 20-bit 槽位索引 + 12-bit 代际号的 `u32` 句柄消除槽位复用 ABA 问题；`BoundedTopK<V>` 与 `StreamingMedian` 分别以 $O(K)$ 堆与高低双堆实现流式 Top-K 与精确实时中位数计算。

## 2026-11-21: 新建 `cap:interval` 区间集合代数与 AVL 增强区间树重叠检索引擎（5/5 PASS，文档直写 `doc/lib/interval.md`）
- **核心设计**：`Interval` 长度采用 `i128` 防止跨正负极值溢出；`IntervalSet` 维持规范化不相交有序不变量，`IntervalTree<V>` 采用 AVL 自平衡树维护子树 `max_end` 实现剪枝检索并全程零泄漏。

## 2026-11-21: 新建 `cap:jsonschema` JSON Schema 结构化校验与 `$ref` 路径诊断引擎（5/5 PASS，文档直写 `doc/lib/jsonschema.md`）
- **核心设计**：在 `SchemaValidator` 内部使用独立 `ArenaAllocator` 持有编译后 Schema AST，校验时输出完整 JSONPath（如 `$.user.email`）与关键字诊断，并在 `$ref` 解析链路强制 64 层递归深度上限。

## 2026-11-21: 新建 `cap:skiplist` 有序跳表（SkipList）内存索引与范围扫描引擎（5/5 PASS，文档直写 `doc/lib/skiplist.md`）
- **核心设计**：采用连续节点池 + `free_head` 空闲槽索引管理多级 `forward` 指针塔，避免高频插入/删除产生堆碎片，并在 `upsert` 时独立深拷贝键切片以消除悬垂引用，1024+ 乱序键测试零泄漏。

## 2026-11-21: 新建 `cap:roaring` Roaring Bitmap 自适应压缩位图集合引擎（5/5 PASS，文档直写 `doc/lib/roaring.md`）
- **核心设计**：高 16 位桶有序存储，单块基数跨越 4096 阈值自动在有序 `u16` 切片与 `1024 x u64` 位图之间双向切换；混合容器交并差及 `andCardinality` 直接按对偶容器算法高效归约。

## 2026-11-21: 新建 `cap:aho` Aho-Corasick 多模式自动机与敏感词替换引擎（5/5 PASS，文档直写 `doc/lib/aho.md`）
- **核心设计**：在 `build()` 阶段将 Failure Link 展开为完整 DFA 状态转移表并维护 Dictionary Suffix Link，使文本扫描复杂度严格锁定为 $O(N)$，550+ 模式大词典零泄漏稳定运行。

## 2026-11-21: 升级加固 `cap:cbor` RFC 8949 CBOR 确定性编码与安全解码引擎（5/5 PASS，文档直写 `doc/lib/cbor.md`）
- **核心设计**：在兼容既有 `Bin*` 接口的前提下补齐 RFC 8949 标准 `CborValue` 与 `encodeCanonical`（最小整数/浮点及 Map 键按编码字节序排序），并在 `decode` 全链路通过 `errdefer` 清理已构造容器，`std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:otp` RFC 4226 HOTP / RFC 6238 TOTP 与 `otpauth://` URI 认证引擎（5/5 PASS，文档直写 `doc/lib/otp.md`）
- **核心设计**：采用显式 `unix_sec: u64` 注入规避系统时钟依赖，并通过 RFC 4226 Appendix D 与 RFC 6238 Appendix B 全量标准向量逐项比对验证；`verifyTotp` 使用恒定时间比较并返回 `matched_step` 便于上层防重放。

## 2026-11-21: 新建 `cap:lru` O(1) LRU 缓存淘汰、TTL 过期与字节权重配额引擎（5/5 PASS，文档直写 `doc/lib/lru.md`）
- **核心设计**：采用连续槽位池上的侵入式双向链表配合 `StringHashMapUnmanaged` 实现零指针悬垂与严格 $O(1)$ 晋升淘汰，全量 TTL 逻辑由 `now_ms: i64` 确定性注入，无需系统时钟。

## 2026-11-21: 升级加固 `cap:protobuf` Protocol Buffers Wire 格式动态编解码引擎（5/5 PASS，文档直写 `doc/lib/protobuf.md`）
- **核心设计**：在 `lib/protobuf/protobuf.cp` 实现无 `.proto` 代码生成的零拷贝流式编解码器，严格校验 10 字节 Varint 溢出、0 号字段、`WireType` 合法性与 64 层子消息嵌套深度上限，并通过 `DebugAllocator` 确认零泄漏。

## 2026-11-21: 新建 `cap:snappy` Snappy Block Format 高速无损压缩与解压引擎（5/5 PASS，文档直写 `doc/lib/snappy.md`）
- **核心设计**：压缩器采用 14-bit 乘法哈希表实现 64KB 块内 $O(1)$ 前缀匹配；解压器完整支持 0..4 字节扩展字面量与 Copy-1/2/4 重叠拷贝，严格拦截 `offset=0`/越界偏移与超大 Varint 解压声明。

## 2026-11-21: 新建 `cap:ringhash` 一致性哈希环、Jump Hash 与 Rendezvous 负载均衡引擎（5/5 PASS，文档直写 `doc/lib/ringhash.md`）
- **核心设计**：`ConsistentRing` 与 `RendezvousHasher` 内部均独立拷贝 `node_id` 避免调用方切片悬垂，`ConsistentRing.getReplicas` 顺时针扫描相邻虚拟节点时严格按物理节点去重；同时提供无状态 $O(\ln N)$ `jumpConsistentHash`。

## 2026-11-21: 新建 `cap:base58` Base58 / Base58Check 与 Bech32/Bech32m 编码校验引擎（5/5 PASS，文档直写 `doc/lib/base58.md`）
- **核心设计**：`Base58` 保留前导零字节映射并复用 `cap:crypto` `sha256Raw` 完成 Double-SHA256 校验；`Bech32` 严格执行大小写不混用与 polymod 常数区分 `Bech32 (1)` / `Bech32m (0x2bc830a3)`，并提供通用 `convertBits` 位宽转换。

## 2026-11-21: 升级加固 `cap:expr` 安全业务规则与公式表达式求值引擎（5/5 PASS，文档直写 `doc/lib/expr.md`）
- **核心设计**：在 `lib/expr/expr.cp` 统一封装词法、Pratt/递归下降 AST 与求值上下文，采用解析期与求值期双重深度上限防御畸形深嵌套载荷，并通过 `std.testing.allocator` 确认 5/5 用例零泄漏。

## 2026-11-21: 升级加固 `cap:fsm` 确定性有限状态机 DFA、守卫回调与 GraphViz DOT 引擎（5/5 PASS，文档直写 `doc/lib/fsm.md`）
- **核心设计**：在 `lib/fsm/fsm.cp` 引入动态字符串状态/事件建模与 `GuardFn` 守卫、`TransitionRecord` 轨迹录制及 BFS 拓扑死胡同检测；清理旧 `state_machine.cp` 并通过 `DebugAllocator` 确认 5/5 用例零泄漏。

## 2026-11-21: 新建 `cap:wal` CRC32C 校验预写日志（WAL）帧编解码与崩溃恢复引擎（5/5 PASS，文档直写 `doc/lib/wal.md`）
- **核心设计**：采用 24 字节自描述帧头与 Castagnoli CRC32C（`0x82F63B78`）全帧保护，提供严格校验 `recoverStrict` 与掉电容错 `recoverWithTailTruncation` 双模式，并在截断恢复后支持原地 `restoreTruncated` 续写。

## 2026-11-21: 新建 `cap:hll` HyperLogLog 亚千字节超大规模基数近似估算引擎（5/5 PASS，文档直写 `doc/lib/hll.md`）
- **核心设计**：采用 SplitMix64 + FNV-1a 混合哈希提取桶索引与前导零计数，结合 LinearCounting 小基数修正与调和平均估算实现全量程 `< 3%` 高精度估算，并通过 `PrecisionMismatch` 与 `"HLL1"` 头校验保障分布式合并与反序列化安全。

## 2026-11-21: 新建 `cap:msgpack` MessagePack 二进制紧凑编解码与深度安全防护引擎（5/5 PASS，文档直写 `doc/lib/msgpack.md`）
- **核心设计**：编码器自动按值域选择最小字节表示（如 `fixint`/`fixstr`/`fixarray`），解码器零拷贝借用 `str`/`bin`/`ext` 原始切片并通过 `errdefer` 完整回收已构造嵌套容器，默认 64 层深度限制防止堆栈与资源耗尽。

## 2026-11-21: 新建 `cap:ratelimit` 多算法确定性限流与配额控制引擎（5/5 PASS，文档直写 `doc/lib/ratelimit.md`）
- **核心设计**：全量采用确定性单调毫秒 `now_ms: i64` 注入与时间回拨钳位，零系统调用；涵盖 `TokenBucket`、`SlidingWindowCounter`、`GcraLimiter` 与多租户 `KeyedRateLimiter`，`std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:nanoid` NanoID / 单调 ULID / RFC 9562 UUID v7 分布式短 ID 引擎（5/5 PASS，文档直写 `doc/lib/nanoid.md`）
- **核心设计**：采用位掩码拒绝采样消除非 2 次幂字母表的模数偏差；`MonotonicUlidGenerator` 在同毫秒内严格递增 80-bit 随机数并在全 `0xFF` 时返回 `MonotonicOverflow`；`UuidV7` 严格校验 RFC 9562 `version == 7` 与 `variant == 2`。

## 2026-11-21: 新建 `cap:tar` POSIX ustar 内存归档打包与安全解包引擎（5/5 PASS，文档直写 `doc/lib/tar.md`）
- **核心设计**：纯内存零系统调用实现 ustar 512 字节块打包与解包，自动将 100..256 字节路径拆分到 `prefix` 与 `name` 字段；解包器在 `errdefer` 路径完整清理已解析切片，并默认阻断 `..` 与前导 `/` 路径穿越。

## 2026-11-21: 新建 `cap:globset` 高性能多模式 Glob 路径匹配集合与 `.gitignore` 规则引擎（5/5 PASS，文档直写 `doc/lib/globset.md`）
- **核心设计**：`GlobMatcher` 采用线性 NFA 状态集模拟避免回溯指数爆炸；`GlobSetBuilder` 支持递归嵌套 `{a,{b,c}}` 展开；`IgnoreMatcher` 按规则序求值确保后置 `!` 否定规则可精准恢复被排除路径。

## 2026-11-21: 新建 `cap:cron` 5/6 字段 Cron 表达式解析与调度计算引擎（5/5 PASS，文档直写 `doc/lib/cron.md`）
- **核心设计**：采用紧凑位掩码存储与 Howard Hinnant O(1) 纯 UTC 日历分解算法，零系统调用即可完成闰年 `02-29`、跨月跨年跳跃搜索，并对不可达日期（如 `0 0 31 2 *`）在 28 年周期窗口内准确返回 `NoMatchingTime`。

## 2026-11-21: 新建 `cap:crdt` 分布式无冲突复制数据类型（CvRDT）引擎（5/5 PASS，文档直写 `doc/lib/crdt.md`）
- **核心设计**：按 `vclock`/`counters`/`register` 三模块解耦，`LwwRegister` 基于 `(timestamp_ms, node_id)` 全序破平局，`OrSet` 维护活跃与已移除 `(node_id, seq)` tag 集合确保多节点乱序广播强最终一致性（SEC）。

## 2026-11-21: 新建 `cap:circuit` 分布式弹性断路器、退避抖动与舱壁限流引擎（5/5 PASS，文档直写 `doc/lib/circuit.md`）
- **核心设计**：采用确定性 `now_ms: i64` 注入与环形滑动计数窗口实现零系统调用的三态断路器跃迁；`Bulkhead` 使用 `releasePermit()` 规避编译器 ARC `.release` 关键字冲突，保证 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:fx` 定点多币种金融货币运算与外汇三角换算引擎（5/5 PASS，文档直写 `doc/lib/fx.md`）
- **核心设计**：采用 `i128` 最小单位定点表示与 256 位中间乘除规避浮点误差和溢出，`allocate` 采用 Hamilton 最大余数法确保分配总额严格守恒；`FxBook` 自动推导逆汇率并在无直连时搜索 1 跳公共枢纽币种完成三角换算。

## 2026-11-21: 新建 `cap:geohash` 零分配 Geohash 空间网格与 Haversine 距离引擎（5/5 PASS，文档直写 `doc/lib/geohash.md`）
- **核心设计**：采用栈上定长 `GeohashCode` 与 `Neighbors` 结构体实现零堆分配编解码与 8 邻域查找，经度按 `±180°` 环绕、纬度按 `±90°` 极点钳位，并提供 64-bit Z-order Morton 整型索引接口。

## 2026-11-21: 新建 `cap:radix` 压缩基数树与高性能分段前缀路由引擎（5/5 PASS，文档直写 `doc/lib/radix.md`）
- **核心设计**：`RadixTree<T>` 在删除节点后自动检测非根孤立单子节点并合并边前缀，保持压缩树结构不变量；`RadixRouter<T>` 按 `/` 分段匹配并严格遵循 `static > :param > *wildcard` 优先级与回溯，保证 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:xorfilter` 极省空间静态 Xor8 / Xor16 近似集合过滤器（5/5 PASS，文档直写 `doc/lib/xorfilter.md`）
- **核心设计**：采用 SplitMix64 哈希混合与 3-peeling 超图剥离构建，自动排序去重并支持空集合边界；序列化采用 32 字节小端序自描述头（`"XOR8"`/`"XOR6"` + 版本 + `seed` + `block_length` + `item_count`），严格校验截断与非法头。

## 2026-11-21: 新建 `cap:hpack` RFC 7541 HTTP/2 HPACK 头部压缩编解码引擎（5/5 PASS，文档直写 `doc/lib/hpack.md`）
- **核心设计**：分离 `huffman`/`table`/`codec` 三模块，`table.cp` 使用 O(1) 环形缓冲管理动态表 FIFO 驱逐并严格遵循单条目超过 `max_size` 时清空动态表规则；`huffman.cp` 校验 EOS 符号与非全 1 或超 7-bit 填充，防御畸形输入。

## 2026-11-21: 新建 `cap:syslog` RFC 5424 / RFC 3164 结构化 Syslog 解析与格式化引擎（5/5 PASS，文档直写 `doc/lib/syslog.md`）
- **核心设计**：按 `types`/`parser`/`formatter` 三层解耦，`parse()` 基于 `<PRI>` 后首字符是否为版本号数字自动分流 RFC 5424 与 RFC 3164；`SD-PARAM` 严格处理 `\"`、`\\`、`\]` 转义往返与 UTF-8 BOM 剥离，保证 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:merkle` RFC 6962 域分离 Merkle 哈希树与双树差异定位引擎（5/5 PASS，文档直写 `doc/lib/merkle.md`）
- **核心设计**：叶子前缀 `0x00` 与内部节点前缀 `0x01` 严格域分离复用 `cap:crypto` `sha256Raw`，彻底消除二阶原像碰撞；奇数叶子层自动将末尾节点直接提升，并通过自顶向下哈希剪枝实现 $O(k \log N)$ 同规模/跨规模 `diffTrees` 差异叶子定位。

## 2026-11-21: 新建 `cap:cuckoo` 支持删除的布谷鸟过滤器（5/5 PASS，文档直写 `doc/lib/cuckoo.md`）
- **核心设计**：采用 4 槽位/桶与 16-bit 非零指纹（`0` 表示空槽），通过 `(i ^ hashFingerprint(fp)) & mask` 严格保证备选桶对合可逆；踢出路径达到 `MAX_KICKS = 500` 时沿记录路径逆向还原所有槽位，确保返回 `FilterFull` 时零元素丢失。

## 2026-11-21: 新建 `cap:mqtt` MQTT 3.1.1 二进制 Wire 报文与流式拆包引擎（5/5 PASS，文档直写 `doc/lib/mqtt.md`）

- **落地成果**：`lib/mqtt/`（`packet.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），与 `lib/paseto/` 合并回收提交，并以 `Model: "inherit"` 顶替派发 `cap:cuckoo`（支持删除 `delete` 的布谷鸟过滤器 Cuckoo Filter 近似成员集合引擎），与 `lib/merkle/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:paseto` PASETO v4.local & v4.public 安全令牌引擎（5/5 PASS，文档直写 `doc/lib/paseto.md`）

- **落地成果**：`lib/paseto/`（`paseto.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:merkle`（RFC 6962 域分离防二阶原像攻击 Merkle 哈希树、$O(\log N)$ 包含证明生成/验证与双树差异定位引擎），与 `lib/mqtt/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:lsm` LSM-Tree 存储引擎（5/5 PASS，文档直写 `doc/lib/lsm.md`）

- **落地成果**：`lib/lsm/`（`lsm.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），与 `lib/tlv/` 合并回收提交，并以 `Model: "inherit"` 顶替派发 `cap:paseto`（PASETO v4.local & v4.public 现代无歧义安全令牌引擎），与 `lib/mqtt/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:tlv` 二进制 TLV、分帧与 CRC32C 校验引擎（5/5 PASS，文档直写 `doc/lib/tlv.md`）

- **落地成果**：`lib/tlv/`（`tlv.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:mqtt`（MQTT 3.1.1 二进制 Wire 报文编解码、Varint 剩余长度与流式拆包引擎），与 `lib/lsm/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sheet`（5/5）与 `cap:tz`（5/5）双标准库全绿合并提交

- **落地成果**：`lib/sheet/`（公式 DAG 拓扑重算与 `#CYCLE!` 防护）与 `lib/tz/`（IANA/POSIX DST 时区互转）双双经 `verify_module.sh` 一键全绿通过（零内存泄漏），单轮合并回收提交，并以 `Model: "inherit"` 顶替派发 `cap:tlv`（二进制 TLV 编解码、黏包分帧与 Castagnoli CRC32C 校验引擎）与 `cap:lsm`（LSM-Tree MemTable + SSTable 分块索引 + Tombstone 多路归并压缩引擎），维持 2 路常规正交并行。

---

## 2026-11-21: 新建 `cap:ssh` OpenSSH 公钥指纹与 `authorized_keys` 策略引擎（5/5 PASS，文档直写 `doc/lib/ssh.md`）

- **落地成果**：`lib/ssh/`（`keys.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:tz`（IANA 常见时区、POSIX TZ DST 夏令时规则与 UTC <-> 本地时间互转引擎），与 `lib/sheet/` 保持 2 路常规并行。

---

## 2026-11-21: 新建 `cap:rule` 声明式生产规则与决策引擎（5/5 PASS，文档直写 `doc/lib/rule.md`）

- **落地成果**：`lib/rule/`（`rule.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sheet`（电子表格 `A1` 坐标解析、DAG 公式拓扑自动重算、`#CYCLE!` 环路拦截与 CSV/JSON 导出引擎），与 `lib/ssh/` 保持 2 路常规并行。

---

## 2026-11-21: 紧急热修 `cpc` 吞掉 `export fn` / `export const` 问题并结单交付 `bun_agy`

- **落地成果**：排查发现 `lexer.rs` 将 `export`/`pub` 均映射为 `Token::Export` 且顶层 `VarDecl` 缺少 `is_export` 导致跨 C/C++ ABI 导出丢失。修复后已发布 `cpc` Release 版并通过 `zig ast-check` 与 `test_basic.cp` 验证，完成 `resolve` + `ack` 结单交付并回收热修子智能体，恢复 2 路常规并行。

---

## 2026-11-21: 新建 `cap:http_cache` RFC 9111 HTTP 缓存与 `ETag` 协商引擎（5/5 PASS，文档直写 `doc/lib/http_cache.md`）

- **落地成果**：`lib/http_cache/`（`cache_policy.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:ssh`（OpenSSH 公钥 Wire 解析、`SHA256:` 指纹计算与 `authorized_keys` 策略匹配引擎），与 `lib/rule/` 保持 2 路常规并行。

---

## 2026-11-21: 新建 `cap:dedup` SimHash / MinHash / LSH 近重复去重引擎（5/5 PASS，文档直写 `doc/lib/dedup.md`）

- **落地成果**：`lib/dedup/`（`dedup.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:rule`（声明式业务风控与决策规则引擎：多条件树、Salience 优先级、互斥组与审计报告），与 `lib/http_cache/` 保持 2 路常规并行。

---

## 2026-11-21: 新建 `cap:highlight` 多语言词法高亮与 ANSI/HTML 渲染引擎（5/5 PASS，文档直写 `doc/lib/highlight.md`）

- **落地成果**：`lib/highlight/`（`highlight.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），与 `lib/vcard/` 合并提交，并以 `Model: "inherit"` 顶替派发 `cap:http_cache`（RFC 9111 HTTP `Cache-Control` 解析、强弱 `ETag` 304 条件协商与新鲜度寿命引擎），与 `lib/dedup/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:vcard` RFC 6350 vCard 通讯录解析与生成引擎（5/5 PASS，文档直写 `doc/lib/vcard.md`）

- **落地成果**：`lib/vcard/`（`vcard.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:dedup`（SimHash 海明分片索引 + MinHash + LSH 近重复文本与 RAG 语料去重引擎），与 `lib/highlight/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sat` DPLL 布尔 SAT 求解器与约束建模引擎（5/5 PASS，文档直写 `doc/lib/sat.md`）

- **落地成果**：`lib/sat/`（`solver.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:highlight`（多语言代码词法高亮引擎：CP/Zig/Rust/TS/Python/JSON/SQL/Bash 词法切分与 ANSI/HTML 渲染），与 `lib/vcard/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:tween` 缓动插值、关键帧与弹簧物理动画引擎（5/5 PASS，文档直写 `doc/lib/tween.md`）

- **落地成果**：`lib/tween/`（`tween.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:vcard`（RFC 6350/2426 vCard 3.0/4.0 电子名片通讯录 `.vcf` 解析与生成引擎），与 `lib/sat/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:noise` 确定性 Perlin/Worley/fBm 噪声引擎（5/5 PASS，文档直写 `doc/lib/noise.md`）

- **落地成果**：`lib/noise/`（`noise.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sat`（DPLL + 单位传播布尔可满足性 SAT 求解器与依赖/排班约束建模器），与 `lib/tween/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:spell` SymSpell / BK-Tree / Soundex 拼写纠错引擎（5/5 PASS，文档直写 `doc/lib/spell.md`）

- **落地成果**：`lib/spell/`（`spell.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:tween`（24+ Easing 缓动曲线、关键帧轨道插值与二阶弹簧阻尼物理动画引擎），与 `lib/noise/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ecs` Sparse-Set 实体组件系统 ECS 引擎（5/5 PASS，文档直写 `doc/lib/ecs.md`）

- **落地成果**：`lib/ecs/`（`ecs.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:noise`（确定性 2D/3D Perlin 梯度噪声、Voronoi Cellular 噪声与 fBm 分形地形生成引擎），与 `lib/spell/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:barcode` Code 128 / EAN-13 / UPC-A 条码编码引擎（5/5 PASS，文档直写 `doc/lib/barcode.md`）

- **落地成果**：`lib/barcode/`（`barcode.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:spell`（SymSpell 对称删除索引、BK-Tree 编辑距离树与 Soundex 语音编码拼写纠错引擎），与 `lib/ecs/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:pathfinding` 2D 网格 A* 与视线平滑寻路引擎（5/5 PASS，文档直写 `doc/lib/pathfinding.md`）

- **落地成果**：`lib/pathfinding/`（`grid.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:ecs`（Sparse-Set 架构世代防 ABA 实体组件系统 ECS 引擎），与 `lib/barcode/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:billing` SaaS 用量计费、周期折算与发票引擎（5/5 PASS，文档直写 `doc/lib/billing.md`）

- **落地成果**：`lib/billing/`（`billing.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:pathfinding`（2D 加权网格 A* 防穿角寻路、Dijkstra 距离场与 Bresenham 视线平滑引擎），与 `lib/barcode/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ledger` 复式记账双分录会计总账引擎（5/5 PASS，文档直写 `doc/lib/ledger.md`）

- **落地成果**：`lib/ledger/`（`ledger.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:barcode`（Code 128 Auto A/B/C 与 EAN-13/UPC-A 一维条码编码、校验位计算及矢量 SVG/ASCII 渲染引擎），与 `lib/billing/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sitemap` RFC 9309 `robots.txt` 与 XML Sitemap 引擎（5/5 PASS，文档直写 `doc/lib/sitemap.md`）

- **落地成果**：`lib/sitemap/`（`sitemap.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:billing`（SaaS & AI API 阶梯/累进用量计费、周期按比例折算 Proration、Coupon 折扣与发票生成引擎），与 `lib/ledger/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:lz4` 纯 CP LZ4 Block & Frame 压缩引擎（5/5 PASS，文档直写 `doc/lib/lz4.md`）

- **落地成果**：`lib/lz4/`（`lz4.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:ledger`（复式记账双分录会计总账、不可变冲正与试算平衡对账引擎），与 `lib/sitemap/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:mermaid` 声明式 Mermaid 图表构建引擎（5/5 PASS，文档直写 `doc/lib/mermaid.md`）

- **落地成果**：`lib/mermaid/`（`diagram.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sitemap`（RFC 9309 `robots.txt` 规则匹配与 XML Sitemap 0.9 / Sitemap Index 解析生成引擎），与 `lib/lz4/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bplus` B+ 树有序键值索引与范围扫描引擎（5/5 PASS，文档直写 `doc/lib/bplus.md`）

- **落地成果**：`lib/bplus/`（`bplus.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:lz4`（纯 CP LZ4 Block & Frame 极速压缩解压与 XXH32 校验引擎），与 `lib/mermaid/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bencode` BEP-3 Bencode 编解码与 Torrent InfoHash 引擎（5/5 PASS，文档直写 `doc/lib/bencode.md`）

- **落地成果**：`lib/bencode/`（`bencode.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:mermaid`（声明式 Flowchart / Sequence / ER / State Mermaid 图表构建与安全转义引擎），与 `lib/bplus/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:feed` RSS 2.0 / Atom 1.0 / JSON Feed 1.1 统一引擎（5/5 PASS，文档直写 `doc/lib/feed.md`）

- **落地成果**：`lib/feed/`（`feed.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bplus`（B+ 树有序键值索引、节点分裂合并与叶子链表高效 `rangeScan`/`prefixScan` 引擎），与 `lib/bencode/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:selector` CSS3 选择器编译器与 DOM 查询引擎（5/5 PASS，文档直写 `doc/lib/selector.md`）

- **落地成果**：`lib/selector/`（`selector.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bencode`（BitTorrent BEP-3 Bencode 严格编解码、规范字典序校验与 SHA-1 `InfoHash` 提取引擎），与 `lib/feed/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:optimize` 数值求根与多维函数最优化引擎（5/5 PASS，文档直写 `doc/lib/optimize.md`）

- **落地成果**：`lib/optimize/`（`opt.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:feed`（RSS 2.0 / Atom 1.0 / JSON Feed 1.1 自动识别、统一解析与生成引擎），与 `lib/selector/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:quadtree` 2D 空间四叉树索引与 KNN 引擎（5/5 PASS，文档直写 `doc/lib/quadtree.md`）

- **落地成果**：`lib/quadtree/`（`quadtree.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:optimize`（数值求根 Brent/Bisection、黄金分割 1D 极小化、Nelder-Mead 无导数多维最优化与 Adam 梯度优化器），与 `lib/selector/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:prob` 概率分布、随机采样与 A/B 显著性检验引擎（5/5 PASS，文档直写 `doc/lib/prob.md`）

- **落地成果**：`lib/prob/`（`dist.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:selector`（CSS3 选择器解析与 XML/HTML DOM 树查询匹配引擎），与 `lib/quadtree/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:geom2d` 2D 计算几何、凸包与碰撞检测引擎（5/5 PASS，文档直写 `doc/lib/geom2d.md`）

- **落地成果**：`lib/geom2d/`（`geom.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:quadtree`（2D 空间四叉树索引、矩形/圆形范围检索与 K 近邻查询引擎），与 `lib/prob/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:fft` Radix-2 FFT/IFFT 与频谱分析引擎（5/5 PASS，文档直写 `doc/lib/fft.md`）

- **落地成果**：`lib/fft/`（`fft.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），与 `lib/linalg/` 合并提交，并以 `Model: "inherit"` 顶替派发 `cap:geom2d`（2D 计算几何、Andrew 凸包、线段相交与碰撞检测引擎），与 `lib/prob/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:linalg` 通用稠密矩阵与数值线性代数引擎（5/5 PASS，文档直写 `doc/lib/linalg.md`）

- **落地成果**：`lib/linalg/`（`matrix.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:prob`（概率分布 PDF/CDF/分位数、蒙特卡洛采样与 A/B 测试显著性 Z-test/t-test 引擎），与 `lib/fft/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bpe` Byte-Level BPE Tokenizer 引擎（5/5 PASS，文档直写 `doc/lib/bpe.md`）

- **落地成果**：`lib/bpe/`（`bpe.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），与 `lib/regex/` 合并提交，并以 `Model: "inherit"` 顶替派发 `cap:fft`（Cooley-Tukey Radix-2 FFT/IFFT、窗函数、频谱功率与主频提取 DSP 引擎），与 `lib/linalg/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:regex` 抗 ReDoS 线性时间 Pike VM 正则引擎（5/5 PASS，文档直写 `doc/lib/regex.md`）

- **落地成果**：`lib/regex/`（`compiler.cp`/`vm.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:linalg`（通用任意维度稠密矩阵运算、列主元 LU/高斯消元求解、求逆、行列式与最小二乘线性拟合引擎），与 `lib/bpe/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:svg` 矢量 SVG 画布与统计图表引擎（5/5 PASS，文档直写 `doc/lib/svg.md`）

- **落地成果**：`lib/svg/`（`canvas.cp`/`charts.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bpe`（Byte-Level BPE Tokenizer 训练、合并规则编码、字节级无损解码与 Special Tokens 隔离引擎），与 `lib/regex/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sketch` HLL / T-Digest / 蓄水池概率素描引擎（5/5 PASS，文档直写 `doc/lib/sketch.md`）

- **落地成果**：`lib/sketch/`（`sketch.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:regex`（抗 ReDoS 线性时间 Pike VM / Thompson NFA 正则表达式编译器、捕获组与替换分割引擎），与 `lib/svg/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:intern` 字符串驻留池与双向映射表引擎（5/5 PASS，文档直写 `doc/lib/intern.md`）

- **落地成果**：`lib/intern/`（`intern.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sketch`（HyperLogLog 基数估算、流式 T-Digest/质心分位数与蓄水池等概率抽样引擎），与 `lib/svg/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ical` RFC 5545 iCalendar 与 RRULE 重复规则引擎（5/5 PASS，文档直写 `doc/lib/ical.md`）

- **落地成果**：`lib/ical/`（`ical.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:svg`（零 C 依赖矢量 SVG 绘图画布与折线/柱状/饼图数据可视化引擎），与 `lib/intern/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:zset` 跳表 + 哈希双索引有序集合引擎（5/5 PASS，文档直写 `doc/lib/zset.md`）

- **落地成果**：`lib/zset/`（`zset.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:intern`（高性能字符串驻留池 `StringInterner` + 双向唯一映射表 `BiMap` 引擎），与 `lib/ical/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:raft` 确定性 Raft 共识核心状态机（5/5 PASS，文档直写 `doc/lib/raft.md`）+ 热日志冷归档

- **落地成果**：`lib/raft/`（`raft.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，同时将 `CHANGELOG.md`/`DEVLOG.md` 早期记录安全滚动归档至 `*_H.md`；以 `Model: "inherit"` 顶替派发 `cap:ical`（RFC 5545 iCalendar `.ics` 解析生成与 `RRULE` 重复规则展开引擎），与 `lib/zset/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:agentmem` AI Agent 分层长期记忆与召回引擎（5/5 PASS，文档直写 `doc/lib/agentmem.md`）

- **落地成果**：`lib/agentmem/`（`memory.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:zset`（跳表 SkipList + 哈希双索引有序集合引擎：`zadd`/`zrank`/`zrangeByScore`/`zremRangeByScore` 等），与 `lib/raft/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:mime` MediaType 解析、`Accept` 协商与 `MimeDb` 引擎（5/5 PASS，文档直写 `doc/lib/mime.md`）

- **落地成果**：`lib/mime/`（`mime.cp`/`db.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:raft`（确定性无 I/O Raft 共识核心状态机：任期选举、日志新旧校验、多数派提交与防脑裂退位），与 `lib/agentmem/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:jsonpath` RFC 9535 JSONPath 编译器与查询引擎（5/5 PASS，文档直写 `doc/lib/jsonpath.md`）

- **落地成果**：`lib/jsonpath/`（`parser.cp`/`query.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:agentmem`（AI Agent 分层长期记忆引擎：Working/Episodic/Semantic 三层记忆、三因子召回评分与巩固遗忘机制），与 `lib/mime/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:lock` 读写锁、死锁检测与 Fencing 租约锁引擎（5/5 PASS，文档直写 `doc/lib/lock.md`）

- **落地成果**：`lib/lock/`（`wait_graph.cp`/`lock.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:mime`（RFC 6838 MediaType 解析、HTTP `Accept` 内容协商与 120+ 扩展名双向映射引擎），与 `lib/jsonpath/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:health` 健康探针与优雅停机协调器（5/5 PASS，文档直写 `doc/lib/health.md`）

- **落地成果**：`lib/health/`（`probe.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:jsonpath`（RFC 9535 JSONPath 递归下降、数组切片与过滤表达式查询引擎），与 `lib/lock/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:pool` 通用有界资源与连接池引擎（5/5 PASS，文档直写 `doc/lib/pool.md`）

- **落地成果**：`lib/pool/`（`pool.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:lock`（进程内按名逻辑读写锁、等待图死锁检测与 TTL/Fencing Token 租约锁引擎），与 `lib/health/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bitset` 动态位图与零堆固定位图引擎（5/5 PASS，文档直写 `doc/lib/bitset.md`）

- **落地成果**：`lib/bitset/`（`bitset.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:health`（云原生 Liveness/Readiness/Startup 健康探针汇聚与优雅停机 `ShutdownCoordinator` 引擎），与 `lib/pool/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:pipeline` 洋葱模型中间件与多阶段流水线引擎（5/5 PASS，文档直写 `doc/lib/pipeline.md`）

- **落地成果**：`lib/pipeline/`（`pipeline.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），与 `lib/uritemplate/` 合并收尾提交，并以 `Model: "inherit"` 顶替派发 `cap:pool`（通用资源与连接池引擎：空闲/最大生命周期驱逐、健康校验与泄漏观测），与 `lib/bitset/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:uritemplate` RFC 6570 URI Template 展开引擎（5/5 PASS，文档直写 `doc/lib/uritemplate.md`）

- **落地成果**：`lib/uritemplate/`（`template.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bitset`（字级 64-bit 向量化动态位图、硬件 Popcount/CTZ 快速迭代与集合布尔/Jaccard 运算引擎），与 `lib/pipeline/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ringbuf` 环形缓冲、字节环与微批聚合引擎（5/5 PASS，文档直写 `doc/lib/ringbuf.md`）

- **落地成果**：`lib/ringbuf/`（`ring.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:pipeline`（可组合中间件洋葱链、短路拦截与多阶段可观测执行管道），与 `lib/uritemplate/` 保持 2 路正交并行。

---

