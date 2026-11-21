# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:cuid2` 纯 CP 抗碰撞 CUID2 Base36 唯一 ID 生成与格式校验引擎（SHA3-512 混合时间戳/原子计数/节点指纹，支持 `cap.cuid2` 自动动态挂载，详见 `doc/lib/cuid2.md`）（5/5 PASS，文档直写 `doc/lib/cuid2.md`）
- **核心设计**：采用 `std.crypto.hash.sha3.Sha3_512` 与 `std.Io.Threaded` 随机源，确保小写字母首字符与高并发无碰撞唯一性。

## 2026-11-21: 新建 `cap:totp_hotp` 纯 CP 工业级 RFC 4226 HOTP / RFC 6238 TOTP 动态口令、RFC 4648 Base32 编解码与 `otpauth://` URI 解析构建标准库（支持 `cap.totp_hotp` 自动动态挂载，详见 `doc/lib/totp_hotp.md`）（5/5 PASS，文档直写 `doc/lib/totp_hotp.md`）
- **核心设计**：使用 `std.crypto.auth.hmac` 与 `std.crypto.timing_safe.eql` 实现零漏洞动态截断与常量时间校验，通过 RFC 4226 Appendix D 与 RFC 6238 Appendix B 官方向量全匹配。

## 2026-11-21: 新建 `cap:hkdf_kdf` 纯 CP RFC 5869 HKDF-Extract/Expand 密钥派生引擎（覆盖 SHA-256/384/512 与 TLS 1.3 `expandLabelSha256`，支持 `cap.hkdf_kdf` 自动动态挂载，详见 `doc/lib/hkdf_kdf.md`）（5/5 PASS，文档直写 `doc/lib/hkdf_kdf.md`）
- **核心设计**：基于 Zig 0.17 `std.crypto.kdf.hkdf` 提供零堆固定缓冲区与按需分配两种派生模式，通过 RFC 5869 Appendix A.1/A.3 官方向量与 `255*HashLen` 上限测试。

