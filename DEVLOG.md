# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:parquet` 纯 CP Apache Parquet 列式容器读写、Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码引擎（支持 `cap.parquet` 自动动态挂载，详见 `doc/lib/parquet.md`）（5/5 PASS，文档直写 `doc/lib/parquet.md`）
- **核心设计**：内置独立 Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码器实现零外部依赖 Parquet 列式读写，严格拦截缺失 `PAR1` 魔数、越界 Footer 长度与截断页，全路径零泄漏。

## 2026-11-21: 新建 `cap:webauthn` 纯 CP FIDO2/WebAuthn Passkey 服务端注册与登录断言验证引擎（复用 `cap:cose`，详见 `doc/lib/webauthn.md`）（5/5 PASS，文档直写 `doc/lib/webauthn.md`）
- **核心设计**：直接复用 `cap:cose` 解码 `AttestedCredentialData` 公钥；对 `challenge` 采用常量时间比对、`signCount` 实施防克隆递增拦截，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:cose` 纯 CP RFC 9052/9053 COSE_Sign1 (EdDSA/ES256)、COSE_Mac0 (HS256) 与 COSE_Key CBOR 编解码引擎（支持 `cap.cose` 自动动态挂载，详见 `doc/lib/cose.md`）（5/5 PASS，文档直写 `doc/lib/cose.md`）
- **核心设计**：内置确定性 RFC 8949 CBOR 读写器以消除外部模块耦合，严格按 RFC 9052 构造 `Sig_structure`/`MAC_structure` 并在全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:paseto` 纯 CP PASETO v4.local (XChaCha20+BLAKE2b) 与 v4.public (Ed25519) 安全无算法混淆令牌引擎（支持 `cap.paseto` 自动动态挂载，详见 `doc/lib/paseto.md`）（5/5 PASS，文档直写 `doc/lib/paseto.md`）
- **核心设计**：严格执行 PASETO v4 密钥拆分（`Blake2b(448)` 派生 `Ek`/`n2` 与 `Blake2b256` 派生 `Ak`）及 `PAE([header, payload, footer, implicit_assertion])`，以 `timing_safe.eql` 杜绝认证标签侧信道，全路径零泄漏。

## 2026-11-21: 新建 `cap:zlib` 纯 CP RFC 1950 Zlib、RFC 1952 多成员 Gzip 与 Raw Deflate 统一压缩解压引擎（吸收对标 Bun `src/zlib/`，复用 `cap:deflate`，详见 `doc/lib/zlib.md`）（5/5 PASS，文档直写 `doc/lib/zlib.md`）
- **核心设计**：自研 `decompressDeflatePrefix` 精确切分多成员 Gzip 拼接流边界并在解码过程实时拦截 `max_output_bytes` 膨胀炸弹，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:libarchive` 纯 CP 归档魔数探测、Unix Ar、SVR4 CPIO (newc/crc) 与 USTAR Tar 编解码及路径防穿越引擎（吸收对标 Bun `src/libarchive/`，详见 `doc/lib/libarchive.md`）（5/5 PASS，文档直写 `doc/lib/libarchive.md`）
- **核心设计**：零 C `libarchive` 外部依赖实现 Ar/Cpio/Tar 三格式统一 `ArchiveEntry` 模型；解码入口严格拦截 `..` 目录穿越、空字节注入与总解包字节超限，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:bun_alloc` 纯 CP 零堆 BumpAllocator、O(1) 对象池 PoolAllocator、泄漏审计 TrackingAllocator 与可回滚 ScratchArena 内存分配器套件（吸收对标 Bun `src/bun_alloc/`，详见 `doc/lib/bun_alloc.md`）（5/5 PASS，文档直写 `doc/lib/bun_alloc.md`）
- **核心设计**：全部自定义分配器实现标准 `std.mem.Allocator` VTable 接口，无缝接入任意 CP 标准库数据结构；`ScratchArena` 支持解析器临时区段局部 `rewind`，全路径零泄漏。

## 2026-11-21: 新建 `cap:base64` 纯 CP RFC 4648 Base64/Base64URL 查表编解码、MIME 空白忽略与流式分块引擎（吸收对标 Bun `src/base64/`，详见 `doc/lib/base64.md`）（5/5 PASS，文档直写 `doc/lib/base64.md`）
- **核心设计**：使用 256 字节 O(1) 解码查表并与 `resolver.rs` 优先真实目录路由协同自动挂载 `cap.base64`；严格执行 RFC 4648 §3.5 规范填充校验，全路径零泄漏。

