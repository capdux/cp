# CP 协同开发执行记录 (SESSION) - 2026-11-20 (热数据)

> 历史执行记录已完整归档至 [SESSION_H.md](file:///data/dev/cp/SESSION_H.md)。本文件仅保留最新活跃记录。

### 2026-11-20（运行过程瓶颈分析与 6 条提速方案 + 冷热日志归档 + `cap:http` & `cap:logger` 双路验收固化）
- **用户指令**：分析当前运行过程，给出提速方法与建议（随后用户已将历史热日志滚动归档至 `CHANGELOG_H.md`/`DEVLOG_H.md`/`SESSION_H.md`）。
- **分析与提速落地**：
  - 量化定位 6 大瓶颈（热文档膨胀、主控 3~4 回合串行验收、`cpc` 新增标准库目录触发 45s 重编、子智能体边写边导出误伤下游、调试期未先 `cpc check`、96 核仅开 2 路并发），给出 6 条按 ROI 排序的提速方案，并在新派发子智能体中立即执行“先快检清零、最后再导出 `index.cp`”原子发布纪律；
- **双路成果验收与固化**：
  - 子智能体 9（`470ffeac`，`cap:http`）与子智能体 10（`dd1bc70f`，`cap:logger`）双路完工回收；
  - `lib/http/` 新增 `sse.cp`、`multipart.cp` 并升级 `cookie.cp`，同时修复 `decompressor.cp` 显式导入 `WINDOW_BITS_AUTO` 打通 `-m` 模块模式，[`test_http_web_primitives_deep.cp`](file:///data/dev/cp/cap/test/test_http_web_primitives_deep.cp) 单文件与 `-m` 均 5/5 PASS，既有回归 14/14 PASS；
  - `lib/logger/` 新增 `structured.cp`、`trace.cp`、`sampler.cp`，[`test_logger_production_deep.cp`](file:///data/dev/cp/cap/test/test_logger_production_deep.cp) 单文件与 `-m` 均 5/5 PASS，既有回归 4/4 PASS；
  - 已同步热日志、`cp_agent_ref.md` 与 `*_H.md` 归档提交 Git，同时无缝拉起子智能体 11（`347ae7c2`，`lib/html/` HTML5 实体/白名单清洗器/AI 网页提取）与子智能体 12（`5dc3d9b8`，`lib/json/` RFC 6901 Pointer/RFC 6902 Patch/RFC 7396 Merge Patch/Pretty&Canonical 序列化/NDJSON 流式读写）双路并行。

### 2026-11-20（固化提速与三轨解耦规范至 `GEMINI.md`、`cp_lib_ref.md` 剥离 + `cap:html` & `cap:json` 验收提交 + 升级 3 路正交并行）
- **用户指令**：认可提速方案，已清空 `SESSION.md`/`DEVLOG.md`/`CHANGELOG.md` 并迁入 `*_H.md`；要求：1. 后续按提速方案执行；2. 将内置标准库剥离为 `cp_lib_ref.md`，吸收/改库不再触碰 `cp_ref.md`/`cp_agent_ref.md`；3. 日常仅读 `cp_agent_ref.md`，其语法修改按标题打 `[SYNC_PENDING]` 标记后续另开任务同步至 `cp_ref.md`；4. 总结写入 `GEMINI.md` 并修正所有冲突条款。
- **执行成果**：
  - 完整落地三轨解耦规范（新建 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)、精简 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)、全量更新 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 消除冲突）、新增 [`scripts/verify_module.sh`](file:///data/dev/cp/scripts/verify_module.sh)、升级 [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh) 原子替换与 [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) 自动探测 `lib/<ns>/index.cp`；
  - 回收并验收子智能体 11（`lib/html/`，5/5 PASS）与子智能体 12（`lib/json/`，5/5 PASS），一并提交 Git；
  - 正式启用 96 核 **3 路正交并行**，同步派发子智能体 13（`lib/security/` TOTP/CORS/CSP）、子智能体 14（`lib/dns/` DoH/全记录/resolv.conf）与子智能体 15（`lib/bench/` 统计基准/Welch t-test/分配追踪）。

### 2026-11-20（3 路正交首批 `cap:bench` / `cap:security` / `cap:dns` 合并验收提交，开启第二批 `cap:csv` / `cap:uuid` / `cap:template` 3 路并行）
- **执行成果**：首批 3 路子智能体（`lib/bench/` 9/9、`lib/security/` 9/9、`lib/dns/` 10/10）全部通过 `verify_module.sh` 一键验收回收，合并单次更新 `cp_lib_ref.md` 与轻量热日志并提交 Git；立即启动第二批 3 路正交全新标准库（`lib/csv/`、`lib/uuid/`、`lib/template/`，全面验证动态 `cap.<ns>` 零改动编译器自动挂载）。

### 2026-11-20（回答子智能体模型分层原因 + `cap:csv` 验收提交，顶替拉起 `cap:math` 保持 3 路并行）
- **用户询问**：询问为何主模型为 Gemini 4 Neon · Medium 而子智能体显示为 Gemini 4 Hydrogen，并询问 Hydrogen 档位。已如实说明此前传入 `Model: "flash"` 以实现快慢分层与配额隔离，并提供保持分层或统一 `inherit` 两项选择。
- **执行成果**：子智能体 16（`lib/csv/`，5/5 PASS）与子智能体 17（`lib/uuid/`，5/5 PASS）双双经 `verify_module.sh` 一键验收回收（含动态 `cap.csv` / `cap.uuid` 自动挂载实证），合并单次提交 Git；顶替派发子智能体 19（`lib/math/` 线性代数/统计/AI 向量相似度与量化）与子智能体 20（`lib/schema/` JSON Schema 2020-12 运行时校验器），与 `lib/template/` 维持 3 路正交并行。

### 2026-11-20（`cap:template` 验收固化，顶替拉起 `cap:rpc` JSON-RPC 2.0 / MCP 协议引擎维持 3 路并行）
- **执行成果**：子智能体 18（`lib/template/`，5/5 PASS，含 `cap.template` 动态挂载）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 21（新建 `lib/rpc/` JSON-RPC 2.0 + LSP/MCP Content-Length 分帧 + MCP Server 分发器），与 `lib/math/`、`lib/schema/` 保持 3 路正交并行。

### 2026-11-20（`cap:math` 验收固化，顶替拉起 `cap:cache` LRU/TTL/BloomFilter/CountMinSketch 维持 3 路并行）
- **用户询问**：追问 `Gemini 4 Hydrogen` 档位及与 `Neon · Medium` 编程能力/配额综合选型；已基于今晚 15 个标准库全绿实测给出分级建议（主控与疑难 `cpc` 热修用 `Neon · Medium`，常规 3 路标准库并发用 `Hydrogen`）。
- **用户指令**：认同并要求将“标准库用 Hydrogen (`flash`)、疑难热修用 `inherit` 启动子智能体”的分级选型方案固化写入 `GEMINI.md`。
- **执行成果**：已将分级模型选型写入 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md)；同时 3 路子智能体 19（`lib/math/` 5/5）、20（`lib/schema/` 5/5）、21（`lib/rpc/` 5/5）全部经 `verify_module.sh` 一键验收回收（均实证动态 `cap.<ns>` 自动挂载），合并单次提交 Git；当前无缝维持 `lib/cache/`（`12d3e976`）、`lib/fsm/`（`872f1782`）与 `lib/vector/`（纯 CP 向量检索 + BM25 + RRF 混合搜索）3 路正交并行。

### 2026-11-20（`cap:fsm` & `cap:cache` 双路合并验收提交，顶替拉起 `cap:metrics` 与 `cap:text` 维持 3 路并行）
- **执行成果**：子智能体 23（`lib/fsm/` 5/5）与子智能体 22（`lib/cache/` 5/5）双双经 `verify_module.sh` 一键验收回收（含动态 `cap.fsm`/`cap.cache` 挂载验证），合并单次提交 Git；立即顶替派发子智能体 25（`lib/metrics/` Prometheus/OpenMetrics）与子智能体 26（`lib/text/` AI RAG 文本分块、模糊搜索与 Token 预算估算），与 `lib/vector/` 保持 3 路正交并行。

### 2026-11-20（排查收口旧 Swarm 面板提示 + `cap:vector` 验收提交，顶替拉起 `cap:xml` 维持 3 路并行）
- **用户指令**：要求处理底部旧 Swarm 提示（`task-capsurf-20261119-compiler-regression`），不再发送或提示。已确认 `queue.db` 双端 `unacked` 均为 `[]`、旧 Swarm 成员均已 `killed`，在面板追加无 `@` 的 `[归档闭环]` 收口记录（`seq: 9`）顶掉旧预览。
- **执行成果**：子智能体 24（`lib/vector/`，5/5 PASS，含 `cap.vector` 动态挂载）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发子智能体 27（新建 `lib/xml/` XML 1.0 解析/DOM/安全 Writer/路径查询），与 `lib/metrics/`、`lib/text/` 保持 3 路正交并行。

### 2026-11-20（`cap:text` 验收固化，顶替拉起 `cap:cbor` RFC 8949 CBOR & MessagePack 引擎维持 3 路并行）
- **执行成果**：子智能体 26（`lib/text/`，5/5 PASS，含 `cap.text` 动态挂载）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 28（新建 `lib/cbor/` RFC 8949 CBOR + MessagePack 二进制编解码），与 `lib/metrics/`、`lib/xml/` 保持 3 路正交并行。

### 2026-11-20（`cap:xml` & `cap:metrics` 双路合并验收提交，顶替拉起 `cap:protobuf` 与 `cap:expr` 维持 3 路并行）
- **执行成果**：子智能体 27（`lib/xml/` 5/5）与子智能体 25（`lib/metrics/` 5/5）双双经 `verify_module.sh` 一键验收回收（含动态 `cap.xml`/`cap.metrics` 挂载验证），合并单次提交 Git；立即顶替派发子智能体 29（`lib/protobuf/` Proto3 Wire & gRPC 分帧）与子智能体 30（`lib/expr/` 沙盒安全表达式与策略规则求值引擎），与 `lib/cbor/` 保持 3 路正交并行。

### 2026-11-20（`cap:cbor` 验收固化，顶替拉起 `cap:kv` 崩溃安全 WAL 嵌入式存储引擎维持 3 路并行）
- **执行成果**：子智能体 28（`lib/cbor/`，5/5 PASS，含 `cap.cbor` 动态挂载）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 31（新建 `lib/kv/` CRC32 WAL + Bitcask 持久化 KV 存储与原子压缩），与 `lib/protobuf/`、`lib/expr/` 保持 3 路正交并行。

### 2026-11-20（`cap:expr` & `cap:protobuf` 双路合并验收提交，顶替拉起 `cap:queue` 与 `cap:graph` 维持 3 路并行）
- **执行成果**：子智能体 30（`lib/expr/` 5/5）与子智能体 29（`lib/protobuf/` 5/5）双双经 `verify_module.sh` 一键验收回收（含动态 `cap.expr`/`cap.protobuf` 挂载验证），合并单次提交 Git；立即顶替派发子智能体 32（`lib/queue/` 优先级/延迟/死信任务队列）与子智能体 33（`lib/graph/` 图论与 GraphRAG 知识图谱算法：Dijkstra/Tarjan SCC/拓扑排序/Kruskal MST/PageRank），与 `lib/kv/` 保持 3 路正交并行。

### 2026-11-21（`cap:kv` 验收固化，顶替拉起 `cap:cluster` 分布式集群协调原语维持 3 路并行）
- **执行成果**：子智能体 31（`lib/kv/`，5/5 PASS，含 `cap.kv` 动态挂载）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 34（新建 `lib/cluster/` 虚拟节点一致性哈希环、SWIM 成员状态机与单调 Fencing Token 选主租约），与 `lib/queue/`、`lib/graph/` 保持 3 路正交并行。

### 2026-11-21（`cap:queue` 验收固化，顶替拉起 `cap:geo` 地理空间与 Geohash 检索引擎维持 3 路并行）
- **执行成果**：子智能体 32（`lib/queue/`，5/5 PASS，含 `cap.queue` 动态挂载）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 35（新建 `lib/geo/` Haversine/方位角/点在多边形内/Geohash 8 邻域与半径空间索引），与 `lib/graph/`、`lib/cluster/` 保持 3 路正交并行。

### 2026-11-21（文档目录非复数化与扁平化重构：`docs/` -> `doc/lib/*.md`）
- **用户指令**：将 `docs/` 改为单数 `doc/`，所有库文件说明统一扁平放入 `doc/lib/<mod>.md`，去掉文件同名目录。
- **执行成果**：已通过 `git mv` 将全部 85 份已提交标准库文档迁移至 `doc/lib/*.md`，清理 `docs/pack/zlig.md` 及 `docs/stdlib/` 16 份历史重复稿，全量更新 `cp_lib_ref.md`、`cp_agent_ref.md` 与 `GEMINI.md` 路径引用（`docs/` 0 残留），并通知后台运行中的子智能体使用新路径，单次提交固化。

### 2026-11-21（3 路 `cap:cluster` / `cap:geo` / `cap:graph` 合并验收提交，旧 `docs/` 彻底清空，启动 `cap:crdt` / `cap:color` / `cap:decimal` 3 路并行）
- **执行成果**：子智能体 34（`lib/cluster/` 5/5）、35（`lib/geo/` 5/5）、33（`lib/graph/` 5/5）全部经 `verify_module.sh` 一键验收回收，文档全部归口至 `doc/lib/` 且旧 `docs/` 目录已彻底删除，合并单次提交 Git；当前无缝维持 `lib/crdt/`、`lib/color/`、`lib/decimal/`（高精度 Decimal128 金融定点算术与货币分配）3 路正交并行。

### 2026-11-21（`cap:crdt` 验收固化，顶替拉起 `cap:query` 参数化防注入 SQL 查询构建器维持 3 路并行）
- **用户指令**：确认后续 `lib/` 文档统一按 `doc/lib/<mod>.md` 写入，且 `docs/` 迁移完毕已删除。
- **执行成果**：子智能体 36（`lib/crdt/`，5/5 PASS，含 `cap.crdt` 动态挂载与 `doc/lib/crdt.md`）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 39（新建 `lib/query/` 多方言防注入 SQL 查询构建器），与 `lib/color/`、`lib/decimal/` 保持 3 路正交并行。

### 2026-11-21（`cap:color` 验收固化，顶替拉起 `cap:email` RFC 5322/MIME/SMTP 引擎维持 3 路并行）
- **执行成果**：子智能体 37（`lib/color/`，5/5 PASS，含 `cap.color` 动态挂载与 `doc/lib/color.md`）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 40（新建 `lib/email/` RFC 5322 地址校验/MIME 邮件构建解析/SMTP 线缆编解码），与 `lib/decimal/`、`lib/query/` 保持 3 路正交并行。

### 2026-11-21（`cap:decimal` 验收固化，顶替拉起 `cap:qrcode` QR 码编码与渲染引擎维持 3 路并行）
- **执行成果**：子智能体 38（`lib/decimal/`，5/5 PASS，含 `cap.decimal` 动态挂载与 `doc/lib/decimal.md`）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 41（新建 `lib/qrcode/` ISO/IEC 18004 QR 码编码器、Reed-Solomon GF(256) 与终端半块/SVG 渲染），与 `lib/query/`、`lib/email/` 保持 3 路正交并行。

### 2026-11-21（`cap:qrcode` 验收固化，顶替拉起 `cap:intl` 人性化数值/字节/时长格式化引擎维持 3 路并行）
- **执行成果**：子智能体 41（`lib/qrcode/`，5/5 PASS，含 `cap.qrcode` 动态挂载与 `doc/lib/qrcode.md`）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 42（新建 `lib/intl/` 千分位/紧凑数值/SI&IEC 字节大小与时长双向解析格式化/相对时间），与 `lib/query/`、`lib/email/` 保持 3 路正交并行。

### 2026-11-21（`cap:query` & `cap:email` 双路合并验收提交，顶替拉起 `cap:oauth` 与 `cap:webhook` 维持 3 路并行）
- **执行成果**：子智能体 39（`lib/query/` 5/5）与子智能体 40（`lib/email/` 5/5）双双经 `verify_module.sh` 一键验收回收，文档直写 `doc/lib/{query,email}.md` 并合并提交 Git；立即顶替派发子智能体 43（`lib/oauth/` OAuth 2.0/PKCE S256）与子智能体 44（`lib/webhook/` Standard Webhooks/GitHub/Stripe 签名验签 + 防重放 + 幂等去重表），与 `lib/intl/` 保持 3 路正交并行。

### 2026-11-21（`cap:intl` & `cap:webhook` 双路合并验收提交，顶替拉起 `cap:git` 与 `cap:pdf` 维持 3 路并行）
- **执行成果**：子智能体 42（`lib/intl/` 5/5）与子智能体 44（`lib/webhook/` 5/5）双双经 `verify_module.sh` 一键验收回收，文档直写 `doc/lib/{intl,webhook}.md` 并合并提交 Git；立即顶替派发子智能体 45（`lib/git/` Git 对象/`.gitignore`/仓库内省）与子智能体 46（`lib/pdf/` 零依赖 PDF 1.4 生成与 XRef/元数据内省），与 `lib/oauth/` 保持 3 路正交并行。

### 2026-11-21（`cap:oauth` 验收固化，顶替拉起 `cap:audio` 语音 AI Agent WAV/DSP/VAD 引擎维持 3 路并行）
- **执行成果**：子智能体 43（`lib/oauth/`，5/5 PASS，含 `cap.oauth` 动态挂载与 `doc/lib/oauth.md`）经 `verify_module.sh` 一键验收回收并提交 Git；立即顶替派发子智能体 47（新建 `lib/audio/` WAV PCM16 编解码、重采样、RMS/dB 与 Energy VAD），与 `lib/git/`、`lib/pdf/` 保持 3 路正交并行。

### 2026-11-21（`cap:git` & `cap:pdf` 双路合并验收提交 + 并发下调为 2 路常规节约配额）
- **用户指令**：后续开启 2 个子智能体节约配额，热修单独开个子智能体、完成后回收。
- **执行成果**：
  - 已更新 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 固化“常规 2 路 `flash` 正交并行 + 热修单独开 `inherit` 子智能体、修完即收”；
  - 回收并验收子智能体 45（`lib/git/` 5/5）与子智能体 46（`lib/pdf/` 5/5），文档直写 `doc/lib/{git,pdf}.md`，合并单次提交 Git；
  - 当前已在跑的 3 个子智能体（`lib/audio/`、`lib/tui/`、`lib/wasm/`）待首个完工回收后不再补位，自然平滑回落至长期 2 路并行。

### 2026-11-21（`cap:audio` 验收固化，不补位平滑回落至 2 路常规并行）
- **执行成果**：子智能体 47（`lib/audio/`，5/5 PASS，含 `cap.audio` 动态挂载与 `doc/lib/audio.md`）经 `verify_module.sh` 一键验收回收并提交 Git；按节约配额规范不再补位，当前稳定保持 **2 路**常规 `flash` 子智能体并行（`lib/tui/` 与 `lib/wasm/`）。

### 2026-11-21（`cap:tui` 验收固化，顶替拉起 `cap:asn1` ASN.1 DER & X.509 证书引擎维持 2 路并行）
- **执行成果**：子智能体 48（`lib/tui/`，5/5 PASS，含 `cap.tui` 动态挂载与 `doc/lib/tui.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发子智能体 50（新建 `lib/asn1/` ITU-T X.690 DER 编解码与 X.509 v3 证书内省），与 `lib/wasm/` 严格保持 2 路常规并行。

### 2026-11-21（`cap:wasm` 验收固化，顶替拉起 `cap:pubsub` NATS/MQTT 通配符消息代理维持 2 路并行）
- **执行成果**：子智能体 49（`lib/wasm/`，5/5 PASS，含 `cap.wasm` 动态挂载与 `doc/lib/wasm.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发子智能体 51（新建 `lib/pubsub/` NATS `*`/`>` 与 MQTT `+`/`#` 通配符路由、有界背压队列与保留消息代理），与 `lib/asn1/` 严格保持 2 路常规并行。

### 2026-11-21（`cap:pubsub` 验收固化，顶替拉起 `cap:graphql` 查询解析与格式化引擎维持 2 路并行）
- **执行成果**：子智能体 51（`lib/pubsub/`，5/5 PASS，含 `cap.pubsub` 动态挂载与 `doc/lib/pubsub.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发子智能体 52（新建 `lib/graphql/` GraphQL 词法/AST/查询深度防护与格式化器），与 `lib/asn1/` 严格保持 2 路常规并行。

### 2026-11-21（`cap:asn1` 验收固化，顶替拉起 `cap:rbac` 企业级权限与 AI 工具鉴权引擎维持 2 路并行）
- **执行成果**：子智能体 50（`lib/asn1/`，5/5 PASS，含 `cap.asn1` 动态挂载与 `doc/lib/asn1.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发子智能体 53（新建 `lib/rbac/` 分层角色继承、通配符权限、显式拒绝优先与审计决策追踪），与 `lib/graphql/` 严格保持 2 路常规并行。

### 2026-11-21（`cap:rbac` 验收固化 + 响应用户配额不足暂停流水线）
- **用户指令**：配额不够，暂停，等配额恢复继续。
- **执行成果**：
  - 完成已完工子智能体 53（`lib/rbac/`，5/5 PASS，含 `cap.rbac` 动态挂载与 `doc/lib/rbac.md`）的收尾归档与 Git 提交；
  - 已立即终止后台运行中的子智能体 52（`lib/graphql/`）并停止派发任何新子智能体，当前 0 后台任务、0 配额消耗，待配额恢复后按指令继续。

### 2026-11-21（切号恢复 2 路并行流水线 + `cap:graphql` 验收固化，顶替拉起 `cap:supervisor`）
- **用户指令**：已重启 AGY 并切换配额账号恢复会话；要求重新开启 2 个并发子智能体分发未完成任务，读取 `/root/.agy/comm/COMM.md` 保持多 AGY 通信畅通，本阶段完成更新文档并自动 Git 提交后自主推导下一阶段持续工作。
- **执行成果**：
  - 巡检 `agy-comm.service`（`active`）与 `task_queue.py unacked`（`[]`），重启 2 路常规 `flash` 并行（接续 `lib/graphql/` + 新建 `lib/openapi/`）；
  - `lib/graphql/`（5/5 PASS，含 `cap.graphql` 动态挂载与 `doc/lib/graphql.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发 `lib/supervisor/`（BEAM OTP 监督树策略/抖动退避/舱壁限流/重试预算），与 `lib/openapi/` 保持 2 路正交并行。

### 2026-11-21（`cap:openapi` 验收固化，顶替拉起 `cap:codec` 多进制编解码引擎维持 2 路并行）
- **执行成果**：`lib/openapi/`（5/5 PASS，含 `cap.openapi` 动态挂载与 `doc/lib/openapi.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发 `lib/codec/`（Base58/Base58Check、ASCII85/Base85、Bech32/Bech32m 与 Crockford Base32 编解码），与 `lib/supervisor/` 严格保持 2 路正交并行。

### 2026-11-21（`cap:supervisor` 验收固化，顶替拉起 `cap:prompt` AI Agent 上下文窗口与 LLM 输出提取引擎维持 2 路并行）
- **执行成果**：`lib/supervisor/`（5/5 PASS，含 `cap.supervisor` 动态挂载与 `doc/lib/supervisor.md`）经 `verify_module.sh` 一键验收回收并提交 Git；顶替派发 `lib/prompt/`（多角色对话窗口 Token 预算裁剪、OpenAI/Anthropic 导出与 LLM Fenced/JSON/XML 输出提取），与 `lib/codec/` 严格保持 2 路正交并行。

### 2026-11-21（固化后续子智能体统一使用 `Model: "inherit"` 跟随主智能体模型）
- **用户指令**：后续子智能体使用 `inherit` 启动，跟随主智能体的模型。
- **执行成果**：已确认并严格执行后续所有新派发子智能体统一传入 `Model: "inherit"`（跟随主智能体模型），当前在跑的 `lib/codec/` 与 `lib/prompt/` 完工回收后即全面切换为 `inherit` 派生。

### 2026-11-21（`cap:prompt` 验收固化，顶替以 `inherit` 拉起 `cap:toml` 维持 2 路并行）
- **执行成果**：`lib/prompt/`（5/5 PASS，含 `cap.prompt` 动态挂载与 `doc/lib/prompt.md`）经 `verify_module.sh` 一键验收回收并提交 Git；严格按新规使用 `Model: "inherit"` 顶替派发 `lib/toml/`（TOML v1.0.0 完整解析/校验/序列化与 JSON 转换），与 `lib/codec/` 保持 2 路正交并行。

### 2026-11-21（`cap:codec` 验收固化，顶替以 `inherit` 拉起 `cap:yaml` 维持 2 路并行）
- **执行成果**：`lib/codec/`（5/5 PASS，含 `cap.codec` 动态挂载与 `doc/lib/codec.md`）经 `verify_module.sh` 一键验收回收并提交 Git；使用 `Model: "inherit"` 顶替派发 `lib/yaml/`（YAML 1.2 核心子集解析与序列化/JSON 转换），目前两路常规子智能体均已全面运行在 `inherit` 模式下。
