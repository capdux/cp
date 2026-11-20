# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:color` 色彩科学、WCAG 2.1 无障碍与终端 ANSI 配色引擎（5/5 PASS，自动动态挂载 `cap.color`）

- 新建 [`lib/color/`](file:///data/dev/cp/lib/color/index.cp)（`spaces.cp`、`wcag.cp`、`ansi.cp`、`index.cp`）：`Rgba`/`Hsl`/`Hsv`/`Oklab`/`Oklch` 双向转换与感知插值 `mixOklab`、WCAG 2.1 对比度/AA/AAA/`bestTextColor` 与 `Palette`、ANSI TrueColor/xterm-256 量化与 `stripAnsi`；文档 [`doc/lib/color.md`](file:///data/dev/cp/doc/lib/color.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_color_production_deep.cp`](file:///data/dev/cp/cap/test/test_color_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:crdt` 分布式无冲突复制数据类型套件（5/5 PASS，自动动态挂载 `cap.crdt`）

- 新建 [`lib/crdt/`](file:///data/dev/cp/lib/crdt/index.cp)（`vclock.cp`、`counters.cp`、`register.cp`、`index.cp`）：`VectorClock`（因果偏序 `Before`/`After`/`Equal`/`Concurrent`）、`GCounter`/`PnCounter`（交换/结合/幂等合并）、`LwwRegister`（`(timestamp, node_id)` 决胜）与 `OrSet`（Observed-Remove 并发 add-wins）；文档 [`doc/lib/crdt.md`](file:///data/dev/cp/doc/lib/crdt.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_crdt_production_deep.cp`](file:///data/dev/cp/cap/test/test_crdt_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 3 路新标准库 `cap:cluster`/`cap:geo`/`cap:graph` 全绿落地（15/15 PASS，全部归口 `doc/lib/*.md` 并清零旧 `docs/` 目录）

- **`cap:cluster`**（[`lib/cluster/`](file:///data/dev/cp/lib/cluster/index.cp)）：虚拟节点二分查找 `ConsistentHashRing`（含物理节点去重 `getReplicaNodes`）、SWIM `incarnation` CRDT `MembershipTable` 及单调 `fencing_token` 防脑裂 `LeaderLease`，文档 [`doc/lib/cluster.md`](file:///data/dev/cp/doc/lib/cluster.md)，`-m` 5/5 PASS；
- **`cap:geo`**（[`lib/geo/`](file:///data/dev/cp/lib/geo/index.cp)）：`LatLng`/`BBox`/`Geo`（Haversine 测距/方位角/目标点投影/多边形地理围栏）、Base32 `Geohash`（1..12 位编解码 + 8 邻域）与 `SpatialIndex`（`withinRadius`/`nearestK`），文档 [`doc/lib/geo.md`](file:///data/dev/cp/doc/lib/geo.md)，`-m` 5/5 PASS；
- **`cap:graph`**（[`lib/graph/`](file:///data/dev/cp/lib/graph/index.cp)）：有向无向加权图 `Graph`（BFS/DFS/GraphRAG `kHopSubgraph`）及 `GraphAlgo`（`dijkstra`/`topologicalSort`/迭代式 `tarjanScc`/`kruskalMst`/`pageRank`），文档 [`doc/lib/graph.md`](file:///data/dev/cp/doc/lib/graph.md)，`-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-21] - 文档目录非复数化与扁平化重构：`docs/<mod>/<mod>.md` → `doc/lib/<mod>.md`

- 将原 `docs/` 统一重构为单数 `doc/`，全部 85 份标准库文档扁平化归口至 [`doc/lib/`](file:///data/dev/cp/doc/lib/)（消除 1 文件同名子目录，清理 `docs/pack/zlig.md` 拼写残留及 `docs/stdlib/` 16 份历史重复旧稿，保留 4 份独有文档并迁入 `doc/lib/`）；
- 全量同步更新 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)、[`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md) 与 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 中的文档路径引用（`docs/` 引用 0 残留）。

---

## [2026-11-21] - 新建 `cap:queue` 三堆 O(log N) 优先级/延迟/租约超时/死信任务队列引擎（5/5 PASS，自动动态挂载 `cap.queue`）

- 新建 [`lib/queue/`](file:///data/dev/cp/lib/queue/index.cp)（`job.cp`、`scheduler.cp`、`index.cp`）：`JobPriority`/`RetryPolicy`/`Job` 与 `JobQueue`（ReadyHeap/DelayedHeap/LeaseHeap 三堆 O(log N) 调度、延迟到期回迁、僵尸租约回收、指数退避重试、DLQ 死信与 `requeueDeadLetter`）；同步 [`docs/queue/queue.md`](file:///data/dev/cp/docs/queue/queue.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_queue_production_deep.cp`](file:///data/dev/cp/cap/test/test_queue_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:kv` 零 C 依赖 CRC32 WAL + Bitcask 崩溃安全持久化嵌入式存储（5/5 PASS，自动动态挂载 `cap.kv`）

- 新建 [`lib/kv/`](file:///data/dev/cp/lib/kv/index.cp)（`wal.cp`、`store.cp`、`index.cp`）：`WalCodec`（`"CPKV"` + IEEE CRC32 校验 + `TornTail`/`CorruptRecord` 区分）与 `KvStore`（`open`/`put`/`get`/`delete`/`prefixScan`/`sync`、模拟崩溃尾部 `ftruncate` 自动恢复、`renameat2` 原子 `compact()` 回收死记录）；同步 [`docs/kv/kv.md`](file:///data/dev/cp/docs/kv/kv.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_kv_production_deep.cp`](file:///data/dev/cp/cap/test/test_kv_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:expr`（5/5）与 `cap:protobuf`（5/5）双标准库落地（自动动态挂载 `cap.expr` / `cap.protobuf`）

- **`cap:expr`**（[`lib/expr/`](file:///data/dev/cp/lib/expr/index.cp)）：`ExprValue`/`ExprEnv` 只读点分变量上下文、爬山法优先级求值器（三元/短路 `&&`/`||`/`in`/9 个内置纯函数）及 `MAX_DEPTH=64`/`MAX_STEPS=10000` 沙盒防护，[`test_expr_production_deep.cp`](file:///data/dev/cp/cap/test/test_expr_production_deep.cp) `-m` 5/5 PASS；
- **`cap:protobuf`**（[`lib/protobuf/`](file:///data/dev/cp/lib/protobuf/index.cp)）：Proto3 零拷贝 `ProtoWriter`/`ProtoReader`（Varint/ZigZag/Packed Repeated/`skipField` 与 11B 溢出/保留区防护）及 `GrpcFrameCodec`/`GrpcFrameStream`/`GrpcStatus`/`GrpcTimeout`，[`test_protobuf_production_deep.cp`](file:///data/dev/cp/cap/test/test_protobuf_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 新建 `cap:cbor` RFC 8949 CBOR & MessagePack 紧凑二进制序列化引擎（5/5 PASS，自动动态挂载 `cap.cbor`）

- 新建 [`lib/cbor/`](file:///data/dev/cp/lib/cbor/index.cp)（`value.cp`、`cbor.cp`、`msgpack.cp`、`index.cp`）：共享 `BinValue`/`BinDocument` AST，完整支持 RFC 8949 CBOR Major 0-7（含 Float16/32/64、语义 Tag 与不定长编解码）及 MessagePack 全类型编解码，内置 64 层深度与截断/尾随字节防护；同步 [`docs/cbor/cbor.md`](file:///data/dev/cp/docs/cbor/cbor.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_cbor_production_deep.cp`](file:///data/dev/cp/cap/test/test_cbor_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:xml`（5/5）与 `cap:metrics`（5/5）双标准库落地（自动动态挂载 `cap.xml` / `cap.metrics`）

- **`cap:xml`**（[`lib/xml/`](file:///data/dev/cp/lib/xml/index.cp)）：XML 1.0 DOM 解析（声明/注释/CDATA/自闭合/命名空间/实体解码与 128 层防护）、`findByPath`/`findAllChildren`/`innerText` 查询及紧凑/美化 `XmlWriter` 与 `XmlSerializer`，[`test_xml_production_deep.cp`](file:///data/dev/cp/cap/test/test_xml_production_deep.cp) `-m` 5/5 PASS；
- **`cap:metrics`**（[`lib/metrics/`](file:///data/dev/cp/lib/metrics/index.cp)）：无锁原子 `Counter`、CAS `f64` `Gauge`、累计桶 `MetricsHistogram`（含 `estimateQuantile`）及 `MetricsRegistry`（Prometheus 0.0.4 族聚合/转义导出与 JSON 快照），[`test_metrics_production_deep.cp`](file:///data/dev/cp/cap/test/test_metrics_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 新建 `cap:text` AI RAG 分块、模糊匹配与 LLM Token 预算引擎（5/5 PASS，自动动态挂载 `cap.text`）

- 新建 [`lib/text/`](file:///data/dev/cp/lib/text/index.cp)（`chunker.cp`、`fuzzy.cp`、`tokens.cp`、`index.cp`）：UTF-8 零截断递归/Markdown 章节分块（自动跳过围栏代码块）、Unicode 码点级 Levenshtein/Damerau-Levenshtein/Jaro-Winkler 与 `fuzzyTopK`、快速 LLM `TokenEstimator` 与 UTF-8 安全截断；同步 [`docs/text/text.md`](file:///data/dev/cp/docs/text/text.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_text_production_deep.cp`](file:///data/dev/cp/cap/test/test_text_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:vector` 本地 SIMD 向量索引 + Okapi BM25 + RRF 混合 RAG 搜索引擎（5/5 PASS）

- 新建 [`lib/vector/`](file:///data/dev/cp/lib/vector/index.cp)（`flat_index.cp`、`bm25.cp`、`hybrid.cp`、`index.cp`）：复用 `cap:math` SIMD 算子的 `VectorIndex`（Cosine/Dot/Euclidean + O(1) Swap-Remove）、`Bm25Index`（Okapi BM25 词频饱和与平滑 IDF）及 `RrfFusion`（Reciprocal Rank Fusion 混合检索去重提权）；同步 [`docs/vector/vector.md`](file:///data/dev/cp/docs/vector/vector.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_vector_production_deep.cp`](file:///data/dev/cp/cap/test/test_vector_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:fsm`（5/5）与 `cap:cache`（5/5）双标准库落地（自动动态挂载 `cap.fsm` / `cap.cache`）

- **`cap:fsm`**（[`lib/fsm/`](file:///data/dev/cp/lib/fsm/index.cp)）：确定性 `StateMachine`（审计历史 + `toMermaid`）、三态 `CircuitBreaker`（`Closed`/`Open`/`HalfOpen` + 探针限流与统计）及 `WorkflowDag`（Kahn 分层并行 `executionWaves`、环路拦截、`readyNodes` 与上游失败 BFS 级联 `Skipped`），[`test_fsm_production_deep.cp`](file:///data/dev/cp/cap/test/test_fsm_production_deep.cp) `-m` 5/5 PASS；
- **`cap:cache`**（[`lib/cache/`](file:///data/dev/cp/lib/cache/index.cp)）：O(1) 槽位池 `LruCache<V>`（TTL + `evictExpired` + `CacheStats`）、最优 `m`/`k` 双哈希 `BloomFilter`（含 `unionWith`）及无低估 `CountMinSketch`（含 `merge`），[`test_cache_production_deep.cp`](file:///data/dev/cp/cap/test/test_cache_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 3 路新标准库 `cap:math`/`cap:schema`/`cap:rpc` 全绿落地与子智能体分级模型选型固化

- **分级模型选型固化**（[`GEMINI.md`](file:///data/dev/cp/GEMINI.md)）：明确常规 3 路标准库并行子智能体使用 `Model: "flash"`（Gemini 4 Hydrogen），疑难 `cpc` 编译器热修子智能体使用 `Model: "inherit"`（继承主控模型），修完即收；
- **`cap:math`**（[`lib/math/`](file:///data/dev/cp/lib/math/index.cp)）：`Vec2/3/4` 与列主序 `Mat4`、Welford 单遍统计/分位数/皮尔逊相关与线性回归、8-lane `@Vector(8, f32)` SIMD 向量相似度、`softmaxInPlace`、`topKIndices` 及对称 Int8 量化，[`test_math_production_deep.cp`](file:///data/dev/cp/cap/test/test_math_production_deep.cp) `-m` 5/5 PASS；
- **`cap:schema`**（[`lib/schema/`](file:///data/dev/cp/lib/schema/index.cp)）：JSON Schema Draft 2020-12 流式 `Schema` 与 `Schema.fromJson`（兼容 OpenAI/MCP Tool Calling）、`SchemaValidator`（5 种内置格式、`required`/`additionalProperties`/`uniqueItems`/64 层深度防护与 RFC 6901 JSON Pointer 路径诊断），[`test_schema_production_deep.cp`](file:///data/dev/cp/cap/test/test_schema_production_deep.cp) `-m` 5/5 PASS；
- **`cap:rpc`**（[`lib/rpc/`](file:///data/dev/cp/lib/rpc/index.cp)）：JSON-RPC 2.0 编解码（`JsonRpcCodec`）、LSP/MCP `ContentLengthFramer` 与 `LineRpcFramer` 增量分帧及 `McpServer` 全链路分发器（`initialize`/`ping`/`tools/list`/`tools/call`/`resources/list`），[`test_rpc_production_deep.cp`](file:///data/dev/cp/cap/test/test_rpc_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 新建 `cap:template` Mustache 兼容模板引擎（5/5 PASS，自动动态挂载 `cap.template`）

- 新建 [`lib/template/`](file:///data/dev/cp/lib/template/index.cp)（`value.cp`、`engine.cp`、`index.cp`）：支持 `TemplateValue`/`TemplateContext` 点分路径与父子作用域栈、`{{var}}` HTML 转义与 `{{{raw}}}`/`{{&raw}}`/`renderRaw`、`{{#section}}`/`{{^inverted}}`/`{{! comment}}`/`{{> partial}}`（含最大递归深度防护与独立行空白修剪）；同步 [`docs/template/template.md`](file:///data/dev/cp/docs/template/template.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_template_production_deep.cp`](file:///data/dev/cp/cap/test/test_template_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:csv`（5/5）与 `cap:uuid`（5/5）双标准库落地（零改动编译器自动动态挂载 `cap.csv` / `cap.uuid`）

- **`cap:csv`**（[`lib/csv/`](file:///data/dev/cp/lib/csv/index.cp)）：RFC 4180 增量 `StreamingCsvReader`（跨 chunk BOM/引号换行/双引号转义/列校验/`max_record_bytes` 防护）、`CsvWriter`（CSV/TSV 与公式注入防护）及 `CsvTable`（表头索引、过滤与 `toJsonLines` NDJSON 导出），[`test_csv_production_deep.cp`](file:///data/dev/cp/cap/test/test_csv_production_deep.cp) `-m` 5/5 PASS；
- **`cap:uuid`**（[`lib/uuid/`](file:///data/dev/cp/lib/uuid/index.cp)）：RFC 9562 `Uuid`（v4 CSPRNG、v5 SHA-1 命名空间、v7 同毫秒单调自旋锁可排序）、`Ulid`（Crockford Base32 26 字符含 `I/L->1` `O->0` 与 UUID 互转）及 `NanoId`（无偏拒绝采样），[`test_uuid_production_deep.cp`](file:///data/dev/cp/cap/test/test_uuid_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 3 路正交并行首批收官：`cap:bench`（9/9）、`cap:security`（9/9）与 `cap:dns`（10/10）全绿验收落地

- **`cap:bench`**（[`lib/bench/`](file:///data/dev/cp/lib/bench/index.cp)）：升级 `suite.cp`（`blackBox<T>`、`BenchOptions`/`BenchResult`/`BenchRunner` 预热与 Welford 分位数统计），新增 `compare.cp`（Welch t-test `BenchComparison` 与 Markdown 报告）及 `alloc_tracker.cp`（`CountingAllocator`/`AllocSnapshot`），[`test_bench_production_deep.cp`](file:///data/dev/cp/cap/test/test_bench_production_deep.cp) `-m` 5/5 + 既有 4/4 PASS；
- **`cap:security`**（[`lib/security/`](file:///data/dev/cp/lib/security/index.cp)）：新增 `totp.cp`（RFC 4648 Base32、RFC 4226 HOTP、RFC 6238 TOTP 与 `otpauth://`）、`cors.cp`（Fetch CORS 策略引擎与凭据+通配符防线）及 `csp.cp`（`CspBuilder` 防 CRLF 注入 + `SecurityHeadersPreset`），[`test_security_production_deep.cp`](file:///data/dev/cp/cap/test/test_security_production_deep.cp) `-m` 5/5 + 既有 `test_security_csrf.cp` 4/4 PASS；
- **`cap:dns`**（[`lib/dns/`](file:///data/dev/cp/lib/dns/index.cp)）：新增 `records.cp`（16 跳防环 `DnsNameCodec` + IPv4/IPv6 PTR、SOA/SRV+权重排序/CAA）、`doh.cp`（RFC 8484 Base64URL GET/POST 与 JSON DoH 解析）及 `resolv_conf.cp`（POSIX `/etc/resolv.conf` 与 `ndots` 搜索域展开），[`test_dns_production_deep.cp`](file:///data/dev/cp/cap/test/test_dns_production_deep.cp) `-m` 5/5 + 既有 `test_crypto_sql_dns.cp` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)（零读写 `cp_ref.md`/`cp_agent_ref.md`）。

---

## [2026-11-20] - 研发提速治理固化（`GEMINI.md` / `cp_lib_ref.md` / `verify_module.sh` / 动态 `cap` 路由）与 `cap:html` + `cap:json` 双路落地

### 研发提速体系与三轨解耦规范落地
- **剥离标准库独立契约手册**：新建 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，从 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md) 第十二章剥离全部内置库索引；后续新增/吸收标准库只改 `cp_lib_ref.md` 与 `docs/<mod>/<mod>.md`，日常不读写 `cp_ref.md`；
- **固化提速与治理铁律**：更新 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 消除旧双轨冲突条款，固化三轨解耦规范（`cp_agent_ref.md` 语法修改打 `[SYNC_PENDING]` 标记）、96 核 3 路正交并行、子智能体“最后一步导出 `index.cp`”原子发布纪律与三段式极速调试；
- **工程提速脚本与原子部署**：新增 [`scripts/verify_module.sh`](file:///data/dev/cp/scripts/verify_module.sh) 一键合规与 `-m` 验收脚本；升级 [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh) 为临时文件 + `mv -f` 原子覆盖部署；升级 [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) 支持 `lib/<ns>/index.cp` 自动动态探测挂载（新增标准库 0 改动 Rust、免 45s 重编）。

### `cap:html` HTML5 实体/白名单清洗/AI 网页提取升级 (`lib/html/`，5/5 PASS)
- 新增 [`lib/html/entities.cp`](file:///data/dev/cp/lib/html/entities.cp)（206 个 WHATWG 命名实体二分查找、十/十六进制数字引用、C1 Windows-1252 替换与 U+FFFD 兜底）、[`lib/html/sanitizer.cp`](file:///data/dev/cp/lib/html/sanitizer.cp)（白名单清洗、`on*` 剥离、`javascript:`/`vbscript:`/`data:*` 与实体编码绕过拦截）与 [`lib/html/extractor.cp`](file:///data/dev/cp/lib/html/extractor.cp)（AI Agent 单遍流式提取 title/meta/OG/canonical/links/headings 与干净纯文本）；新增 [`cap/test/test_html_production_deep.cp`](file:///data/dev/cp/cap/test/test_html_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有回归 4/4 PASS）。

### `cap:json` RFC 6901/6902/7396/8785 与 NDJSON 流式引擎升级 (`lib/json/`，5/5 PASS)
- 新增 [`lib/json/pointer.cp`](file:///data/dev/cp/lib/json/pointer.cp)（RFC 6901 `JsonPointer`、RFC 6902 `JsonPatch` 原子应用、RFC 7396 `JsonMergePatch`）、[`lib/json/ndjson.cp`](file:///data/dev/cp/lib/json/ndjson.cp)（跨 chunk `NdjsonReader` 含 `LineTooLong` 防护与 `NdjsonWriter`），升级 [`lib/json/serializer.cp`](file:///data/dev/cp/lib/json/serializer.cp)（RFC 8785 Canonical 排序序列化与 HTML 安全转义）；新增 [`cap/test/test_json_rfc_production_deep.cp`](file:///data/dev/cp/cap/test/test_json_rfc_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有回归 9/9 PASS）。

---

## [2026-11-20] - `cap:http` Web/AI Streaming 原语（+1,713 行）与 `cap:logger` 结构化日志/W3C Trace 追踪（+1,671 行）双路落地

### `cap:http` 生产级 Web / AI Streaming 协议原语升级 (`lib/http/`，0 `std.posix`，0 `@import("bun")`)
- 新增 [`lib/http/sse.cp`](file:///data/dev/cp/lib/http/sse.cp)（486 行）：WHATWG Server-Sent Events / LLM 流式解析器与编码器（`SseEvent`、`SseParser` 跨 chunk 切分/BOM 剥离/多行 `data:` 拼接/`[DONE]` 识别/单事件大小 DoS 上限防护、`SseEncoder`）；
- 新增 [`lib/http/multipart.cp`](file:///data/dev/cp/lib/http/multipart.cp)（417 行）：RFC 7578 `multipart/form-data` 二进制安全打包与解析器（`MultipartBoundary`、`MultipartBuilder`、`MultipartParser`）；
- 升级 [`lib/http/cookie.cp`](file:///data/dev/cp/lib/http/cookie.cp)（810 行）：保留原有 `CookieMap` 兼容，新增 RFC 6265 `SameSite`/`HttpCookie`/`CookieCodec`（`Set-Cookie` 与 IMF-fixdate 编解码）及 `HttpCookieJar`（域名后缀与公共后缀边界防护、路径前缀匹配、`Secure`/过期过滤）；
- 修复 [`lib/http/decompressor.cp`](file:///data/dev/cp/lib/http/decompressor.cp) 模块化模式缺失 `WINDOW_BITS_AUTO` 显式导入，打通 `cap:http` 全量 `-m` 编译；增量导出至 [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp)，更新 [`docs/http/http.md`](file:///data/dev/cp/docs/http/http.md)；新增 [`cap/test/test_http_web_primitives_deep.cp`](file:///data/dev/cp/cap/test/test_http_web_primitives_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有 HTTP 回归 14/14 零回归）。

### `cap:logger` 结构化日志与 W3C Trace Context / OpenTelemetry 追踪升级 (`lib/logger/`，0 `std.posix`，0 `@import("bun")`)
- 新增 [`lib/logger/structured.cp`](file:///data/dev/cp/lib/logger/structured.cp)（713 行）：`StructuredLogger`/`LogFormatter`（RFC 8259 JSON 转义、Logfmt、PrettyAnsi）、`withField` 子 Logger、可插拔 Sink 及不区分大小写敏感字段自动脱敏（`password`/`token`/`authorization`/`secret` -> `"[REDACTED]"`）；
- 新增 [`lib/logger/trace.cp`](file:///data/dev/cp/lib/logger/trace.cp)（691 行）：W3C Trace Context Level 1 `TraceParent`（严格校验/全零拒绝/子 Span 派生）、`TraceState` 及 `Span`/`SpanEvent`/`SpanStatus`（含 OTel JSON 导出）；
- 新增 [`lib/logger/sampler.cp`](file:///data/dev/cp/lib/logger/sampler.cp)（267 行）：`RatioSampler`（FNV-1a+SplitMix64 确定性采样）、`BurstSampler`（防日志风暴降采样）与零堆 `RingLogBuffer<CAP>`；
- 增量导出至 [`lib/logger/index.cp`](file:///data/dev/cp/lib/logger/index.cp)，更新 [`docs/logger/logger.md`](file:///data/dev/cp/docs/logger/logger.md)；新增 [`cap/test/test_logger_production_deep.cp`](file:///data/dev/cp/cap/test/test_logger_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有 `test_shell_napi_elf_logger_deep.cp` 4/4 零回归）。
