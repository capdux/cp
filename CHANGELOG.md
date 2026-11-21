# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:scrypt` 纯 CP 工业级 RFC 7914 `scrypt` 内存硬化口令派生与 PHC 字符串编解码校验标准库（支持 `cap.scrypt` 自动动态挂载，详见 `doc/lib/scrypt.md`）（5/5 PASS，自动动态挂载 `cap.scrypt`）
- **新增标准库**：`lib/scrypt/{scrypt,index}.cp`，实现 RFC 7914 `scrypt` KDF、PHC `$scrypt$ln=...,r=...,p=...$salt$hash` 格式编解码、随机盐派生、常量时间校验与防 DoS 内存上限拦截。\n- **新增文档与测试**：`doc/lib/scrypt.md` 扁平文档与 `cap/test/test_scrypt_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ksuid` 纯 CP Segment KSUID 20 字节 / 27 字符 Base62 字典序唯一 ID 与零堆编解码引擎（支持 `cap.ksuid` 自动动态挂载，详见 `doc/lib/ksuid.md`）（5/5 PASS，自动动态挂载 `cap.ksuid`）
- **新增标准库**：`lib/ksuid/{ksuid,index}.cp`，实现 Segment KSUID 官方向量全匹配的 20B/27-char Base62 零堆编解码与字典序比较。\n- **新增文档与测试**：`doc/lib/ksuid.md` 扁平文档与 `cap/test/test_ksuid_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:lockfree_queue` 纯 CP 工业级无锁并发环形队列与有界多线程通道标准库（缓存行隔离 SPSC、Vyukov MPMC、可关闭 `BoundedChannel`，支持 `cap.lockfree_queue` 自动动态挂载，详见 `doc/lib/lockfree_queue.md`）（5/5 PASS，自动动态挂载 `cap.lockfree_queue`）
- **新增标准库**：`lib/lockfree_queue/{spsc,mpmc,channel,index}.cp`，实现缓存行对齐 `SpscQueue`、Vyukov `MpmcQueue` 及动态有界阻塞 `BoundedChannel`。\n- **新增文档与测试**：`doc/lib/lockfree_queue.md` 扁平文档与 `cap/test/test_lockfree_queue_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:argon2` 纯 CP 工业级 RFC 9106 Argon2 (`Argon2id` / `Argon2i` / `Argon2d`) 内存硬化口令派生与 PHC 字符串编解码校验标准库（支持 `cap.argon2` 自动动态挂载，详见 `doc/lib/argon2.md`）（5/5 PASS，自动动态挂载 `cap.argon2`）
- **新增标准库**：`lib/argon2/{types,engine,index}.cp`，实现 RFC 9106 `Argon2id`/`Argon2i`/`Argon2d`、PHC 格式解析/生成、随机盐派生、常量时间摘要校验与内存上限防护。\n- **新增文档与测试**：`doc/lib/argon2.md` 扁平文档与 `cap/test/test_argon2_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:snowflake` 纯 CP 线程安全分布式 64-bit Snowflake ID 生成器、时钟回拨保护、字段反解与零堆 Base62 编解码引擎（支持 `cap.snowflake` 自动动态挂载，详见 `doc/lib/snowflake.md`）（5/5 PASS，自动动态挂载 `cap.snowflake`）
- **新增标准库**：`lib/snowflake/{snowflake,index}.cp`，实现 Mutex 保护 `SnowflakeGenerator`、4096 毫秒内序列翻转、时钟回拨拦截、`decomposeId` 与零堆 `encodeBase62`/`decodeBase62`。
- **新增文档与测试**：`doc/lib/snowflake.md` 扁平文档与 `cap/test/test_snowflake_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:blake3` 纯 CP 零堆 BLAKE3 Merkle 树 256-bit 密码学哈希、Keyed MAC、KDF 与 XOF 可扩展输出引擎（支持 `cap.blake3` 自动动态挂载，详见 `doc/lib/blake3.md`）（5/5 PASS，自动动态挂载 `cap.blake3`）
- **新增标准库**：`lib/blake3/{compress,hasher,index}.cp`，实现固定 `[54][8]u32` CV 栈零堆流式 `Blake3Hasher`、Keyed MAC、上下文 `deriveKey` 及任意偏移 `finalSeek` XOF 输出，全量对齐 BLAKE3 官方向量。
- **新增文档与测试**：`doc/lib/blake3.md` 扁平文档与 `cap/test/test_blake3_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:crc` 纯 CP 零堆全谱系 CRC-8/16/32/32C/64 查表校验和与 O(log N) `crc32Combine` 合并引擎（支持 `cap.crc` 自动动态挂载，详见 `doc/lib/crc.md`）（5/5 PASS，自动动态挂载 `cap.crc`）
- **新增标准库**：`lib/crc/{crc,index}.cp`，实现 CRC-8/SMBUS、CRC-16/CCITT-FALSE/XMODEM/MODBUS、CRC-32 IEEE/Castagnoli 与 `crc32Combine`、CRC-64 XZ/ECMA-182 及零堆流式接口。
- **新增文档与测试**：`doc/lib/crc.md` 扁平文档与 `cap/test/test_crc_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bech32` 纯 CP BIP-173 Bech32 & BIP-350 Bech32m 纠错码、5/8-bit 转换与 SegWit 地址编码引擎（支持 `cap.bech32` 自动动态挂载，详见 `doc/lib/bech32.md`）（5/5 PASS，自动动态挂载 `cap.bech32`）
- **新增标准库**：`lib/bech32/{bech32,index}.cp`，实现 BIP-173/350 官方向量全匹配的 `encode`/`decode`、`convertBits`、`encodeBytes`/`decodeBytes` 与 `encodeSegwitAddr` (v0/v1 Taproot)，严格拦截混合大小写与校验和错误。
- **新增文档与测试**：`doc/lib/bech32.md` 扁平文档与 `cap/test/test_bech32_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:shamir` 纯 CP GF(2^8) 信息论安全 Shamir (k, n) 门限密钥分片与拉格朗日插值恢复引擎（支持 `cap.shamir` 自动动态挂载，详见 `doc/lib/shamir.md`）（5/5 PASS，自动动态挂载 `cap.shamir`）
- **新增标准库**：`lib/shamir/{shamir,index}.cp`，实现 CSPRNG 随机多项式 `splitSecret`、GF(2^8) 拉格朗日插值 `combineShares` 与 `CPSS1` `Share.encode`/`decode` 格式化，严格拦截不足门限、重复索引与非法格式。
- **新增文档与测试**：`doc/lib/shamir.md` 扁平文档与 `cap/test/test_shamir_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:rope` 纯 CP Piece Table 大文本编辑、碎片合并、行号/列号互转与 Undo/Redo 事务栈引擎（支持 `cap.rope` 自动动态挂载，详见 `doc/lib/rope.md`）（5/5 PASS，自动动态挂载 `cap.rope`）
- **新增标准库**：`lib/rope/{piece_table,index}.cp`，实现连续尾部追加碎片合并 `RopeText`、`offsetToLineCol`/`lineColToOffset`/`getLine`（兼容 `\r\n`）及分叉自动清空 Redo 栈的 `undo`/`redo`。
- **新增文档与测试**：`doc/lib/rope.md` 扁平文档与 `cap/test/test_rope_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:reed_solomon` 纯 CP GF(2^8) 系统性 Cauchy Reed-Solomon 纠删码与高斯-约旦无损恢复引擎（支持 `cap.reed_solomon` 自动动态挂载，详见 `doc/lib/reed_solomon.md`）（5/5 PASS，自动动态挂载 `cap.reed_solomon`）
- **新增标准库**：`lib/reed_solomon/{rs,index}.cp`，实现 GF(2^8) 有限域运算、系统性 Cauchy 编码矩阵（支持 `K + M <= 256`）及任意 `<= M` 丢失数据/校验分片的高斯-约旦无损恢复。
- **新增文档与测试**：`doc/lib/reed_solomon.md` 扁平文档与 `cap/test/test_reed_solomon_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:http_router` 纯 CP Radix Trie 高性能 HTTP 路由器、零堆参数提取、405 Allow 头生成与 RouteGroup 引擎（支持 `cap.http_router` 自动动态挂载，详见 `doc/lib/http_router.md`）（5/5 PASS，自动动态挂载 `cap.http_router`）
- **新增标准库**：`lib/http_router/{types,trie,index}.cp`，实现 Static > Param > CatchAll Radix Trie 匹配、零堆 `MAX_PARAMS = 16` 提取、405 自动 `Allow` 头、路由冲突检测与嵌套 `RouteGroup`。
- **新增文档与测试**：`doc/lib/http_router.md` 扁平文档与 `cap/test/test_http_router_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:xxhash` 纯 CP 零堆 xxHash (XXH32/XXH64/XXH3_64/XXH3_128) 与流式分块增量哈希引擎（支持 `cap.xxhash` 自动动态挂载，详见 `doc/lib/xxhash.md`）（5/5 PASS，自动动态挂载 `cap.xxhash`）
- **新增标准库**：`lib/xxhash/{xxh32,xxh64,xxh3,index}.cp`，实现 XXH32/XXH64 流式与一次性哈希及 XXH3_64/XXH3_128 全 7 个长度分段，通过官方向量与 `1..7` 字节流式一致性验证。
- **新增文档与测试**：`doc/lib/xxhash.md` 扁平文档与 `cap/test/test_xxhash_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:murmur3` 纯 CP 零堆 MurmurHash3 x86_32 & x64_128 非加密哈希与流式增量计算引擎（支持 `cap.murmur3` 自动动态挂载，详见 `doc/lib/murmur3.md`）（5/5 PASS，自动动态挂载 `cap.murmur3`）
- **新增标准库**：`lib/murmur3/{murmur3,index}.cp`，实现 SMHasher 官方向量全匹配的 MurmurHash3_x86_32 与 MurmurHash3_x64_128，以及零堆流式 `Murmur3_32Stream`/`Murmur3_128Stream`。
- **新增文档与测试**：`doc/lib/murmur3.md` 扁平文档与 `cap/test/test_murmur3_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:siphash` 纯 CP 零堆 SipHash-2-4 (64/128-bit) 与 SipHash-1-3 抗 Hash-Flooding PRF 与流式认证码引擎（支持 `cap.siphash` 自动动态挂载，详见 `doc/lib/siphash.md`）（5/5 PASS，自动动态挂载 `cap.siphash`）
- **新增标准库**：`lib/siphash/{siphash,index}.cp`，实现官方测试向量全匹配的 SipHash-2-4 (64/128-bit)、SipHash-1-3、零堆流式 `SipHashStream` 及常量时间 `verifySipHash24`。
- **新增文档与测试**：`doc/lib/siphash.md` 扁平文档与 `cap/test/test_siphash_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:nfa_regex` 纯 CP Thompson NFA 线性时间 O(N*M) 无回溯正则表达式引擎（免疫 ReDoS，支持锚点/字符类/分组/多分支，详见 `doc/lib/nfa_regex.md`）（5/5 PASS，自动动态挂载 `cap.nfa_regex`）
- **新增标准库**：`lib/nfa_regex/{nfa,index}.cp`，实现免疫 ReDoS 的 Thompson NFA 编译与线性模拟引擎，支持 `^`/`$`、`[a-z0-9]`/`[^...]`、`\d`/`\w`/`\s`、分组与多分支以及 `findFirst`/`findAll`。
- **新增文档与测试**：`doc/lib/nfa_regex.md` 扁平文档与 `cap/test/test_nfa_regex_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:redis_cluster` 纯 CP Redis/Valkey Cluster CRC16-CCITT 16384 槽位路由、Hash Tag 提取与 MOVED/ASK 拓扑热更新引擎（支持 `cap.redis_cluster` 自动动态挂载，详见 `doc/lib/redis_cluster.md`）（5/5 PASS，自动动态挂载 `cap.redis_cluster`）
- **新增标准库**：`lib/redis_cluster/{crc16,topology,index}.cp`，实现 `crc16Ccitt` (XMODEM `0x1021`)、`{hash_tag}` 提取、`ensureSameSlot` 跨槽拦截、O(1) `[16384]i32` `ClusterTopology` 与 `MOVED`/`ASK` (含 IPv6) 重定向解析。
- **新增文档与测试**：`doc/lib/redis_cluster.md` 扁平文档与 `cap/test/test_redis_cluster_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:capnp` 纯 CP CapnProto 零拷贝分段消息帧、Struct/List/Far/Double-Far 指针与 TraversalLimit 防放大攻击引擎（支持 `cap.capnp` 自动动态挂载，详见 `doc/lib/capnp.md`）（5/5 PASS，自动动态挂载 `cap.capnp`）
- **新增标准库**：`lib/capnp/{types,pointer,reader,builder,index}.cp`，实现 CapnProto 分段流帧（含奇偶段对齐）、Struct/List/Far/Double-Far 指针编解码、默认值 XOR 编码及 `TraversalLimit` (64 MiB/64 层) 防放大攻击。
- **新增文档与测试**：`doc/lib/capnp.md` 扁平文档与 `cap/test/test_capnp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:msgpack_rpc` 纯 CP MessagePack-RPC (Request/Response/Notification) 编解码、深度防护与半包黏包流式拆帧引擎（支持 `cap.msgpack_rpc` 自动动态挂载，详见 `doc/lib/msgpack_rpc.md`）（5/5 PASS，自动动态挂载 `cap.msgpack_rpc`）
- **新增标准库**：`lib/msgpack_rpc/{codec,index}.cp`，实现完整 MessagePack 类型编解码、Request/Response/Notification 帧、`MAX_RPC_DEPTH = 32` 与 `RpcStreamDecoder` 半包黏包拆帧。
- **新增文档与测试**：`doc/lib/msgpack_rpc.md` 扁平文档与 `cap/test/test_msgpack_rpc_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:jmespath` 纯 CP JMESPath JSON 查询、切片/通配符/Flatten 投影、MultiSelect、管道与 13 种内置函数引擎（支持 `cap.jmespath` 自动动态挂载，详见 `doc/lib/jmespath.md`）（5/5 PASS，自动动态挂载 `cap.jmespath`）
- **新增标准库**：`lib/jmespath/{value,parser,eval,index}.cp`，实现 RFC 风格 `compileJmesPath`/`searchJmesPath`（覆盖 `[]`/`[*]`/`*`/过滤器 `[?expr]`/`{k:v}`/`[a,b]`/`|`/`@`/反引号字面量与 13 个内置函数）及 `MAX_DEPTH = 64`/零步长防 DoS 拦截。
- **新增文档与测试**：`doc/lib/jmespath.md` 扁平文档与 `cap/test/test_jmespath_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:dns_wire` 纯 CP RFC 1035/6891 二进制 DNS 报文编解码、0xC0xx 后缀压缩与防循环指针炸弹引擎（支持 `cap.dns_wire` 自动动态挂载，详见 `doc/lib/dns_wire.md`）（5/5 PASS，自动动态挂载 `cap.dns_wire`）
- **新增标准库**：`lib/dns_wire/{types,codec,index}.cp`，实现 12 字节 `DnsHeader`、全记录类型（A/AAAA/CNAME/NS/PTR/MX/TXT/SRV/SOA/OPT）编解码、`0xC0xx` 后缀压缩与 32 跳 `CompressionLoop` 防循环指针拦截。
- **新增文档与测试**：`doc/lib/dns_wire.md` 扁平文档与 `cap/test/test_dns_wire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

