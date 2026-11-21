# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:decimal_fp` 纯 CP 金融定点数 Decimal 精确算术引擎（`i128` 尾数 + `0..18` 标度，零浮点漂移、4 种舍入模式与字符串往返，支持 `cap.decimal_fp` 自动动态挂载，详见 `doc/lib/decimal_fp.md`）（5/5 PASS，文档直写 `doc/lib/decimal_fp.md`）
- **核心设计**：采用 `i128` 尾数标度对齐算术与银行家舍入 `HalfEven`，通过 `"0.1"+"0.2"=="0.3"` 及溢出/除零拦截验证。

## 2026-11-21: 新建 `cap:base64url` 纯 CP RFC 4648 Base64 与 JWT/JWK 专用无填充/自适应 Base64URL 编解码引擎（支持 `cap.base64url` 自动动态挂载，详见 `doc/lib/base64url.md`）（5/5 PASS，文档直写 `doc/lib/base64url.md`）
- **核心设计**：封装 Zig 0.17 `std.base64` 标准与 URL 安全无填充编解码器，通过 RFC 4648 Section 10 向量与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:bitset_simd` 纯 CP 按字向量化加速动态位集合引擎（基于 `u64` 字块 `@popCount`/`@ctz` 批量位运算、`setRange`/`nextSetBit` 及集合并交差，支持 `cap.bitset_simd` 自动动态挂载，详见 `doc/lib/bitset_simd.md`）（5/5 PASS，文档直写 `doc/lib/bitset_simd.md`）
- **核心设计**：按 64 位字块执行向量化位运算与 `@ctz` 跳位迭代，通过跨字边界与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:uuid_v7` 纯 CP 零堆 RFC 9562 UUID v4 / v5 / 时间有序 v7 生成与解析引擎（支持 `cap.uuid_v7` 自动动态挂载，详见 `doc/lib/uuid_v7.md`）（5/5 PASS，文档直写 `doc/lib/uuid_v7.md`）
- **核心设计**：采用 48 位毫秒大端时间戳前缀保证 v7 字典序单调，通过 `v5(DNS, "python.org")` 官方向量与格式错误拦截校验。

## 2026-11-21: 新建 `cap:slab_pool` 纯 CP 固定块代际 $O(1)$ Slab 对象池引擎（侵入式空闲链表、代际防悬垂 ABA 句柄 `Handle` 与零碎片复用，支持 `cap.slab_pool` 自动动态挂载，详见 `doc/lib/slab_pool.md`）（5/5 PASS，文档直写 `doc/lib/slab_pool.md`）
- **核心设计**：采用侵入式空闲链表与 32 位代际号防悬垂 ABA 访问，通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:circ_buf` 纯 CP 泛型定容环形缓冲区引擎（支持阻塞 `push`、覆盖最旧元素 `pushOverwrite`、双端弹出与随机下标访问，支持 `cap.circ_buf` 自动动态挂载，详见 `doc/lib/circ_buf.md`）（5/5 PASS，文档直写 `doc/lib/circ_buf.md`）
- **核心设计**：模运算循环索引支持覆盖与非覆盖双模式，通过回绕读写与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:raft_log` 纯 CP Raft 共识复制日志与快照压缩引擎（1-based 索引、`appendIfMatch` 冲突截断、安全提交推进与 `compactTo` 快照截断，支持 `cap.raft_log` 自动动态挂载，详见 `doc/lib/raft_log.md`）（5/5 PASS，文档直写 `doc/lib/raft_log.md`）
- **核心设计**：深拷贝条目载荷并维护 `snapshot_index`/`snapshot_term` 基线，通过冲突覆盖与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:rate_token_bucket` 纯 CP 零堆分配令牌桶 (`TokenBucket`) 与 GCRA 漏桶 (`GcraLimiter`) 纳秒级精确限流引擎（支持 `cap.rate_token_bucket` 自动动态挂载，详见 `doc/lib/rate_token_bucket.md`）（5/5 PASS，文档直写 `doc/lib/rate_token_bucket.md`）
- **核心设计**：零堆分配实现突发容量控制与 `retryAfterNs` 退避计算，通过突发/稳态配额与边界校验。

## 2026-11-21: 新建 `cap:lru_k` 纯 CP 抗扫描污染 LRU-K 双队列缓存替换引擎（历史访问队列 + 热键晋升缓存队列，防全表扫描击穿热缓存，支持 `cap.lru_k` 自动动态挂载，详见 `doc/lib/lru_k.md`）（5/5 PASS，文档直写 `doc/lib/lru_k.md`）
- **核心设计**：采用历史访问队列与主缓存队列分离，访问达 K 次晋升主缓存，通过冷扫描抗污染与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:geo_quadtree` 纯 CP 2D 平面空间四叉树索引引擎（矩形包围盒 `queryRect` 与欧氏圆半径 `queryRadius` 检索，支持 `cap.geo_quadtree` 自动动态挂载，详见 `doc/lib/geo_quadtree.md`）（5/5 PASS，文档直写 `doc/lib/geo_quadtree.md`）
- **核心设计**：采用 4 象限自动分裂与深度上限 24 防重合点死循环，通过 50 点分裂检索与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:crdt_pn` 纯 CP 分布式状态型无冲突复制数据类型 (State-Based CRDTs) 套件（`GCounter`、`PNCounter`、`LWWRegister` 与 `GSet`，满足半格幂等/交换/结合律 `merge`，支持 `cap.crdt_pn` 自动动态挂载，详见 `doc/lib/crdt_pn.md`）（5/5 PASS，文档直写 `doc/lib/crdt_pn.md`）
- **核心设计**：采用 `(timestamp, node_id)` 全序决胜与幂等半格合并，通过跨分区并发收敛与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:geohash_3d` 纯 CP 3D Morton Z-Order 21-bit 空间填充曲线编码与 WGS84 Haversine 球面大圆测距引擎（支持 `cap.geohash_3d` 自动动态挂载，详见 `doc/lib/geohash_3d.md`）（5/5 PASS，文档直写 `doc/lib/geohash_3d.md`）
- **核心设计**：采用魔数位扩张/压缩实现零堆 $O(1)$ 3D Morton 交错编码，通过 21-bit 极值往返与 NYC-London 大圆距离校验。