## 2026-11-21: 新建 `cap:exe_format` 纯 CP ELF64/PE32+/Mach-O 64/WASM/Shebang 魔数探测、节区解析与自包含可执行尾部载荷读写引擎（吸收对标 Bun `src/exe_format/`，详见 `doc/lib/exe_format.md`）（5/5 PASS，文档直写 `doc/lib/exe_format.md`）
- **核心设计**：全部头部/节区表解析采用零拷贝边界安全小端读取并拦截畸形偏移；`StandaloneTrailer` 在文件末尾写入定长魔数、长度与 FNV-1a 64 校验和，支持篡改与截断拦截。

## 2026-11-21: 新建 `cap:shell_parser` 纯 CP POSIX/Bash Shell 词法分词、管道与重定向 AST 解析及防注入单引号转义引擎（吸收对标 Bun `src/shell_parser/`，详见 `doc/lib/shell_parser.md`）（5/5 PASS，文档直写 `doc/lib/shell_parser.md`）
- **核心设计**：词法层严格拦截 `UnterminatedQuote` 与 `TrailingEscape`，语法层拦截连续空管道 `UnexpectedToken`；`quoteArg` 采用 POSIX 安全单引号与内部 `'` 转义，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:threading` 纯 CP Futex 驱动零堆并发同步原语集（ResetEvent/WaitGroup/CountDownLatch/Semaphore/SpinLock/OnceFlag/Barrier，吸收对标 Bun `src/threading/`，详见 `doc/lib/threading.md`）（5/5 PASS，文档直写 `doc/lib/threading.md`）
- **核心设计**：全部 7 种同步原语基于 `std.atomic.Value` 与 `std.Thread.Futex` 实现零堆分配并发协同，CAS 防计数下溢，`Barrier` 通过 generation 计数支持多轮复用。

## 2026-11-21: 新建 `cap:crash_handler` 纯 CP 结构化崩溃诊断、零堆固定缓冲格式化、环形面包屑与 RFC 8259 JSON 报告导出引擎（吸收对标 Bun `src/crash_handler/`，详见 `doc/lib/crash_handler.md`）（5/5 PASS，文档直写 `doc/lib/crash_handler.md`）
- **核心设计**：针对崩溃现场不可分配堆内存的场景设计 `FixedCrashWriter(N)` 与 `BreadcrumbRing(CAP)`，超容自动设置 `truncated` 标志而非 panic，并全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:analytics` 纯 CP 有界环形缓冲遥测收集、背压策略、敏感字段自动脱敏与 JSON/NDJSON 批量导出引擎（吸收对标 Bun `src/analytics/`，详见 `doc/lib/analytics.md`）（5/5 PASS，文档直写 `doc/lib/analytics.md`）
- **核心设计**：`AnalyticsCollector` 固定容量环形队列严格执行溢出策略与输入长度边界校验，配合 `freeBatch` 与 `DebugAllocator`/`std.testing.allocator` 验证零内存泄漏。

## 2026-11-21: 新建 `cap:sha_hmac` 纯 CP 统一密码学摘要、流式 HMAC 常量时间验签与 RFC 5869 HKDF / RFC 2898 PBKDF2 密钥派生引擎（吸收对标 Bun `src/sha_hmac/`，详见 `doc/lib/sha_hmac.md`）（5/5 PASS，文档直写 `doc/lib/sha_hmac.md`）
- **核心设计**：流式 `StreamingHasher`/`StreamingHmac` 与一次性 API 比特级等价；HKDF 严格校验 `out_len <= 255 * HashLen`、PBKDF2 拦截 `iterations == 0`，并通过 RFC 4231/5869/6070 官方向量全量验证。

## 2026-11-21: 新建 `cap:wyhash` 纯 CP Wyhash v4 高速 64-bit 非加密哈希、流式分块摘要与 Lemire 无偏差 WyRand 伪随机引擎（吸收对标 Bun `src/wyhash/`，详见 `doc/lib/wyhash.md`）（5/5 PASS，文档直写 `doc/lib/wyhash.md`）
- **核心设计**：`WyhashStream` 通过小缓冲合并保证 `1..7` 字节任意切片投喂与单次 `wyhash` 100% 比特一致；`WyRand.nextRange` 采用 Lemire 乘法无模偏差缩放，全路径零堆分配。

## 2026-11-21: 新建 `cap:csrf` 纯 CP HMAC-SHA256 CSRF 令牌、抗 BREACH 随机 XOR 掩码与 Origin/Referer/Sec-Fetch-Site 防护引擎（吸收对标 Bun `src/csrf/`，详见 `doc/lib/csrf.md`）（5/5 PASS，文档直写 `doc/lib/csrf.md`）
- **核心设计**：`maskToken` 每次生成随机 32 字节一次性 pad 与原始令牌异或，彻底消除 BREACH 压缩侧信道；`verifyOrigin` 拒绝 `Sec-Fetch-Site: cross-site`、前缀域名伪造与 `@` userinfo 欺骗，全路径零泄漏。

