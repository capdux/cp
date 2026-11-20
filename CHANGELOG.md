# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:csrf` 纯 CP HMAC-SHA256 CSRF 令牌、抗 BREACH 随机 XOR 掩码与 Origin/Referer/Sec-Fetch-Site 防护引擎（吸收对标 Bun `src/csrf/`，详见 `doc/lib/csrf.md`）（5/5 PASS，自动动态挂载 `cap.csrf`）
- **新增标准库**：`lib/csrf/{types,token,origin,index}.cp`（吸收对标 Bun `src/csrf/`），提供 HMAC-SHA256 常量时间签名验签与 TTL 过期校验、抗 BREACH 32B 随机 XOR 掩码令牌，以及严格 `scheme:host:port`（含 IPv6 与 `userinfo` 欺骗拦截）Origin 校验。
- **新增文档与测试**：`doc/lib/csrf.md` 扁平文档与 `cap/test/test_csrf_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:collections` 纯 CP 高性能 BitSet、无堆 BoundedDeque、RingDeque、内联溢出 SmallVec 与泛型 BinaryHeap 集合库（吸收对标 Bun `src/collections/`，详见 `doc/lib/collections.md`）（5/5 PASS，自动动态挂载 `cap.collections`）
- **新增标准库**：`lib/collections/{errors,bitset,deque,small_vec,binary_heap,index}.cp`（吸收对标 Bun `src/collections/`），提供 `BitSet` 集合代数与 `nextSetBit`、零堆 `BoundedDeque(T,N)`、`RingDeque(T)`、内联零堆自动溢出 `SmallVec(T,INLINE_CAP)` 与 `BinaryHeap(T,cmp)`。
- **新增文档与测试**：`doc/lib/collections.md` 扁平文档与 `cap/test/test_collections_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:perf` 纯 CP 高分辨率作用域计时、P95/P99 热点聚合与 Chrome Trace Event JSON 导出引擎（吸收对标 Bun `src/perf/`，详见 `doc/lib/perf.md`）（5/5 PASS，自动动态挂载 `cap.perf`）
- **新增标准库**：`lib/perf/{clock,types,recorder,stats,trace,index}.cp`（吸收对标 Bun `src/perf/`），提供单调/虚拟可注入时钟、`PerfRecorder` 嵌套 Span 采样、`summarizeByName` P95/P99 聚合及 `exportChromeTraceJson`（RFC 8259 转义）。
- **新增文档与测试**：`doc/lib/perf.md` 扁平文档与 `cap/test/test_perf_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:paths` 纯 CP POSIX/Windows 双风格路径解析、规范化与防目录穿越安全边界引擎（吸收对标 Bun `src/paths/`，详见 `doc/lib/paths.md`）（5/5 PASS，自动动态挂载 `cap.paths`）
- **新增标准库**：`lib/paths/{types,parse,ops,security,index}.cp`（吸收对标 Bun `src/paths/`），提供 POSIX/Windows 盘符与 UNC 路径零堆切片解析、`normalize`/`join`/`resolve`/`relative`、`safeJoin`/`isSubpath` 防穿越与空字节拦截及 `matchGlobSimple`。
- **新增文档与测试**：`doc/lib/paths.md` 扁平文档与 `cap/test/test_paths_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:tlv` 纯 CP ASN.1 BER/DER Tag-Length-Value 严格编解码、OID 双向互转与流式构建引擎（支持 `cap.tlv` 自动动态挂载，详见 `doc/lib/tlv.md`）（5/5 PASS，自动动态挂载 `cap.tlv`）
- **升级/重构标准库**：`lib/tlv/{types,oid,decoder,encoder,index}.cp`（移除旧 `tlv.cp`），实现单/高编号 Base-128 Tag、严格 DER 长度/不定长/深度防护解码、点分 OID 双向互转及整数最小补码规范化 `TlvWriter`。
- **更新文档与测试**：`doc/lib/tlv.md` 扁平文档与 `cap/test/test_tlv_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:semver_range` 纯 CP Semver 2.0 语义化版本解析、npm/Cargo 范围表达式求值与选版引擎（支持 `cap.semver_range` 自动动态挂载，详见 `doc/lib/semver_range.md`）（5/5 PASS，自动动态挂载 `cap.semver_range`）
- **新增标准库**：`lib/semver_range/{types,version,range,selector,index}.cp`，实现严格 Semver 2.0 解析比较、Caret/Tilde/通配符/连字符区间/`||` 并集范围求值、默认预发布隔离与 `maxSatisfying`/`minSatisfying`/`intersects` 选版引擎。
- **新增文档与测试**：`doc/lib/semver_range.md` 扁平文档与 `cap/test/test_semver_range_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ulid` 纯 CP Crockford Base32 128-bit 单调递增 ULID 编解码、UUID 互转与 CSPRNG 生成器引擎（支持 `cap.ulid` 自动动态挂载，详见 `doc/lib/ulid.md`）（5/5 PASS，自动动态挂载 `cap.ulid`）
- **新增标准库**：`lib/ulid/{types,codec,ulid,generator,index}.cp`，实现零堆分配 Crockford Base32 编解码（严格/容错双模式与首字符 `<=7` 128-bit 溢出拦截）、`MonotonicUlidGenerator` 同毫秒 80-bit 单调递增与 `toUuidString`/`fromUuidString` 双向无损互转。
- **新增文档与测试**：`doc/lib/ulid.md` 扁平文档与 `cap/test/test_ulid_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:graph` 纯 CP 有向加权图、DAG 拓扑排序、显式栈 Tarjan SCC 与 Dijkstra/Bellman-Ford 最短路径引擎（详见 `doc/lib/graph.md`）（5/5 PASS，自动动态挂载 `cap.graph`）
- **升级标准库**：`lib/graph/{graph,directed_graph,dag_algorithms,index}.cp`，在保留既有 `Graph` 的基础上新增稠密索引 `DirectedGraph`、`topologicalSort`（Kahn）、非递归三色 `findCycle`、迭代式 `tarjanScc`、`dijkstra` 与支持负权环检测的 `bellmanFord`。
- **更新文档与测试**：`doc/lib/graph.md` 扁平文档与 `cap/test/test_graph_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:idna` 纯 CP RFC 3492 Punycode 编解码与 IDNA 国际化域名转换校验引擎（支持 `cap.idna` 自动动态挂载，详见 `doc/lib/idna.md`）（5/5 PASS，自动动态挂载 `cap.idna`）
- **新增标准库**：`lib/idna/{punycode,idna,index}.cp`，实现严格 RFC 3492 Bootstring 编解码（含全路径 `u32`/码点溢出保护）、`toAscii`/`toUnicode`（支持 `.` 及全角等价分隔符）、零堆分配 `isValidDomain` 与 RFC 1034/1123 长度/连字符/双向控制符校验。
- **新增文档与测试**：`doc/lib/idna.md` 扁平文档与 `cap/test/test_idna_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bencode` 纯 CP BEP-3 BitTorrent Bencode 严格编解码、规范化字典排序与零拷贝 InfoHash SHA-1 引擎（支持 `cap.bencode` 自动动态挂载，详见 `doc/lib/bencode.md`）（5/5 PASS，自动动态挂载 `cap.bencode`）
- **新增标准库**：`lib/bencode/{types,codec,torrent,index}.cp`，严格校验 BEP-3 前导零/负零/乱序重复键/64 层深度上限，支持字典自动排序编码及零 AST 分配 `.torrent` `"info"` 切片提取与 `computeInfoHashSha1`。
- **新增文档与测试**：`doc/lib/bencode.md` 扁平文档与 `cap/test/test_bencode_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:websocket_frame` 纯 CP RFC 6455 WebSocket 帧协议、XOR 掩码、分片重组与控制帧交织引擎（支持 `cap.websocket_frame` 自动动态挂载，详见 `doc/lib/websocket_frame.md`）（5/5 PASS，自动动态挂载 `cap.websocket_frame`）
- **新增标准库**：`lib/websocket_frame/{types,codec,assembler,index}.cp`，实现 RFC 6455 `computeAcceptKey`、7/16/64-bit 帧编码与 4B 循环掩码、`decodeFrame` 及支持 1..N 字节半包黏包与分片中途交织控制帧的 `WsStreamAssembler`。
- **新增文档与测试**：`doc/lib/websocket_frame.md` 扁平文档与 `cap/test/test_websocket_frame_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:oauth2` 纯 CP RFC 6749/7636 OAuth 2.0、PKCE S256 与 OIDC 协议引擎（支持 `cap.oauth2` 自动动态挂载，详见 `doc/lib/oauth2.md`）（5/5 PASS，自动动态挂载 `cap.oauth2`）
- **新增标准库**：`lib/oauth2/{types,pkce,authorize,token,index}.cp`，实现拒绝采样 CSPRNG PKCE `S256`、常量时间 `state`/Challenge 校验、RFC 3986 授权 URL 与回调解析、三大 Grant/Revocation 表单编码及 `TokenResponse` 解析。
- **新增文档与测试**：`doc/lib/oauth2.md` 扁平文档与 `cap/test/test_oauth2_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:histogram` 纯 CP HDR 延迟直方图与分位统计引擎（5/5 PASS，自动动态挂载 `cap.histogram`）
- **新增标准库**：`lib/histogram/{hdr,ewma,export,index}.cp`，实现 Gil Tene 对数-线性子分桶 `HdrHistogram`（1..5 位有效数字精度、`recordValueWithExpectedInterval`、`merge`、P50/P95/P99/P99.9/P99.99）、`Ewma`（1m/5m/15m）、固定环形槽位 `SlidingWindowCounter` 及 `formatSummary`/`formatPrometheusBuckets` 导出。
- **新增文档与测试**：`doc/lib/histogram.md` 扁平文档与 `cap/test/test_histogram_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:unicode` 纯 CP UTF-8/UTF-16、UAX #29 Grapheme 与终端显示宽度引擎（5/5 PASS，自动动态挂载 `cap.unicode`）
- **新增标准库**：`lib/unicode/{utf,props,grapheme,width,index}.cp`（吸收对标 Bun `src/unicode/`），实现 UTF-8/UTF-16 代理对编解码、Unicode 空白/标识符/大小写属性、零分配 `GraphemeIterator`（ZWJ/国旗/组合音标/CRLF）及跳过 ANSI 转义的 `displayWidth`/`truncateByWidth`。
- **新增文档与测试**：`doc/lib/unicode.md` 扁平文档与 `cap/test/test_unicode_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:picohttp` 纯 CP 零堆分配 HTTP/1.1 & Chunked 流式解析引擎（5/5 PASS，自动动态挂载 `cap.picohttp`）
- **新增标准库**：`lib/picohttp/{picohttp,index}.cp`，基于 caller-provided 缓冲实现 RFC 9110/9112 `parseRequest`/`parseResponse`/`parseHeaders`、`ChunkedDecoder`（支持任意 `1..N` 字节半包跨 chunk、`chunk-ext` 与 Trailer 跳过）以及 `findHeader`/`parseContentLength` 冲突校验。
- **新增文档与测试**：`doc/lib/picohttp.md` 扁平文档与 `cap/test/test_picohttp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:interchange` 纯 CP JSON/JSONC/JSON5/TOML/YAML 统一数据交换引擎（5/5 PASS，自动动态挂载 `cap.interchange`）
- **新增标准库**：`lib/interchange/{value,json5,bridge,interchange,index}.cp`（吸收对标 Bun `src/interchange/`），提供统一 `InterchangeValue`（含 `max_depth = 64`、`getPath`、`clone`、`equals`）、原生 JSON5/JSONC 解析序列化，并桥接复用 `cap:toml`/`cap:yaml` 实现五格式统一 `parse`/`parseAuto`/`stringify`/`convert`。
- **新增文档与测试**：`doc/lib/interchange.md` 扁平文档与 `cap/test/test_interchange_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:valkey` 纯 CP RESP2/RESP3 Valkey/Redis 协议编解码与增量流式解析引擎（5/5 PASS，自动动态挂载 `cap.valkey`）
- **新增标准库**：`lib/valkey/{valkey,index}.cp`（吸收对标 Bun `src/valkey/`），完整建模 RESP2/RESP3 14 种核心类型（含 `Map`/`Set`/`Push`/`VerbatimString`/`BigNumber`/`Double` 与 `Attribute` 跳过），提供 `RespStreamParser` 增量半包黏包解析、`encodeCommand`/`encodeValue` 及 `max_depth`/`max_bulk_len` 防资源耗尽守卫。
- **新增文档与测试**：`doc/lib/valkey.md` 扁平文档与 `cap/test/test_valkey_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