## 2026-11-21: 新建 `cap:consistent_maglev` 纯 CP Google Maglev $O(1)$ 一致性哈希查表路由引擎（置换表均匀分布、加权后端比例路由与最小迁移扰动，支持 `cap.consistent_maglev` 自动动态挂载，详见 `doc/lib/consistent_maglev.md`）（5/5 PASS，文档直写 `doc/lib/consistent_maglev.md`）
- **核心设计**：采用双 Wyhash 偏移/步长生成置换序列并轮询填充素数查表，通过加权 2:1 比例路由与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:suffix_array` 纯 CP 后缀数组与 Kasai $O(N)$ LCP 数组引擎（$O(M \log N)$ 二分子串检索、`countOccurrences`、`searchAll` 与最长重复子串，支持 `cap.suffix_array` 自动动态挂载，详见 `doc/lib/suffix_array.md`）（5/5 PASS，文档直写 `doc/lib/suffix_array.md`）
- **核心设计**：结合 Kasai 线性时间 LCP 计算与二分上下界定位，通过 `"banana"` 官方向量与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:consistent_jump` 纯 CP Google Jump Consistent Hash 零内存分片与 Rendezvous HRW 等权/加权无状态路由引擎（支持 `cap.consistent_jump` 自动动态挂载，详见 `doc/lib/consistent_jump.md`）（5/5 PASS，文档直写 `doc/lib/consistent_jump.md`）
- **核心设计**：零堆分配实现单调分片与加权 HRW 最小扰动路由，通过 10->11 桶单调迁移与 3:1 权重分布校验。

