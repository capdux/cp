# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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