## 2026-11-21: 新建 `cap:collections` 纯 CP 高性能 BitSet、无堆 BoundedDeque、RingDeque、内联溢出 SmallVec 与泛型 BinaryHeap 集合库（吸收对标 Bun `src/collections/`，详见 `doc/lib/collections.md`）（5/5 PASS，文档直写 `doc/lib/collections.md`）
- **核心设计**：`BoundedDeque` 与小容量 `SmallVec` 在常见热路径上实现零堆分配；`SmallVec` 超过内联阈值自动迁移至 `Allocator` 并在 `deinit` 时统一安全回收，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:perf` 纯 CP 高分辨率作用域计时、P95/P99 热点聚合与 Chrome Trace Event JSON 导出引擎（吸收对标 Bun `src/perf/`，详见 `doc/lib/perf.md`）（5/5 PASS，文档直写 `doc/lib/perf.md`）
- **核心设计**：支持确定性虚拟时钟注入实现无抖动测试；`PerfRecorder` 严格校验空名称、未配对 `end` 与最大容量限制，并在 `std.testing.allocator` 下全路径零泄漏。

## 2026-11-21: 新建 `cap:paths` 纯 CP POSIX/Windows 双风格路径解析、规范化与防目录穿越安全边界引擎（吸收对标 Bun `src/paths/`，详见 `doc/lib/paths.md`）（5/5 PASS，文档直写 `doc/lib/paths.md`）
- **核心设计**：`parse`/`dirname`/`basename`/`extname` 保持零堆切片视图；`isSubpath`/`safeJoin` 在规范化后按分段对比，免疫 `/var/www` vs `/var/www2` 前缀陷阱并拦截空字节与绝对路径/盘符注入。

## 2026-11-21: 新建 `cap:tlv` 纯 CP ASN.1 BER/DER Tag-Length-Value 严格编解码、OID 双向互转与流式构建引擎（支持 `cap.tlv` 自动动态挂载，详见 `doc/lib/tlv.md`）（5/5 PASS，文档直写 `doc/lib/tlv.md`）
- **核心设计**：`TlvNode` 对原语载荷采用零拷贝 `raw_value` 切片视图；严格拦截 `0x80` 不定长与非最短 DER 长度编码，并在 `std.testing.allocator` 下全路径零泄漏。

## 2026-11-21: 新建 `cap:semver_range` 纯 CP Semver 2.0 语义化版本解析、npm/Cargo 范围表达式求值与选版引擎（支持 `cap.semver_range` 自动动态挂载，详见 `doc/lib/semver_range.md`）（5/5 PASS，文档直写 `doc/lib/semver_range.md`）
- **核心设计**：按 Semver 2.0 第 11 条优先级规则实现预发布数字/字母比对并忽略 `+build`；范围求值遵循 npm 同 `[major, minor, patch]` 元组预发布隔离规则，并在 `std.testing.allocator` 下全路径零泄漏。

## 2026-11-21: 新建 `cap:ulid` 纯 CP Crockford Base32 128-bit 单调递增 ULID 编解码、UUID 互转与 CSPRNG 生成器引擎（支持 `cap.ulid` 自动动态挂载，详见 `doc/lib/ulid.md`）（5/5 PASS，文档直写 `doc/lib/ulid.md`）
- **核心设计**：全路径零堆分配栈上编解码，`MonotonicUlidGenerator` 使用 `std.crypto.random.bytes` 初始化熵并在同毫秒内对 80-bit 大端递增，溢出时严格返回 `UlidError.MonotonicOverflow` 杜绝碰撞。

## 2026-11-21: 新建 `cap:graph` 纯 CP 有向加权图、DAG 拓扑排序、显式栈 Tarjan SCC 与 Dijkstra/Bellman-Ford 最短路径引擎（详见 `doc/lib/graph.md`）（5/5 PASS，文档直写 `doc/lib/graph.md`）
- **核心设计**：`findCycle` 与 `tarjanScc` 均采用显式栈迭代实现以规避深图递归栈溢出；`DirectedGraph` 与所有算法返回值严格配合 `std.testing.allocator` 实现零内存泄漏。

## 2026-11-21: 新建 `cap:idna` 纯 CP RFC 3492 Punycode 编解码与 IDNA 国际化域名转换校验引擎（支持 `cap.idna` 自动动态挂载，详见 `doc/lib/idna.md`）（5/5 PASS，文档直写 `doc/lib/idna.md`）
- **核心设计**：在 `punycodeEncode`/`punycodeDecode` 中全面执行 `u32` 乘法与加法溢出保护；`isValidDomain` 使用固定栈缓冲实现零堆分配验证，并完整阻断双向控制符与首尾连字符畸形域名。

## 2026-11-21: 新建 `cap:bencode` 纯 CP BEP-3 BitTorrent Bencode 严格编解码、规范化字典排序与零拷贝 InfoHash SHA-1 引擎（支持 `cap.bencode` 自动动态挂载，详见 `doc/lib/bencode.md`）（5/5 PASS，文档直写 `doc/lib/bencode.md`）
- **核心设计**：解码端在分配容器前先校验整数规范性与字典严格字典序，`extractRawSection` 直接在原始字节流跳过标量与嵌套结构定位 `"info"` 原始区间，确保 `info_hash` 计算零 AST 重编码偏差。

## 2026-11-21: 新建 `cap:websocket_frame` 纯 CP RFC 6455 WebSocket 帧协议、XOR 掩码、分片重组与控制帧交织引擎（支持 `cap.websocket_frame` 自动动态挂载，详见 `doc/lib/websocket_frame.md`）（5/5 PASS，文档直写 `doc/lib/websocket_frame.md`）
- **核心设计**：零堆分配计算 `Sec-WebSocket-Accept` 28 字节指纹；`WsStreamAssembler` 严格遵循 RFC 6455 §5.4 允许分片消息中途插入 Ping/Pong/Close 控制帧，并在组装完毕后执行完整 UTF-8 与 Close 状态码校验，全路径零泄漏。

## 2026-11-21: 新建 `cap:oauth2` 纯 CP RFC 6749/7636 OAuth 2.0、PKCE S256 与 OIDC 协议引擎（支持 `cap.oauth2` 自动动态挂载，详见 `doc/lib/oauth2.md`）（5/5 PASS，文档直写 `doc/lib/oauth2.md`）
- **核心设计**：使用 `getrandom` 拒绝采样消除 PKCE 生成模偏差，以常量时间比对防范 `state`/Verifier 时序侧信道；`TokenResponse` 兼容标准令牌与 OAuth2 错误 JSON 解析并在全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:histogram` 纯 CP HDR 延迟直方图与分位统计引擎（5/5 PASS，文档直写 `doc/lib/histogram.md`）
- **核心设计**：采用对数-线性子分桶在微秒至小时跨度内保持 O(log N) 空间复杂度，并支持 Coordinated Omission 补偿与跨线程直方图无损 `merge`；`SlidingWindowCounter` 仅在初始化时单次分配环形槽位，热路径零堆分配。