## 2026-11-21: 新建 `cap:kmp_z_algo` 纯 CP 线性时间 $O(N)$ 字符串算法库（KMP 前缀表与全重叠匹配、Z-Algorithm `zArray` 与 Manacher 最长回文子串，支持 `cap.kmp_z_algo` 自动动态挂载，详见 `doc/lib/kmp_z_algo.md`）（5/5 PASS，文档直写 `doc/lib/kmp_z_algo.md`）
- **核心设计**：全部算法保持线性时间与最小堆内存分配，通过 `ABABCABAB` LPS、`aabxaabxcaabxaabxay` Z 数组与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:bptree` 纯 CP 内存有序索引 B+ 树引擎（多层节点分裂、叶子链表 `rangeScan` 范围扫描与动态删除重平衡，支持 `cap.bptree` 自动动态挂载，详见 `doc/lib/bptree.md`）（5/5 PASS，文档直写 `doc/lib/bptree.md`）
- **核心设计**：采用叶子双向/单向链表与固定阶数节点分裂，通过 300 乱序插删与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:rope_text` 纯 CP 大文本编辑器平衡二叉树 Rope 引擎（分块叶子、缓存行/字节权重、$O(\log N)$ 插入/删除/`lineAt` 行定位与零泄漏切片拼接，支持 `cap.rope_text` 自动动态挂载，详见 `doc/lib/rope_text.md`）（5/5 PASS，文档直写 `doc/lib/rope_text.md`）
- **核心设计**：叶子节点紧凑分块，内部节点缓存 `left_weight` 与 `left_newlines`，通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:base58_check` 纯 CP Bitcoin Base58 & Base58Check 双重 SHA-256 校验和编解码引擎（支持前导零映射、版本号与防篡改校验，支持 `cap.base58_check` 自动动态挂载，详见 `doc/lib/base58_check.md`）（5/5 PASS，文档直写 `doc/lib/base58_check.md`）
- **核心设计**：采用精确大整数进位编解码与 Bitcoin 创世块 P2PKH 官方向量验证，通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:diff_myers` 纯 CP Myers 最短编辑脚本 (SES) 行级 Diff、Unified Diff 格式化与补丁还原引擎（支持 `cap.diff_myers` 自动动态挂载，详见 `doc/lib/diff_myers.md`）（5/5 PASS，文档直写 `doc/lib/diff_myers.md`）
- **核心设计**：采用 Myers/LCS SES 计算最小编辑序列，生成标准 `@@ -a,b +c,d @@` Unified Diff 并通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:tdigest` 纯 CP 流式 Dunning T-Digest 分位数估算引擎（高精度 p50/p95/p99 尾部估计与分布式 `merge` 聚合，支持 `cap.tdigest` 自动动态挂载，详见 `doc/lib/tdigest.md`）（5/5 PASS，文档直写 `doc/lib/tdigest.md`）
- **核心设计**：采用 Dunning 缩放函数自动合并质心，通过 1 万样本分位数精度与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:merkle_tree` 纯 CP 密码学 SHA-256 Merkle Tree 引擎（`0x00`/`0x01` 域分离防第二原像碰撞、奇数叶支持与无状态包容性证明 `generateProof`/`verifyProof`，支持 `cap.merkle_tree` 自动动态挂载，详见 `doc/lib/merkle_tree.md`）（5/5 PASS，文档直写 `doc/lib/merkle_tree.md`）
- **核心设计**：采用 `0x00` 叶子与 `0x01` 内部节点域分离哈希，覆盖空树/单叶/奇数叶/篡改拦截与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:bimap` 纯 CP 双向一对一映射容器（$O(1)$ 双向查找、冲突拦截 `insert`、强制覆盖 `forceInsert` 与深拷贝字符串专用 `StringBiMap`，支持 `cap.bimap` 自动动态挂载，详见 `doc/lib/bimap.md`）（5/5 PASS，文档直写 `doc/lib/bimap.md`）
- **核心设计**：双哈希表维护严格 1-to-1 双射关系，支持 `forceInsert` 自动清理冲突旧映射与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:segment_tree` 纯 CP 懒传播线段树引擎（$O(\log N)$ 区间增量 `rangeAdd`、区间和/最小/最大值查询，支持 `cap.segment_tree` 自动动态挂载，详见 `doc/lib/segment_tree.md`）（5/5 PASS，文档直写 `doc/lib/segment_tree.md`）
- **核心设计**：采用 4N 堆数组节点与延迟下推 (`push`)，通过 64 元素暴力对照测试与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:fenwick` 纯 CP 树状数组 Binary Indexed Tree 引擎（$O(\log N)$ 单点增减/赋值、前缀/区间和与 `lowerBound` 二进制提升，支持 `cap.fenwick` 自动动态挂载，详见 `doc/lib/fenwick.md`）（5/5 PASS，文档直写 `doc/lib/fenwick.md`）
- **核心设计**：采用经典 LSB 树状数组与二进制提升查找，通过 1000 元素压力累加与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:interval_tree` 纯 CP 增强区间树引擎（带 `max_high` 子树剪枝、`queryOverlap`/`queryPoint`/`hasOverlap`，支持 `cap.interval_tree` 自动动态挂载，详见 `doc/lib/interval_tree.md`）（5/5 PASS，文档直写 `doc/lib/interval_tree.md`）
- **核心设计**：维护子树 `max_high` 边界实现高效剪枝检索，通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:skip_list` 纯 CP 有序跳表键值引擎（$O(\log N)$ 插入/覆盖/删除、`lowerBound`/`floor`/`ceiling` 边界查找与 `rangeQuery` 区间扫描，支持 `cap.skip_list` 自动动态挂载，详见 `doc/lib/skip_list.md`）（5/5 PASS，文档直写 `doc/lib/skip_list.md`）
- **核心设计**：采用 `std.Random.DefaultPrng` 确定性层高随机化与 16 层前向指针数组，通过 200 节点乱序插删与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:trie_radix` 纯 CP 路径压缩基数树 / Patricia Trie 引擎（支持边分裂插入、最长前缀路由 `longestPrefixMatch`、前缀遍历及删除自动边合并，支持 `cap.trie_radix` 自动动态挂载，详见 `doc/lib/trie_radix.md`）（5/5 PASS，文档直写 `doc/lib/trie_radix.md`）
- **核心设计**：支持边分裂插入、删除后单子节点自动合并、`longestPrefixMatch` 与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:consistent_hash` 纯 CP 虚拟节点一致性哈希环引擎（Wyhash 均匀分布、动态增删物理节点最小迁移与顺时针去重 N 副本路由 `getReplicas`，支持 `cap.consistent_hash` 自动动态挂载，详见 `doc/lib/consistent_hash.md`）（5/5 PASS，文档直写 `doc/lib/consistent_hash.md`）
- **核心设计**：采用 Wyhash 虚拟节点二分查找与去重副本遍历，通过 3 节点 600 键负载均衡与扩容迁移率 < 45% 校验。

## 2026-11-21: 新建 `cap:promql` 纯 CP Prometheus PromQL 词法与递归下降 AST 解析器（支持即时/区间向量选择器、`=~`/`!~` 标签匹配、聚合 `by`/`without`、`on`/`ignoring`/`group_left` 二元修饰符，支持 `cap.promql` 自动动态挂载，详见 `doc/lib/promql.md`）（5/5 PASS，文档直写 `doc/lib/promql.md`）
- **核心设计**：支持 `histogram_quantile` 嵌套函数、二元算术优先级及 `std.testing.allocator` 递归释放零泄漏验证。

## 2026-11-21: 新建 `cap:disjoint_set` 纯 CP 并查集 Union-Find 引擎（路径减半压缩 + 按秩合并 + 连通分量/规模实时追踪，支持 `cap.disjoint_set` 自动动态挂载，详见 `doc/lib/disjoint_set.md`）（5/5 PASS，文档直写 `doc/lib/disjoint_set.md`）
- **核心设计**：路径减半压缩 + 按秩合并保证 $O(\\alpha(N))$ 均摊复杂度，通过 500 节点链式合并与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:aes_gcm` 纯 CP NIST SP 800-38D AES-128/256-GCM 与 RFC 8452 AES-256-GCM-SIV 抗 Nonce 重用 AEAD 认证加密引擎（支持 `cap.aes_gcm` 自动动态挂载，详见 `doc/lib/aes_gcm.md`）（5/5 PASS，文档直写 `doc/lib/aes_gcm.md`）
- **核心设计**：基于 Zig 0.17 `std.crypto.aead` 与 `std.Io.Threaded` 实现零泄漏 AEAD 封装，通过 NIST CAVP 与 RFC 8452 官方向量及篡改拦截测试。