## 2026-11-21: 新建 `cap:scrypt` 纯 CP 工业级 RFC 7914 `scrypt` 内存硬化口令派生与 PHC 字符串编解码校验标准库（支持 `cap.scrypt` 自动动态挂载，详见 `doc/lib/scrypt.md`）（5/5 PASS，文档直写 `doc/lib/scrypt.md`）
- **核心设计**：基于 Zig 0.17 `std.crypto.pwhash.scrypt.kdf` 与 `std.crypto.timing_safe.eql` 实现，通过 RFC 7914 Section 12 官方向量与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:ksuid` 纯 CP Segment KSUID 20 字节 / 27 字符 Base62 字典序唯一 ID 与零堆编解码引擎（支持 `cap.ksuid` 自动动态挂载，详见 `doc/lib/ksuid.md`）（5/5 PASS，文档直写 `doc/lib/ksuid.md`）
- **核心设计**：采用 `5 x u32` 大端字数组除法与乘法完成零堆 160-bit Base62 编码解码，通过官方已知向量与 27 个 `z` 溢出测试验证。

## 2026-11-21: 新建 `cap:lockfree_queue` 纯 CP 工业级无锁并发环形队列与有界多线程通道标准库（缓存行隔离 SPSC、Vyukov MPMC、可关闭 `BoundedChannel`，支持 `cap.lockfree_queue` 自动动态挂载，详见 `doc/lib/lockfree_queue.md`）（5/5 PASS，文档直写 `doc/lib/lockfree_queue.md`）
- **核心设计**：采用 `align(64)` 缓存行隔离消除伪共享，Vyukov 槽位序列号免疫 ABA 问题，`BoundedChannel` 基于 `std.atomic.Mutex` 支持优雅关闭与剩余元素排空。

## 2026-11-21: 新建 `cap:argon2` 纯 CP 工业级 RFC 9106 Argon2 (`Argon2id` / `Argon2i` / `Argon2d`) 内存硬化口令派生与 PHC 字符串编解码校验标准库（支持 `cap.argon2` 自动动态挂载，详见 `doc/lib/argon2.md`）（5/5 PASS，文档直写 `doc/lib/argon2.md`）
- **核心设计**：基于 Zig 0.17 `std.crypto.argon2.kdf` 与 `std.Io.Threaded` 实现零泄漏口令哈希与 PHC 校验，通过 RFC 9106 官方向量与 `std.testing.allocator` 内存校验。

## 2026-11-21: 新建 `cap:snowflake` 纯 CP 线程安全分布式 64-bit Snowflake ID 生成器、时钟回拨保护、字段反解与零堆 Base62 编解码引擎（支持 `cap.snowflake` 自动动态挂载，详见 `doc/lib/snowflake.md`）（5/5 PASS，文档直写 `doc/lib/snowflake.md`）
- **核心设计**：41-bit 时间戳 + 5-bit DC + 5-bit Worker + 12-bit 序列位布局，通过 4 线程并发 2000 ID 排序唯一性压测，全路径零泄漏。

## 2026-11-21: 新建 `cap:blake3` 纯 CP 零堆 BLAKE3 Merkle 树 256-bit 密码学哈希、Keyed MAC、KDF 与 XOF 可扩展输出引擎（支持 `cap.blake3` 自动动态挂载，详见 `doc/lib/blake3.md`）（5/5 PASS，文档直写 `doc/lib/blake3.md`）
- **核心设计**：采用固定 54 层 CV 栈实现高达 `2^54` 块的零堆流式 Merkle 树归约，常量时间 `verifyKeyedMac` 消除侧信道，全路径零堆分配。

## 2026-11-21: 新建 `cap:crc` 纯 CP 零堆全谱系 CRC-8/16/32/32C/64 查表校验和与 O(log N) `crc32Combine` 合并引擎（支持 `cap.crc` 自动动态挂载，详见 `doc/lib/crc.md`）（5/5 PASS，文档直写 `doc/lib/crc.md`）
- **核心设计**：全路径零堆分配与编译期表生成，精准匹配标准 `"123456789"` 全部测试向量，并基于 GF(2) 矩阵平方实现 O(log N) `crc32Combine`。

## 2026-11-21: 新建 `cap:bech32` 纯 CP BIP-173 Bech32 & BIP-350 Bech32m 纠错码、5/8-bit 转换与 SegWit 地址编码引擎（支持 `cap.bech32` 自动动态挂载，详见 `doc/lib/bech32.md`）（5/5 PASS，文档直写 `doc/lib/bech32.md`）
- **核心设计**：全路径严格校验 HRP ASCII 范围、混合大小写与 BCH 校验和，支持任意字节载荷与 SegWit 规范，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:shamir` 纯 CP GF(2^8) 信息论安全 Shamir (k, n) 门限密钥分片与拉格朗日插值恢复引擎（支持 `cap.shamir` 自动动态挂载，详见 `doc/lib/shamir.md`）（5/5 PASS，文档直写 `doc/lib/shamir.md`）
- **核心设计**：基于 `std.crypto.random` 生成随机多项式系数并逐字节在 GF(2^8) 上求值与插值，任意 `< k` 份均拦截并返回 `InsufficientShares`，全路径零泄漏。

## 2026-11-21: 新建 `cap:rope` 纯 CP Piece Table 大文本编辑、碎片合并、行号/列号互转与 Undo/Redo 事务栈引擎（支持 `cap.rope` 自动动态挂载，详见 `doc/lib/rope.md`）（5/5 PASS，文档直写 `doc/lib/rope.md`）
- **核心设计**：250 步随机编辑与朴素 `ArrayList(u8)` 参考模型 100% 等价，连续尾部插入自动合并为单个 Piece 抑制碎片膨胀，全路径零泄漏。

