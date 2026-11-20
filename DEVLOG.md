# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:otlp` 纯 CP OpenTelemetry OTLP (Traces/Metrics/Logs) JSON 导出与 W3C TraceContext/Baggage 传播引擎（支持 `cap.otlp` 自动动态挂载，详见 `doc/lib/otlp.md`）（5/5 PASS，文档直写 `doc/lib/otlp.md`）
- **核心设计**：`parseTraceParent` 拒绝全零 ID 与 `ff` 版本，`parseBaggage` 强制执行 `8192B`/`180` 项上限防护；OTLP JSON 导出器将 64 位整数序列化为十进制字符串以兼容 Protobuf-JSON 规范，全路径零泄漏。

## 2026-11-21: 新建 `cap:mqtt` 纯 CP MQTT v3.1.1/v5.0 变长整数、全报文编解码、1..7B 流式分帧与 §4.7 Topic 通配符路由引擎（支持 `cap.mqtt` 自动动态挂载，详见 `doc/lib/mqtt.md`）（5/5 PASS，文档直写 `doc/lib/mqtt.md`）
- **核心设计**：解码器严格校验第 4 字节 VarInt 延续位防溢出、QoS 与 `packet_id` 一致性及 `max_packet_bytes` 防资源耗尽；`topicMatches` 完整实现 `+`/`#` 与 `$SYS` 前置隔离，全路径零泄漏。

## 2026-11-21: 新建 `cap:jq` 纯 CP `jq` JSON 结构化查询、多结果流展开、管道组合与有界防 DoS 转换引擎（支持 `cap.jq` 自动动态挂载，详见 `doc/lib/jq.md`）（5/5 PASS，文档直写 `doc/lib/jq.md`）
- **核心设计**：采用扁平节点池存储 Filter AST 消除深递归释放复杂度，求值器内置 `MAX_JQ_EVAL_DEPTH = 128` 与 `MAX_JQ_OUTPUTS = 65536` 展开上限防资源耗尽，全路径零泄漏。

