# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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