## 2026-11-21: 新建 `cap:reed_solomon` 纯 CP GF(2^8) 系统性 Cauchy Reed-Solomon 纠删码与高斯-约旦无损恢复引擎（支持 `cap.reed_solomon` 自动动态挂载，详见 `doc/lib/reed_solomon.md`）（5/5 PASS，文档直写 `doc/lib/reed_solomon.md`）
- **核心设计**：采用系统性 Cauchy 矩阵保证任意 `K` 行子矩阵满秩可逆，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:http_router` 纯 CP Radix Trie 高性能 HTTP 路由器、零堆参数提取、405 Allow 头生成与 RouteGroup 引擎（支持 `cap.http_router` 自动动态挂载，详见 `doc/lib/http_router.md`）（5/5 PASS，文档直写 `doc/lib/http_router.md`）
- **核心设计**：匹配路径采用栈上段切片零堆提取参数，插入期自动阻断同级不同参数名冲突与非末尾通配符，全路径零泄漏。

## 2026-11-21: 新建 `cap:xxhash` 纯 CP 零堆 xxHash (XXH32/XXH64/XXH3_64/XXH3_128) 与流式分块增量哈希引擎（支持 `cap.xxhash` 自动动态挂载，详见 `doc/lib/xxhash.md`）（5/5 PASS，文档直写 `doc/lib/xxhash.md`）
- **核心设计**：全路径零堆分配实现，覆盖 `0..>240B` 全分支 XXH3 累加器与流式状态机，比特级对齐官方标准。

## 2026-11-21: 新建 `cap:murmur3` 纯 CP 零堆 MurmurHash3 x86_32 & x64_128 非加密哈希与流式增量计算引擎（支持 `cap.murmur3` 自动动态挂载，详见 `doc/lib/murmur3.md`）（5/5 PASS，文档直写 `doc/lib/murmur3.md`）
- **核心设计**：全路径零堆分配实现，流式与一次性接口 100% 比特一致并通过 SMHasher 官方测试向量精确比对。

## 2026-11-21: 新建 `cap:siphash` 纯 CP 零堆 SipHash-2-4 (64/128-bit) 与 SipHash-1-3 抗 Hash-Flooding PRF 与流式认证码引擎（支持 `cap.siphash` 自动动态挂载，详见 `doc/lib/siphash.md`）（5/5 PASS，文档直写 `doc/lib/siphash.md`）
- **核心设计**：全路径零堆分配实现，匹配官方 Appendix A 全系列 64-bit/128-bit 标准向量，以 `timing_safe.eql` 实现抗侧信道标签验签。

## 2026-11-21: 新建 `cap:nfa_regex` 纯 CP Thompson NFA 线性时间 O(N*M) 无回溯正则表达式引擎（免疫 ReDoS，支持锚点/字符类/分组/多分支，详见 `doc/lib/nfa_regex.md`）（5/5 PASS，文档直写 `doc/lib/nfa_regex.md`）
- **核心设计**：基于状态代际时间戳 `gen_buf` 的零回溯 NFA 模拟，彻底消除指数级回溯风险并限制 `MAX_NFA_STATES = 4096`，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:redis_cluster` 纯 CP Redis/Valkey Cluster CRC16-CCITT 16384 槽位路由、Hash Tag 提取与 MOVED/ASK 拓扑热更新引擎（支持 `cap.redis_cluster` 自动动态挂载，详见 `doc/lib/redis_cluster.md`）（5/5 PASS，文档直写 `doc/lib/redis_cluster.md`）
- **核心设计**：`ClusterTopology` 通过 `[16384]i32` 槽位索引表实现 O(1) 路由与 `MOVED` 原地热更新，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:capnp` 纯 CP CapnProto 零拷贝分段消息帧、Struct/List/Far/Double-Far 指针与 TraversalLimit 防放大攻击引擎（支持 `cap.capnp` 自动动态挂载，详见 `doc/lib/capnp.md`）（5/5 PASS，文档直写 `doc/lib/capnp.md`）
- **核心设计**：`MessageReader` 采用零拷贝分段视图与 `TraversalLimit` 读预算，彻底拦截循环指针与指针重叠放大攻击，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:msgpack_rpc` 纯 CP MessagePack-RPC (Request/Response/Notification) 编解码、深度防护与半包黏包流式拆帧引擎（支持 `cap.msgpack_rpc` 自动动态挂载，详见 `doc/lib/msgpack_rpc.md`）（5/5 PASS，文档直写 `doc/lib/msgpack_rpc.md`）
- **核心设计**：自包含轻量化 MessagePack-RPC 编解码器与 `RpcStreamDecoder`，严格校验数组长度、消息类型与最大帧字节上限，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:jmespath` 纯 CP JMESPath JSON 查询、切片/通配符/Flatten 投影、MultiSelect、管道与 13 种内置函数引擎（支持 `cap.jmespath` 自动动态挂载，详见 `doc/lib/jmespath.md`）（5/5 PASS，文档直写 `doc/lib/jmespath.md`）
- **核心设计**：内置 `MAX_DEPTH = 64`、零步长 `InvalidSliceStep` 与函数参数类型强校验，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:dns_wire` 纯 CP RFC 1035/6891 二进制 DNS 报文编解码、0xC0xx 后缀压缩与防循环指针炸弹引擎（支持 `cap.dns_wire` 自动动态挂载，详见 `doc/lib/dns_wire.md`）（5/5 PASS，文档直写 `doc/lib/dns_wire.md`）
- **核心设计**：解码端对 `0xC0xx` 压缩指针严格限制最大跳转深度（32 跳）并校验非 `0x00`/`0xC0` 非法标签；编码端支持可选后缀字典压缩，全路径 `std.testing.allocator` 零泄漏。