## 2026-11-21: 新建 `cap:cel` 纯 CP Common Expression Language (CEL) 安全策略表达式编译、宏推导与有界沙盒求值引擎（支持 `cap.cel` 自动动态挂载，详见 `doc/lib/cel.md`）（5/5 PASS，文档直写 `doc/lib/cel.md`）
- **核心设计**：采用非图灵完备有界 `max_steps` 沙盒求值器与严格类型规则（禁止隐式 Int+String），全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:graphql` 纯 CP GraphQL 词法分词、递归下降 AST、块字符串解析、Fragment 循环检测与深度/复杂度防 DoS 引擎（支持 `cap.graphql` 自动动态挂载，详见 `doc/lib/graphql.md`）（5/5 PASS，文档直写 `doc/lib/graphql.md`）
- **核心设计**：`validator.cp` 对命名 Fragment 展开执行三色环路检测与最大深度/复杂度预算拦截，彻底阻断恶意深嵌套 GraphQL 查询，全路径零泄漏。

## 2026-11-21: 新建 `cap:sql_ast` 纯 CP SQL 词法分词、递归下降 DML/DDL/SELECT AST 解析、指纹脱敏与堆叠注入拦截引擎（支持 `cap.sql_ast` 自动动态挂载，详见 `doc/lib/sql_ast.md`）（5/5 PASS，文档直写 `doc/lib/sql_ast.md`）
- **核心设计**：词法与语法层完整拦截未闭合字符串/块注释、深度超限与多语句 `; DROP` 堆叠注入，`normalizeSqlFingerprint` 自动脱敏字面量为 `?`，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:dwarf` 纯 CP DWARF v4/v5 `.debug_line` 行号表状态机与 `.debug_abbrev`/`.debug_info`/`.debug_str` DIE 调试信息解析引擎（支持 `cap.dwarf` 自动动态挂载，详见 `doc/lib/dwarf.md`）（5/5 PASS，文档直写 `doc/lib/dwarf.md`）
- **核心设计**：`.debug_line` 状态机内置循环步数上限与 `file_index` 越界防护，`lookupAddress` 支持区间二分定位 `file:line:column`，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:wasm_fmt` 纯 CP WebAssembly `.wasm` 二进制解析、LEB128 5 字节溢出防护与 Custom Section 构建引擎（支持 `cap.wasm_fmt` 自动动态挂载，详见 `doc/lib/wasm_fmt.md`）（5/5 PASS，文档直写 `doc/lib/wasm_fmt.md`）
- **核心设计**：`parseWasm` 严格校验标准 Section ID 单调递增与 Payload 边界，禁止重复标准段并拦截第 5 字节符号位溢出，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:onnx` 纯 CP ONNX ModelProto/GraphProto/TensorProto 二进制编解码、权重提取与三色 DFS 拓扑无环校验引擎（支持 `cap.onnx` 自动动态挂载，详见 `doc/lib/onnx.md`）（5/5 PASS，文档直写 `doc/lib/onnx.md`）
- **核心设计**：内置确定性 Protobuf Wire 读写器并严格执行 `CyclicGraph`/`DuplicateOutput` 与越界长度拦截，全路径通过 `std.testing.allocator` / `DebugAllocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:attention` 纯 CP Transformer Scaled Dot-Product/GQA 注意力、支持 Speculative 回滚的 KvCache 与 Top-K/Top-P/RepetitionPenalty 采样器引擎（支持 `cap.attention` 自动动态挂载，详见 `doc/lib/attention.md`）（5/5 PASS，文档直写 `doc/lib/attention.md`）
- **核心设计**：SDPA 减去行最大值保证指数数值稳定，GQA 严格校验 `num_q_heads % num_kv_heads == 0`，`KvCache` 支持 Speculative Decoding 步长回滚，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:tensor` 纯 CP Strided 多维张量视图、广播代数、矩阵乘法与 Transformer 核心算子（Softmax/RMSNorm/LayerNorm/SiLU/GELU/RoPE）引擎（支持 `cap.tensor` 自动动态挂载，详见 `doc/lib/tensor.md`）（5/5 PASS，文档直写 `doc/lib/tensor.md`）
- **核心设计**：`Tensor` 支持 `MAX_TENSOR_DIMS = 4` 的零拷贝步幅视图与连续化检查；`softmaxRows` 采用减去行最大值防溢出，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:gguf` 纯 CP GGUF v2/v3 llama.cpp 本地大模型二进制格式解析、对齐写入与 Q8_0/Q4_0 块量化反量化引擎（支持 `cap.gguf` 自动动态挂载，详见 `doc/lib/gguf.md`）（5/5 PASS，文档直写 `doc/lib/gguf.md`）
- **核心设计**：`parseGguf` 严格校验数组嵌套深度/长度炸弹上限与张量数据区间不越界总长度，`Q8_0`/`Q4_0` 实现 32 权重/块精确反量化，全路径零泄漏。

## 2026-11-21: 新建 `cap:safetensors` 纯 CP HuggingFace Safetensors 模型权重二进制解析（含溢出/越界/重叠拦截）、Builder 与 IEEE-754 BF16/FP16 位级转换引擎（支持 `cap.safetensors` 自动动态挂载，详见 `doc/lib/safetensors.md`）（5/5 PASS，文档直写 `doc/lib/safetensors.md`）
- **核心设计**：`parseSafetensors` 通过 `@mulWithOverflow` 与非空张量区间重叠检查阻断恶意模型头越界；`BF16`/`FP16` 位级转换完整覆盖次正规数、Inf/NaN 与舍入，全路径零泄漏。

