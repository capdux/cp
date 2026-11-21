# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

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