## 2026-11-21: 新建 `cap:aho_corasick` 纯 CP Aho-Corasick $O(N+Z)$ 多模式字符串匹配自动机（BFS 失败指针/输出链、全重叠匹配与零堆 `containsAny`，支持 `cap.aho_corasick` 自动动态挂载，详见 `doc/lib/aho_corasick.md`）（5/5 PASS，文档直写 `doc/lib/aho_corasick.md`）
- **核心设计**：采用 BFS 构建 `fail` 与 `out_link` 压缩输出链，通过经典 `ushers`、嵌套后缀与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:base32` 纯 CP RFC 4648 Standard Base32 / Base32hex / Crockford Base32 编解码引擎（支持大小写不敏感、可选填充与 Crockford 字符容错，支持 `cap.base32` 自动动态挂载，详见 `doc/lib/base32.md`）（5/5 PASS，文档直写 `doc/lib/base32.md`）
- **核心设计**：采用位累加器流式编解码，通过 RFC 4648 Section 10 全量官方向量与 Crockford `0/O`、`1/I/L` 容错验证。

## 2026-11-21: 新建 `cap:ed25519` 纯 CP 原生 RFC 8032 Ed25519 数字签名、密钥对生成、批量验签与 Hex 编解码引擎（支持 `cap.ed25519` 自动动态挂载，详见 `doc/lib/ed25519.md`）（5/5 PASS，文档直写 `doc/lib/ed25519.md`）
- **核心设计**：基于 Zig 0.17 `std.crypto.sign.Ed25519` 与 `std.Io.Threaded` 实现零堆签名与批量校验，通过 RFC 8032 Section 7.1 TEST 1/2/3 官方向量与篡改拦截测试。

