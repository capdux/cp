# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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

