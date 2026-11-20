# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:bpe` 纯 CP Byte-Pair Encoding (Tiktoken 风格) LLM Tokenizer 编解码、语料训练与 UTF-8 安全截断引擎（支持 `cap.bpe` 自动动态挂载，详见 `doc/lib/bpe.md`）（5/5 PASS，自动动态挂载 `cap.bpe`）
- **新增标准库**：`lib/bpe/{types,vocab,tokenizer,index}.cp`，实现 Byte-Level 0..255 兜底词表、`special_tokens` 切分与 `encodeOrdinary` 防注入、`loadTiktokenText` 解析、`trainFromCorpus` 语料训练及 `truncateToMaxTokens` UTF-8 边界安全截断。
- **新增文档与测试**：`doc/lib/bpe.md` 扁平文档与 `cap/test/test_bpe_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:quant` 纯 CP AI Embedding 向量量化（Scalar Int8 对称/仿射、Binary 1-bit @popCount 汉明距离、Product Quantization K-Means & ADC 查表）引擎（支持 `cap.quant` 自动动态挂载，详见 `doc/lib/quant.md`）（5/5 PASS，自动动态挂载 `cap.quant`）
- **新增标准库**：`lib/quant/{types,scalar,binary,pq,index}.cp`，实现 Int8 对称/仿射量化与 `dotInt8`、32 倍压缩 Binary 1-bit `@popCount` 汉明检索、以及 `trainPqKMeans`/`encodePq`/`buildAdcDistanceTable`/`adcDistance` PQ 子空间量化。
- **新增文档与测试**：`doc/lib/quant.md` 扁平文档与 `cap/test/test_quant_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bm25` 纯 CP Okapi BM25 倒排索引全文检索、邻接短语匹配与 RRF 混合检索融合引擎（支持 `cap.bm25` 自动动态挂载，详见 `doc/lib/bm25.md`）（5/5 PASS，自动动态挂载 `cap.bm25`）
- **新增标准库**：`lib/bm25/{tokenizer,bm25,rrf,index}.cp`，实现停用词与 UTF-8/CJK 保留分词器、支持 `removeDocument` 与 `positions` 邻接短语检索的 `Bm25Index` 及多路无量纲 `rrfFuse`。
- **新增文档与测试**：`doc/lib/bm25.md` 扁平文档与 `cap/test/test_bm25_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:hnsw` 纯 CP HNSW 多层近似最近邻向量图索引（L2/Cosine/InnerProduct、Beam Search、软删除与二进制快照，支持 `cap.hnsw` 自动动态挂载，详见 `doc/lib/hnsw.md`）（5/5 PASS，自动动态挂载 `cap.hnsw`）
- **新增标准库**：`lib/hnsw/{metric,graph,index}.cp`，实现 SplitMix64 `mL` 层高采样、贪心下降 + `ef_construction` Beam Search、`M`/`M0` 双向裁剪、`markDeleted` 软删除过滤及 `saveBytes`/`loadBytes` 二进制快照。
- **新增文档与测试**：`doc/lib/hnsw.md` 扁平文档与 `cap/test/test_hnsw_production_deep.cp`（5/5 PASS，16D 120 向量 Recall@5 >= 0.9，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:mvcc` 纯 CP MVCC 多版本事务快照隔离（First-Committer-Wins 写写冲突检测、Read-Your-Own-Writes 与水位线 GC）引擎（支持 `cap.mvcc` 自动动态挂载，详见 `doc/lib/mvcc.md`）（5/5 PASS，自动动态挂载 `cap.mvcc`）
- **新增标准库**：`lib/mvcc/{types,store,index}.cp`，实现单调时间戳 Oracle、降序版本链、Read-Your-Own-Writes + Snapshot Isolation、First-Committer-Wins `WriteConflict` 拦截及 `minActiveReadTs`/`gcVersions` 安全版本清理。
- **新增文档与测试**：`doc/lib/mvcc.md` 扁平文档与 `cap/test/test_mvcc_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bplus_tree` 纯 CP 可变阶 B+ 树索引（支持分裂/借位/合并/叶子双向链表扫描）与 4KB CRC32C Slotted Page 二进制页引擎（支持 `cap.bplus_tree` 自动动态挂载，详见 `doc/lib/bplus_tree.md`）（5/5 PASS，自动动态挂载 `cap.bplus_tree`）
- **新增标准库**：`lib/bplus_tree/{types,tree,slotted_page,index}.cp`，实现可变阶 `BPlusTree`（`insert` 分裂、`remove` 左右借位/合并/根收缩、`rangeScan`/`rangeIter` 与 `validateInvariants()` 全量不变量自检）及 4096B `SlottedPage`（含 `compact` 与 `Crc32Iscsi` 校验）。
- **新增文档与测试**：`doc/lib/bplus_tree.md` 扁平文档与 `cap/test/test_bplus_tree_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:lsm_sst` 纯 CP LSM-Tree 前缀压缩 SSTable 数据块、Kirsch-Mitzenmacher 布隆过滤器、CRC32C 校验与多路墓碑归并引擎（支持 `cap.lsm_sst` 自动动态挂载，详见 `doc/lib/lsm_sst.md`）（5/5 PASS，自动动态挂载 `cap.lsm_sst`）
- **新增标准库**：`lib/lsm_sst/{block,bloom,sstable,index}.cp`，实现 Restart Points 二分查找前缀压缩块、`BloomFilter`、`"CPSST001"` Footer + `Crc32Iscsi` 块校验 `SstBuilder`/`SstReader` 及支持墓碑覆盖的 `SstMergeIter`。
- **新增文档与测试**：`doc/lib/lsm_sst.md` 扁平文档与 `cap/test/test_lsm_sst_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:http2_frame` 纯 CP RFC 9113 HTTP/2 9 字节帧头、10 种帧编解码、半包黏包流式拆包与流控状态机引擎（支持 `cap.http2_frame` 自动动态挂载，详见 `doc/lib/http2_frame.md`）（5/5 PASS，自动动态挂载 `cap.http2_frame`）
- **新增标准库**：`lib/http2_frame/{types,codec,decoder,window,index}.cp`，实现 RFC 9113 全 10 种帧类型编解码、`H2StreamDecoder` `1..7` 字节切片拆包、`FRAME_SIZE_ERROR` 校验及 `H2ConnectionWindow` 连接/流级流控窗口与状态机。
- **新增文档与测试**：`doc/lib/http2_frame.md` 扁平文档与 `cap/test/test_http2_frame_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:grpc_wire` 纯 CP gRPC over HTTP/2 5 字节 Length-Prefixed 增量分帧、grpc-timeout 六单位与状态头编解码引擎（支持 `cap.grpc_wire` 自动动态挂载，详见 `doc/lib/grpc_wire.md`）（5/5 PASS，自动动态挂载 `cap.grpc_wire`）
- **新增标准库**：`lib/grpc_wire/{status,headers,frame,index}.cp`，实现 17 种 `GrpcStatus`、`parseServiceMethodPath`、`grpc-timeout` 六单位防溢出编解码、`grpc-message` 百分号转义、`-bin` Base64 头及支持 `1..7` 字节切片拆包的 `GrpcStreamFramer`。
- **新增文档与测试**：`doc/lib/grpc_wire.md` 扁平文档与 `cap/test/test_grpc_wire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:kafka_wire` 纯 CP Apache Kafka Wire Protocol、ApiVersions/Metadata、流式分帧与 CRC32C RecordBatch v2 引擎（支持 `cap.kafka_wire` 自动动态挂载，详见 `doc/lib/kafka_wire.md`）（5/5 PASS，自动动态挂载 `cap.kafka_wire`）
- **新增标准库**：`lib/kafka_wire/{wire,protocol,record_batch,index}.cp`，实现 ZigZag Varint/Compact String/Tagged Fields 原语、`ApiVersions`/`Metadata` 请求响应编解码、`KafkaStreamFramer` `1..7` 字节半包黏包拆包及 Castagnoli CRC32C 校验的 `RecordBatch v2`。
- **新增文档与测试**：`doc/lib/kafka_wire.md` 扁平文档与 `cap/test/test_kafka_wire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:mysql_wire` 纯 CP MySQL Client/Server Wire Protocol 报文编解码、增量分帧与双 SHA-1 认证引擎（支持 `cap.mysql_wire` 自动动态挂载，详见 `doc/lib/mysql_wire.md`）（5/5 PASS，自动动态挂载 `cap.mysql_wire`）
- **新增标准库**：`lib/mysql_wire/{packet,auth,protocol,index}.cp`，实现 Length-Encoded 原语、`MysqlStreamFramer` `1..7` 字节半包黏包拆包、`HandshakeV10`/`HandshakeResponse41` 与 `mysql_native_password` 双 SHA-1 认证，以及 `OK`/`ERR`/`EOF`/`COM`/`ColumnDef41`/`TextResultRow` 编解码。
- **新增文档与测试**：`doc/lib/mysql_wire.md` 扁平文档与 `cap/test/test_mysql_wire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:pgwire` 纯 CP PostgreSQL v3.0 Wire Protocol 报文编解码、增量半包拆包与 MD5 认证引擎（支持 `cap.pgwire` 自动动态挂载，详见 `doc/lib/pgwire.md`）（5/5 PASS，自动动态挂载 `cap.pgwire`）
- **新增标准库**：`lib/pgwire/{types,codec,stream,auth,index}.cp`，实现 Frontend (`StartupMessage`/`SslRequest`/`Query`/`Parse`/`Bind`/`Execute`/`Sync`) 与 Backend (`Authentication`/`RowDescription`/`DataRow`/`ErrorResponse`/`ReadyForQuery`) 编解码、`1..7` 字节半包黏包流式解析及 `computePgMd5Password`。
- **新增文档与测试**：`doc/lib/pgwire.md` 扁平文档与 `cap/test/test_pgwire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:gossip` 纯 CP Sans-IO SWIM 分布式集群成员发现、Incarnation 冲突消解与故障探测引擎（支持 `cap.gossip` 自动动态挂载，详见 `doc/lib/gossip.md`）（5/5 PASS，自动动态挂载 `cap.gossip`）
- **新增标准库**：`lib/gossip/{types,wire,node,index}.cp`，实现 SWIM 成员状态冲突消解、Piggyback 感染传播队列、紧凑二进制 Wire 编解码及 Sans-IO `SwimClusterNode`（支持 `Ping`/`PingReq`/`Ack`/`Sync` 与自证清白 Refutation）。
- **新增文档与测试**：`doc/lib/gossip.md` 扁平文档与 `cap/test/test_gossip_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:a2a` 纯 CP Agent-to-Agent (A2A) 多智能体协作协议、AgentCard 发现、Task 状态机与 SSE 事件编解码引擎（支持 `cap.a2a` 自动动态挂载，详见 `doc/lib/a2a.md`）（5/5 PASS，自动动态挂载 `cap.a2a`）
- **新增标准库**：`lib/a2a/{protocol,server,index}.cp`，实现 RFC 8259 转义 `AgentCard`、7 态不可逆 `TaskState` 状态机、`tasks/send`/`tasks/get`/`tasks/cancel` JSON-RPC 调度与 `TaskStatusUpdateEvent`/`TaskArtifactUpdateEvent` SSE 流式编解码。
- **新增文档与测试**：`doc/lib/a2a.md` 扁平文档与 `cap/test/test_a2a_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:raft` 纯 CP Sans-IO Raft 分布式共识状态机、日志复制、冲突回退与快照截断引擎（支持 `cap.raft` 自动动态挂载，详见 `doc/lib/raft.md`）（5/5 PASS，自动动态挂载 `cap.raft`）
- **新增标准库**：`lib/raft/{types,log,node,index}.cp`，实现确定性 Sans-IO `RaftNode`、选举定时器与 Log Up-to-Date 投票校验、Leader `AppendEntries` 冲突回退优化、Raft §5.4.2 仅提交当前任期规则及 `RaftLog.compactTo` 快照基线管理。
- **新增文档与测试**：`doc/lib/raft.md` 扁平文档与 `cap/test/test_raft_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:mcp` 纯 CP Model Context Protocol (MCP 2025-03-26) Agent 工具/资源/提示词路由与 NDJSON/Content-Length 增量分帧引擎（支持 `cap.mcp` 自动动态挂载，详见 `doc/lib/mcp.md`）（5/5 PASS，自动动态挂载 `cap.mcp`）
- **新增标准库**：`lib/mcp/{protocol,framer,server,index}.cp`，实现 MCP 2025-03-26 能力协商、`tools`/`resources`/`prompts` 路由、`isError: true` 自动封装与支持 `1..7` 字节半包黏包及 `max_frame_bytes` 防护的 `McpStreamFramer`。
- **新增文档与测试**：`doc/lib/mcp.md` 扁平文档与 `cap/test/test_mcp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:wireguard` 纯 CP WireGuard Noise_IKpsk2 握手状态机、TAI64N 单调时间戳与 RFC 6479 2048-bit 滑动窗口防重放引擎（支持 `cap.wireguard` 自动动态挂载，详见 `doc/lib/wireguard.md`）（5/5 PASS，自动动态挂载 `cap.wireguard`）
- **新增标准库**：`lib/wireguard/{replay,noise,index}.cp`，实现零堆 2048-bit `ReplayWindow`、`Tai64n` 编解码与完整 `Noise_IKpsk2`（X25519+BLAKE2s+ChaCha20Poly1305）端到端握手与双向传输帧加解密。
- **新增文档与测试**：`doc/lib/wireguard.md` 扁平文档与 `cap/test/test_wireguard_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:arrow_ipc` 纯 CP Apache Arrow IPC Streaming 与 `"ARROW1"` File Format 列式内存批处理及零拷贝读写引擎（支持 `cap.arrow_ipc` 自动动态挂载，详见 `doc/lib/arrow_ipc.md`）（5/5 PASS，自动动态挂载 `cap.arrow_ipc`）
- **新增标准库**：`lib/arrow_ipc/{types,flatbuf,ipc,index}.cp`，实现 5 种列类型（含 LSB-first `ValidityBitmap` 与 32-bit offset `Utf8`）、轻量确定性 Flatbuffer `Schema`/`RecordBatch`/`Footer` 编解码、8 字节对齐 Streaming (`0xFFFFFFFF` EOS) 与 `"ARROW1"` File Format 读写。
- **新增文档与测试**：`doc/lib/arrow_ipc.md` 扁平文档与 `cap/test/test_arrow_ipc_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ssh_wire` 纯 CP RFC 4251 SSH-2 Wire 编解码、OpenSSH 公私钥解析与 SSHSIG 签名验签引擎（支持 `cap.ssh_wire` 自动动态挂载，详见 `doc/lib/ssh_wire.md`）（5/5 PASS，自动动态挂载 `cap.ssh_wire`）
- **新增标准库**：`lib/ssh_wire/{wire,keys,openssh_key,sshsig,index}.cp`，实现 RFC 4251 `SshReader`/`SshWriter`（含 `mpint` 规范补码）、`ssh-ed25519`/`ecdsa-sha2-nistp256` 公钥与 `SHA256:` 指纹、`openssh-key-v1` 私钥解析及 `SSHSIG` PEM 签名验签。
- **新增文档与测试**：`doc/lib/ssh_wire.md` 扁平文档与 `cap/test/test_ssh_wire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:parquet` 纯 CP Apache Parquet 列式容器读写、Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码引擎（支持 `cap.parquet` 自动动态挂载，详见 `doc/lib/parquet.md`）（5/5 PASS，自动动态挂载 `cap.parquet`）
- **新增标准库**：`lib/parquet/{types,thrift,rle,codec,index}.cp`，实现 ZigZag Varint 与 32 层深度防护 `ThriftReader`/`ThriftWriter`、Parquet RLE/Bit-Packed 混合编解码、`PAR1` 容器校验及 `INT32`/`INT64`/`DOUBLE`/`BYTE_ARRAY`/`BOOLEAN` 列 `writeParquet`/`readParquet`。
- **新增文档与测试**：`doc/lib/parquet.md` 扁平文档与 `cap/test/test_parquet_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:webauthn` 纯 CP FIDO2/WebAuthn Passkey 服务端注册与登录断言验证引擎（复用 `cap:cose`，详见 `doc/lib/webauthn.md`）（5/5 PASS，自动动态挂载 `cap.webauthn`）
- **新增标准库**：`lib/webauthn/{types,auth_data,client_data,verify,index}.cp`（复用 `cap:cose`），实现 `AuthenticatorData` 二进制拆包与 `COSE_Key` 提取、`ClientDataJSON` 常量时间 Challenge/Origin 校验、`verifyRegistrationNone` 与支持 ES256 DER / EdDSA 64B 验签及 `signCount` 防克隆检测的 `verifyAssertion`。
- **新增文档与测试**：`doc/lib/webauthn.md` 扁平文档与 `cap/test/test_webauthn_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

