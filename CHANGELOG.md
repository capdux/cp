# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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