## 2026-11-21: 新建 `cap:bpe` 纯 CP Byte-Pair Encoding (Tiktoken 风格) LLM Tokenizer 编解码、语料训练与 UTF-8 安全截断引擎（支持 `cap.bpe` 自动动态挂载，详见 `doc/lib/bpe.md`）（5/5 PASS，文档直写 `doc/lib/bpe.md`）
- **核心设计**：采用最小 rank 优先相邻对迭代合并与 `validUtf8PrefixLen` 截断守卫确保永不切碎多字节 UTF-8 字符，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:quant` 纯 CP AI Embedding 向量量化（Scalar Int8 对称/仿射、Binary 1-bit @popCount 汉明距离、Product Quantization K-Means & ADC 查表）引擎（支持 `cap.quant` 自动动态挂载，详见 `doc/lib/quant.md`）（5/5 PASS，文档直写 `doc/lib/quant.md`）
- **核心设计**：Binary 1-bit 利用 `@popCount` 实现单周期汉明距离；PQ K-Means 支持可配置子空间与确定性训练并在 ADC 查表阶段实现 O(M) 近似距离计算，全路径零泄漏。

## 2026-11-21: 新建 `cap:bm25` 纯 CP Okapi BM25 倒排索引全文检索、邻接短语匹配与 RRF 混合检索融合引擎（支持 `cap.bm25` 自动动态挂载，详见 `doc/lib/bm25.md`）（5/5 PASS，文档直写 `doc/lib/bm25.md`）
- **核心设计**：`Bm25Index` 在倒排表中记录词元位置列表支持二分邻接短语校验，并与 `rrfFuse` 协同为 `cap:hnsw` 混合检索提供词法打分基石，全路径零泄漏。

## 2026-11-21: 新建 `cap:hnsw` 纯 CP HNSW 多层近似最近邻向量图索引（L2/Cosine/InnerProduct、Beam Search、软删除与二进制快照，支持 `cap.hnsw` 自动动态挂载，详见 `doc/lib/hnsw.md`）（5/5 PASS，文档直写 `doc/lib/hnsw.md`）
- **核心设计**：采用确定性 SplitMix64 层高采样与双向邻接表裁剪保证图连通性与可复现性，并通过紧凑小端快照序列化与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:mvcc` 纯 CP MVCC 多版本事务快照隔离（First-Committer-Wins 写写冲突检测、Read-Your-Own-Writes 与水位线 GC）引擎（支持 `cap.mvcc` 自动动态挂载，详见 `doc/lib/mvcc.md`）（5/5 PASS，文档直写 `doc/lib/mvcc.md`）
- **核心设计**：`commit()` 检查写集内各键最新版本是否晚于 `read_ts`，冲突即自动回滚并返回 `WriteConflict`；`gcVersions` 严格保留每个键 `<= safe_watermark_ts` 的最新基准版本，全路径零泄漏。

## 2026-11-21: 新建 `cap:bplus_tree` 纯 CP 可变阶 B+ 树索引（支持分裂/借位/合并/叶子双向链表扫描）与 4KB CRC32C Slotted Page 二进制页引擎（支持 `cap.bplus_tree` 自动动态挂载，详见 `doc/lib/bplus_tree.md`）（5/5 PASS，文档直写 `doc/lib/bplus_tree.md`）
- **核心设计**：500 条乱序插入与 300 条删除全过程持续执行 `validateInvariants()` 校验叶子同深与双向链表一致性；4KB `SlottedPage` 采用数据库经典向心增长布局与尾部 CRC32C 防篡改，全路径零泄漏。