## 2026-11-21: 新建 `cap:unicode` 纯 CP UTF-8/UTF-16、UAX #29 Grapheme 与终端显示宽度引擎（5/5 PASS，文档直写 `doc/lib/unicode.md`）
- **核心设计**：零外部 `uucode` C/Zig 构表依赖，纯 CP 实现 UAX #29 簇边界状态机与 East Asian Width/Emoji 宽度表；`displayWidth` 支持跳过 ANSI CSI/OSC 控制序列，`truncateByWidth` 确保不切碎多字节与组合 Emoji 簇。

## 2026-11-21: 新建 `cap:picohttp` 纯 CP 零堆分配 HTTP/1.1 & Chunked 流式解析引擎（5/5 PASS，文档直写 `doc/lib/picohttp.md`）
- **核心设计**：零 C 绑定、零堆分配完成 HTTP/1.1 Pipelining 偏移解析与严格 `tchar`/CRLF 注入防护；`ChunkedDecoder` 原地紧缩与非破坏性流式双模式兼顾高性能与低内存。

## 2026-11-21: 新建 `cap:interchange` 纯 CP JSON/JSONC/JSON5/TOML/YAML 统一数据交换引擎（5/5 PASS，文档直写 `doc/lib/interchange.md`）
- **核心设计**：以独立拥有的 `InterchangeValue` 树为中间表示，JSON/JSONC/JSON5 由原生纯 CP 状态机解析（支持注释、单引号、未引号键、`Infinity`/`NaN` 与代理对转义），TOML/YAML 复用现有 `cap:toml`/`cap:yaml` 后深拷贝并立即释放原文档，实现零泄漏格式互转。

## 2026-11-21: 新建 `cap:valkey` 纯 CP RESP2/RESP3 Valkey/Redis 协议编解码与增量流式解析引擎（5/5 PASS，文档直写 `doc/lib/valkey.md`）
- **核心设计**：剔除所有 Bun/JSValue 耦合，基于 `std.mem.Allocator` 实现可自释放 `RespValue` 树；`RespStreamParser` 在预分配前先校验长度与嵌套深度上限，支持 `1..7` 字节切片连续投喂并精准区分 `Incomplete` 与协议异常。

