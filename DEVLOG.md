# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

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