## 2026-11-21: 新建 `cap:lsm_sst` 纯 CP LSM-Tree 前缀压缩 SSTable 数据块、Kirsch-Mitzenmacher 布隆过滤器、CRC32C 校验与多路墓碑归并引擎（支持 `cap.lsm_sst` 自动动态挂载，详见 `doc/lib/lsm_sst.md`）（5/5 PASS，文档直写 `doc/lib/lsm_sst.md`）
- **核心设计**：`BlockBuilder` 每 16 条记录写一个 Restart Point 支持块内二分；`SstBuilder` 强制字典序输入并在每块尾部附加 `Crc32Iscsi` 防损坏篡改，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:http2_frame` 纯 CP RFC 9113 HTTP/2 9 字节帧头、10 种帧编解码、半包黏包流式拆包与流控状态机引擎（支持 `cap.http2_frame` 自动动态挂载，详见 `doc/lib/http2_frame.md`）（5/5 PASS，文档直写 `doc/lib/http2_frame.md`）
- **核心设计**：`H2StreamDecoder` 对 `PING`/`RST_STREAM`/`WINDOW_UPDATE` 执行定长约束与 `2^31-1` 窗口溢出检测，`H2ConnectionWindow` 精确跟踪多路复用流状态转移，全路径零泄漏。

## 2026-11-21: 新建 `cap:grpc_wire` 纯 CP gRPC over HTTP/2 5 字节 Length-Prefixed 增量分帧、grpc-timeout 六单位与状态头编解码引擎（支持 `cap.grpc_wire` 自动动态挂载，详见 `doc/lib/grpc_wire.md`）（5/5 PASS，文档直写 `doc/lib/grpc_wire.md`）
- **核心设计**：`GrpcStreamFramer` 严格校验 `compressed_flag <= 1` 与 `max_message_bytes` 防炸弹上限，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:kafka_wire` 纯 CP Apache Kafka Wire Protocol、ApiVersions/Metadata、流式分帧与 CRC32C RecordBatch v2 引擎（支持 `cap.kafka_wire` 自动动态挂载，详见 `doc/lib/kafka_wire.md`）（5/5 PASS，文档直写 `doc/lib/kafka_wire.md`）
- **核心设计**：`encodeRecordBatch` 自动计算并回填覆盖 `attributes` 至批尾的 `Crc32Iscsi` 校验和，`KafkaStreamFramer` 严格拦截超限 `max_frame_bytes`，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:mysql_wire` 纯 CP MySQL Client/Server Wire Protocol 报文编解码、增量分帧与双 SHA-1 认证引擎（支持 `cap.mysql_wire` 自动动态挂载，详见 `doc/lib/mysql_wire.md`）（5/5 PASS，文档直写 `doc/lib/mysql_wire.md`）
- **核心设计**：4 字节小端帧头支持 `max_packet_bytes` 上限拦截；`TextResultRow` 正确区分 `0xFB` NULL 列与空字符串，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:pgwire` 纯 CP PostgreSQL v3.0 Wire Protocol 报文编解码、增量半包拆包与 MD5 认证引擎（支持 `cap.pgwire` 自动动态挂载，详见 `doc/lib/pgwire.md`）（5/5 PASS，文档直写 `doc/lib/pgwire.md`）
- **核心设计**：`PgFrontendStreamDecoder` 在 `SslRequest` 后保持 `expect_startup` 状态以正确衔接后续 `StartupMessage`；全路径严格校验包长下限与 `max_packet_bytes` 防资源耗尽，零内存泄漏。

