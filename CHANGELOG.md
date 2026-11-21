# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:geo_quadtree` 纯 CP 2D 平面空间四叉树索引引擎（矩形包围盒 `queryRect` 与欧氏圆半径 `queryRadius` 检索，支持 `cap.geo_quadtree` 自动动态挂载，详见 `doc/lib/geo_quadtree.md`）（5/5 PASS，自动动态挂载 `cap.geo_quadtree`）
- **新增标准库**：`lib/geo_quadtree/{quadtree,index}.cp`，实现泛型二维 `QuadTree(V)` 与深度上限保护。\n- **新增文档与测试**：`doc/lib/geo_quadtree.md` 扁平文档与 `cap/test/test_geo_quadtree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:crdt_pn` 纯 CP 分布式状态型无冲突复制数据类型 (State-Based CRDTs) 套件（`GCounter`、`PNCounter`、`LWWRegister` 与 `GSet`，满足半格幂等/交换/结合律 `merge`，支持 `cap.crdt_pn` 自动动态挂载，详见 `doc/lib/crdt_pn.md`）（5/5 PASS，自动动态挂载 `cap.crdt_pn`）
- **新增标准库**：`lib/crdt_pn/{crdt,index}.cp`，实现 `GCounter`、`PNCounter`、`LWWRegister(T)` 与 `GSet(T)`。\n- **新增文档与测试**：`doc/lib/crdt_pn.md` 扁平文档与 `cap/test/test_crdt_pn_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:geohash_3d` 纯 CP 3D Morton Z-Order 21-bit 空间填充曲线编码与 WGS84 Haversine 球面大圆测距引擎（支持 `cap.geohash_3d` 自动动态挂载，详见 `doc/lib/geohash_3d.md`）（5/5 PASS，自动动态挂载 `cap.geohash_3d`）
- **新增标准库**：`lib/geohash_3d/{geohash_3d,index}.cp`，实现 63-bit 3D Morton 编码解码、三维量化与 Haversine 球面测距。\n- **新增文档与测试**：`doc/lib/geohash_3d.md` 扁平文档与 `cap/test/test_geohash_3d_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:consistent_maglev` 纯 CP Google Maglev $O(1)$ 一致性哈希查表路由引擎（置换表均匀分布、加权后端比例路由与最小迁移扰动，支持 `cap.consistent_maglev` 自动动态挂载，详见 `doc/lib/consistent_maglev.md`）（5/5 PASS，自动动态挂载 `cap.consistent_maglev`）
- **新增标准库**：`lib/consistent_maglev/{maglev,index}.cp`，实现 Maglev 置换表与 $O(1)$ 查表路由。\n- **新增文档与测试**：`doc/lib/consistent_maglev.md` 扁平文档与 `cap/test/test_consistent_maglev_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:suffix_array` 纯 CP 后缀数组与 Kasai $O(N)$ LCP 数组引擎（$O(M \log N)$ 二分子串检索、`countOccurrences`、`searchAll` 与最长重复子串，支持 `cap.suffix_array` 自动动态挂载，详见 `doc/lib/suffix_array.md`）（5/5 PASS，自动动态挂载 `cap.suffix_array`）
- **新增标准库**：`lib/suffix_array/{suffix_array,index}.cp`，实现后缀数组构建、Kasai LCP 数组与二分子串检索。\n- **新增文档与测试**：`doc/lib/suffix_array.md` 扁平文档与 `cap/test/test_suffix_array_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:consistent_jump` 纯 CP Google Jump Consistent Hash 零内存分片与 Rendezvous HRW 等权/加权无状态路由引擎（支持 `cap.consistent_jump` 自动动态挂载，详见 `doc/lib/consistent_jump.md`）（5/5 PASS，自动动态挂载 `cap.consistent_jump`）
- **新增标准库**：`lib/consistent_jump/{consistent_jump,index}.cp`，实现 Jump Consistent Hash 与 Rendezvous HRW 等权/加权路由。\n- **新增文档与测试**：`doc/lib/consistent_jump.md` 扁平文档与 `cap/test/test_consistent_jump_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:kmp_z_algo` 纯 CP 线性时间 $O(N)$ 字符串算法库（KMP 前缀表与全重叠匹配、Z-Algorithm `zArray` 与 Manacher 最长回文子串，支持 `cap.kmp_z_algo` 自动动态挂载，详见 `doc/lib/kmp_z_algo.md`）（5/5 PASS，自动动态挂载 `cap.kmp_z_algo`）
- **新增标准库**：`lib/kmp_z_algo/{kmp_z_algo,index}.cp`，实现 KMP、Z-Algorithm 与 Manacher 最长回文。\n- **新增文档与测试**：`doc/lib/kmp_z_algo.md` 扁平文档与 `cap/test/test_kmp_z_algo_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bptree` 纯 CP 内存有序索引 B+ 树引擎（多层节点分裂、叶子链表 `rangeScan` 范围扫描与动态删除重平衡，支持 `cap.bptree` 自动动态挂载，详见 `doc/lib/bptree.md`）（5/5 PASS，自动动态挂载 `cap.bptree`）
- **新增标准库**：`lib/bptree/{bptree,index}.cp`，实现泛型有序 `BPlusTree(K, V)` 插入分裂、`rangeScan` 与删除。\n- **新增文档与测试**：`doc/lib/bptree.md` 扁平文档与 `cap/test/test_bptree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:rope_text` 纯 CP 大文本编辑器平衡二叉树 Rope 引擎（分块叶子、缓存行/字节权重、$O(\log N)$ 插入/删除/`lineAt` 行定位与零泄漏切片拼接，支持 `cap.rope_text` 自动动态挂载，详见 `doc/lib/rope_text.md`）（5/5 PASS，自动动态挂载 `cap.rope_text`）
- **新增标准库**：`lib/rope_text/{rope,index}.cp`，实现平衡二叉树 `Rope` 文本缓冲区与行索引。\n- **新增文档与测试**：`doc/lib/rope_text.md` 扁平文档与 `cap/test/test_rope_text_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:base58_check` 纯 CP Bitcoin Base58 & Base58Check 双重 SHA-256 校验和编解码引擎（支持前导零映射、版本号与防篡改校验，支持 `cap.base58_check` 自动动态挂载，详见 `doc/lib/base58_check.md`）（5/5 PASS，自动动态挂载 `cap.base58_check`）
- **新增标准库**：`lib/base58_check/{base58_check,index}.cp`，实现 Base58 与 Double-SHA256 Base58Check 编解码。\n- **新增文档与测试**：`doc/lib/base58_check.md` 扁平文档与 `cap/test/test_base58_check_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:diff_myers` 纯 CP Myers 最短编辑脚本 (SES) 行级 Diff、Unified Diff 格式化与补丁还原引擎（支持 `cap.diff_myers` 自动动态挂载，详见 `doc/lib/diff_myers.md`）（5/5 PASS，自动动态挂载 `cap.diff_myers`）
- **新增标准库**：`lib/diff_myers/{diff,index}.cp`，实现 Myers SES 行级 Diff、`reconstructNew` 与 `formatUnifiedDiff`。\n- **新增文档与测试**：`doc/lib/diff_myers.md` 扁平文档与 `cap/test/test_diff_myers_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:tdigest` 纯 CP 流式 Dunning T-Digest 分位数估算引擎（高精度 p50/p95/p99 尾部估计与分布式 `merge` 聚合，支持 `cap.tdigest` 自动动态挂载，详见 `doc/lib/tdigest.md`）（5/5 PASS，自动动态挂载 `cap.tdigest`）
- **新增标准库**：`lib/tdigest/{tdigest,index}.cp`，实现流式 `TDigest` 质心压缩、`quantile` 插值与 `merge`。\n- **新增文档与测试**：`doc/lib/tdigest.md` 扁平文档与 `cap/test/test_tdigest_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:merkle_tree` 纯 CP 密码学 SHA-256 Merkle Tree 引擎（`0x00`/`0x01` 域分离防第二原像碰撞、奇数叶支持与无状态包容性证明 `generateProof`/`verifyProof`，支持 `cap.merkle_tree` 自动动态挂载，详见 `doc/lib/merkle_tree.md`）（5/5 PASS，自动动态挂载 `cap.merkle_tree`）
- **新增标准库**：`lib/merkle_tree/{merkle,index}.cp`，实现域分离 SHA-256 Merkle Tree 与包容性证明生成/验签。\n- **新增文档与测试**：`doc/lib/merkle_tree.md` 扁平文档与 `cap/test/test_merkle_tree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bimap` 纯 CP 双向一对一映射容器（$O(1)$ 双向查找、冲突拦截 `insert`、强制覆盖 `forceInsert` 与深拷贝字符串专用 `StringBiMap`，支持 `cap.bimap` 自动动态挂载，详见 `doc/lib/bimap.md`）（5/5 PASS，自动动态挂载 `cap.bimap`）
- **新增标准库**：`lib/bimap/{bimap,index}.cp`，实现泛型 `BiMap(L, R)` 与自动内存托管 `StringBiMap`。\n- **新增文档与测试**：`doc/lib/bimap.md` 扁平文档与 `cap/test/test_bimap_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:segment_tree` 纯 CP 懒传播线段树引擎（$O(\log N)$ 区间增量 `rangeAdd`、区间和/最小/最大值查询，支持 `cap.segment_tree` 自动动态挂载，详见 `doc/lib/segment_tree.md`）（5/5 PASS，自动动态挂载 `cap.segment_tree`）
- **新增标准库**：`lib/segment_tree/{segment_tree,index}.cp`，实现泛型懒传播 `SegmentTree(T)` 区间更新与聚合查询。\n- **新增文档与测试**：`doc/lib/segment_tree.md` 扁平文档与 `cap/test/test_segment_tree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:fenwick` 纯 CP 树状数组 Binary Indexed Tree 引擎（$O(\log N)$ 单点增减/赋值、前缀/区间和与 `lowerBound` 二进制提升，支持 `cap.fenwick` 自动动态挂载，详见 `doc/lib/fenwick.md`）（5/5 PASS，自动动态挂载 `cap.fenwick`）
- **新增标准库**：`lib/fenwick/{fenwick,index}.cp`，实现泛型 `FenwickTree(T)` 区间和与 `lowerBound`。\n- **新增文档与测试**：`doc/lib/fenwick.md` 扁平文档与 `cap/test/test_fenwick_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:interval_tree` 纯 CP 增强区间树引擎（带 `max_high` 子树剪枝、`queryOverlap`/`queryPoint`/`hasOverlap`，支持 `cap.interval_tree` 自动动态挂载，详见 `doc/lib/interval_tree.md`）（5/5 PASS，自动动态挂载 `cap.interval_tree`）
- **新增标准库**：`lib/interval_tree/{interval_tree,index}.cp`，实现泛型 `IntervalTree(T, V)` 区间插入与重叠查询。\n- **新增文档与测试**：`doc/lib/interval_tree.md` 扁平文档与 `cap/test/test_interval_tree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:skip_list` 纯 CP 有序跳表键值引擎（$O(\log N)$ 插入/覆盖/删除、`lowerBound`/`floor`/`ceiling` 边界查找与 `rangeQuery` 区间扫描，支持 `cap.skip_list` 自动动态挂载，详见 `doc/lib/skip_list.md`）（5/5 PASS，自动动态挂载 `cap.skip_list`）
- **新增标准库**：`lib/skip_list/{skip_list,index}.cp`，实现泛型有序跳表 `SkipList(K, V)` 与边界/区间检索。\n- **新增文档与测试**：`doc/lib/skip_list.md` 扁平文档与 `cap/test/test_skip_list_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:trie_radix` 纯 CP 路径压缩基数树 / Patricia Trie 引擎（支持边分裂插入、最长前缀路由 `longestPrefixMatch`、前缀遍历及删除自动边合并，支持 `cap.trie_radix` 自动动态挂载，详见 `doc/lib/trie_radix.md`）（5/5 PASS，自动动态挂载 `cap.trie_radix`）
- **新增标准库**：`lib/trie_radix/{radix,index}.cp`，实现路径压缩基数树 `RadixTree(V)`。\n- **新增文档与测试**：`doc/lib/trie_radix.md` 扁平文档与 `cap/test/test_trie_radix_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:consistent_hash` 纯 CP 虚拟节点一致性哈希环引擎（Wyhash 均匀分布、动态增删物理节点最小迁移与顺时针去重 N 副本路由 `getReplicas`，支持 `cap.consistent_hash` 自动动态挂载，详见 `doc/lib/consistent_hash.md`）（5/5 PASS，自动动态挂载 `cap.consistent_hash`）
- **新增标准库**：`lib/consistent_hash/{consistent_hash,index}.cp`，实现 `HashRing` 虚拟节点环、增删节点与 `getReplicas`。\n- **新增文档与测试**：`doc/lib/consistent_hash.md` 扁平文档与 `cap/test/test_consistent_hash_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:promql` 纯 CP Prometheus PromQL 词法与递归下降 AST 解析器（支持即时/区间向量选择器、`=~`/`!~` 标签匹配、聚合 `by`/`without`、`on`/`ignoring`/`group_left` 二元修饰符，支持 `cap.promql` 自动动态挂载，详见 `doc/lib/promql.md`）（5/5 PASS，自动动态挂载 `cap.promql`）
- **新增标准库**：`lib/promql/{promql,index}.cp`，实现 PromQL 词法器、Duration 解析与完整递归下降 AST 解析器。\n- **新增文档与测试**：`doc/lib/promql.md` 扁平文档与 `cap/test/test_promql_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:disjoint_set` 纯 CP 并查集 Union-Find 引擎（路径减半压缩 + 按秩合并 + 连通分量/规模实时追踪，支持 `cap.disjoint_set` 自动动态挂载，详见 `doc/lib/disjoint_set.md`）（5/5 PASS，自动动态挂载 `cap.disjoint_set`）
- **新增标准库**：`lib/disjoint_set/{disjoint_set,index}.cp`，实现带路径压缩与按秩合并的 `DisjointSet`。\n- **新增文档与测试**：`doc/lib/disjoint_set.md` 扁平文档与 `cap/test/test_disjoint_set_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:aes_gcm` 纯 CP NIST SP 800-38D AES-128/256-GCM 与 RFC 8452 AES-256-GCM-SIV 抗 Nonce 重用 AEAD 认证加密引擎（支持 `cap.aes_gcm` 自动动态挂载，详见 `doc/lib/aes_gcm.md`）（5/5 PASS，自动动态挂载 `cap.aes_gcm`）
- **新增标准库**：`lib/aes_gcm/{aes_gcm,index}.cp`，实现 AES-128-GCM、AES-256-GCM、AES-256-GCM-SIV 与随机 Nonce `seal256Alloc`/`open256Alloc`。\n- **新增文档与测试**：`doc/lib/aes_gcm.md` 扁平文档与 `cap/test/test_aes_gcm_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:aho_corasick` 纯 CP Aho-Corasick $O(N+Z)$ 多模式字符串匹配自动机（BFS 失败指针/输出链、全重叠匹配与零堆 `containsAny`，支持 `cap.aho_corasick` 自动动态挂载，详见 `doc/lib/aho_corasick.md`）（5/5 PASS，自动动态挂载 `cap.aho_corasick`）
- **新增标准库**：`lib/aho_corasick/{aho_corasick,index}.cp`，实现 Aho-Corasick 自动机、`findAllOverlapping` 与 `containsAny`。\n- **新增文档与测试**：`doc/lib/aho_corasick.md` 扁平文档与 `cap/test/test_aho_corasick_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:base32` 纯 CP RFC 4648 Standard Base32 / Base32hex / Crockford Base32 编解码引擎（支持大小写不敏感、可选填充与 Crockford 字符容错，支持 `cap.base32` 自动动态挂载，详见 `doc/lib/base32.md`）（5/5 PASS，自动动态挂载 `cap.base32`）
- **新增标准库**：`lib/base32/{base32,index}.cp`，实现 RFC 4648 Standard/Hex/Crockford Base32 编解码与异常填充校验。\n- **新增文档与测试**：`doc/lib/base32.md` 扁平文档与 `cap/test/test_base32_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ed25519` 纯 CP 原生 RFC 8032 Ed25519 数字签名、密钥对生成、批量验签与 Hex 编解码引擎（支持 `cap.ed25519` 自动动态挂载，详见 `doc/lib/ed25519.md`）（5/5 PASS，自动动态挂载 `cap.ed25519`）
- **新增标准库**：`lib/ed25519/{ed25519,index}.cp`，实现 RFC 8032 Ed25519 密钥对、`sign`/`signRandomized`、`verify`、`verifyBatch` 与 Hex 编解码。\n- **新增文档与测试**：`doc/lib/ed25519.md` 扁平文档与 `cap/test/test_ed25519_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