## 2026-11-21: 新建 `cap:gossip` 纯 CP Sans-IO SWIM 分布式集群成员发现、Incarnation 冲突消解与故障探测引擎（支持 `cap.gossip` 自动动态挂载，详见 `doc/lib/gossip.md`）（5/5 PASS，文档直写 `doc/lib/gossip.md`）
- **核心设计**：Sans-IO 引擎将成员探测定时器与传输分离，严格执行 `incarnation` 优先级及 `Left > Offline > Suspect > Active` 仲裁，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:a2a` 纯 CP Agent-to-Agent (A2A) 多智能体协作协议、AgentCard 发现、Task 状态机与 SSE 事件编解码引擎（支持 `cap.a2a` 自动动态挂载，详见 `doc/lib/a2a.md`）（5/5 PASS，文档直写 `doc/lib/a2a.md`）
- **核心设计**：`A2aServer` 严格执行终态不可逆转换与 `-32002 TaskNotCancelable` 拦截，内置任务/历史/产物容量上限防护，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:raft` 纯 CP Sans-IO Raft 分布式共识状态机、日志复制、冲突回退与快照截断引擎（支持 `cap.raft` 自动动态挂载，详见 `doc/lib/raft.md`）（5/5 PASS，文档直写 `doc/lib/raft.md`）
- **核心设计**：Sans-IO 架构将共识状态机与网络/存储解耦，所有输出以 `RaftAction` 动作列表返回，天然适配内存集群确定性仿真与 CP Actor 信箱驱动，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:mcp` 纯 CP Model Context Protocol (MCP 2025-03-26) Agent 工具/资源/提示词路由与 NDJSON/Content-Length 增量分帧引擎（支持 `cap.mcp` 自动动态挂载，详见 `doc/lib/mcp.md`）（5/5 PASS，文档直写 `doc/lib/mcp.md`）
- **核心设计**：`McpStreamFramer` 同时支持 NDJSON 与 LSP `Content-Length` 双传输分帧，并在分配帧缓冲前拦截超限载荷；`McpServer` 自动将业务 handler 错误转为协议合规的 `isError: true` 工具结果，全路径零泄漏。

## 2026-11-21: 新建 `cap:wireguard` 纯 CP WireGuard Noise_IKpsk2 握手状态机、TAI64N 单调时间戳与 RFC 6479 2048-bit 滑动窗口防重放引擎（支持 `cap.wireguard` 自动动态挂载，详见 `doc/lib/wireguard.md`）（5/5 PASS，文档直写 `doc/lib/wireguard.md`）
- **核心设计**：采用先验密 AEAD 再更新 `ReplayWindow` 策略杜绝伪造包污染窗口，对 `mac1` 与公钥执行常量时间比对并强制 `TAI64N` 单调递增防重放，全路径零泄漏。

## 2026-11-21: 新建 `cap:arrow_ipc` 纯 CP Apache Arrow IPC Streaming 与 `"ARROW1"` File Format 列式内存批处理及零拷贝读写引擎（支持 `cap.arrow_ipc` 自动动态挂载，详见 `doc/lib/arrow_ipc.md`）（5/5 PASS，文档直写 `doc/lib/arrow_ipc.md`）
- **核心设计**：内置确定性 Flatbuffer 读写器避免外部依赖，所有 `Buffer` 偏移/长度、`Footer` 边界与 `Utf8` offsets 均执行防越界拦截，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:ssh_wire` 纯 CP RFC 4251 SSH-2 Wire 编解码、OpenSSH 公私钥解析与 SSHSIG 签名验签引擎（支持 `cap.ssh_wire` 自动动态挂载，详见 `doc/lib/ssh_wire.md`）（5/5 PASS，文档直写 `doc/lib/ssh_wire.md`）
- **核心设计**：严格执行 RFC 4251 `mpint` 高位 `0x80` 前导 `0x00` 补码、`openssh-key-v1` `checkint1 == checkint2` 与 8 字节对齐校验，以及 `SSHSIG` `"SSHSIG"` 魔数与 `namespace` 绑定验签，全路径零泄漏。

## 2026-11-21: 新建 `cap:parquet` 纯 CP Apache Parquet 列式容器读写、Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码引擎（支持 `cap.parquet` 自动动态挂载，详见 `doc/lib/parquet.md`）（5/5 PASS，文档直写 `doc/lib/parquet.md`）
- **核心设计**：内置独立 Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码器实现零外部依赖 Parquet 列式读写，严格拦截缺失 `PAR1` 魔数、越界 Footer 长度与截断页，全路径零泄漏。

## 2026-11-21: 新建 `cap:webauthn` 纯 CP FIDO2/WebAuthn Passkey 服务端注册与登录断言验证引擎（复用 `cap:cose`，详见 `doc/lib/webauthn.md`）（5/5 PASS，文档直写 `doc/lib/webauthn.md`）
- **核心设计**：直接复用 `cap:cose` 解码 `AttestedCredentialData` 公钥；对 `challenge` 采用常量时间比对、`signCount` 实施防克隆递增拦截，全路径通过 `std.testing.allocator` 零泄漏验证。

