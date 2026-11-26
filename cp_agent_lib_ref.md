# CP 智能体标准库与生态极简速查手册 (`cp_agent_lib_ref.md`)

> **更新日期**：2026-11-27 | **自动生成**：`python3 scripts/gen_agent_lib_ref.py`
> **覆盖规模**：内置标准库 **204 通道**（`cap:*`）+ 官方 Web 插件 **34 包**（`@web/*`）+ 官方纯 CP/原生包 **704 包**（`@cap/*`）+ 上游映射 **917 条**（npm 456 / crates.io 461）。
> **AGY 查库铁律（极省 Token）**：
> 1. **写代码前先查本文件**：优先用内置 `cap:*`（零安装），其次 `import ... from "@cap/<name>"`，**严禁重复造轮子**；
> 2. **查精确签名不读全量源码**：内置库看 `doc/lib/<mod>.md` 或 `lib/<mod>/index.cp`；`@cap/*` 直接读 **`packages/@cap/<name>/src/index.d.cp`**（极短类型声明）；
> 3. **按 npm / crates.io 原库名查映射**：`rg -n "<orig>" packages/@cap/registry.json`（改名包下方已标 `←原名`）；
> 4. **日常禁止读取 370KB 的 `cp_lib_ref.md` 或万行 `cp_ref.md`**；仅新增库时追加 `cp_lib_ref.md` 并跑 `python3 scripts/gen_agent_lib_ref.py` 刷新本文件。

---

## 一、 根容器与内建全局符号（免安装）

- **惰性根容器**：`import cap from "cap";` — 编译期按需挂载 `cap.<ns>`（未引用 0 开销）。
- **免 import 内置全局**：`RegExp`（`/pat/flags`）、`JSON`（`JSON.stringify` / `JSON.parse<T>`）、并发原语 `@spawn`/`@receive`/`@reply`/`@send`/`@sender`/`@self`。

## 二、 内置标准库速查矩阵 (`cap:*`)

| 通道 | 核心入口符号 | 职责 |
| :--- | :--- | :--- |
| `cap:sort` | `Sort`, `asc`, `desc` 等 | 工业级 PDQSort 自适应快速排序、稳定块排序、原… |
| `cap:hash` | `Hash`, `wyhash`, `xxhash3` 等 | 30~50 GB/s SIMD 极速散列、Wyhash… |
| `cap:bytes` | `BytesError`, `KIB..PIB`, `KB..PB` 等 | 零分配纯整数 SI/IEC 字节大小双向解析与格式化（… |
| `cap:time/human` | `MsError`, `parseMs`, `parseDurationNs` 等 | 零分配 128-bit 定点人类可读时长解析与格式化（… |
| `cap:col`（兼容 `cap:collections`） | `Vec<T>`, `VecDeque<T>`, `BinaryHeap<T>` 等 | 完备标准容器与零堆分配紧凑集合 |
| `cap:event`（兼容 `cap:events` / `cap:event_loop`） | `EventBus`, `EventEmitter`, `Event` 等 | 工业级统一事件发布/订阅总线、高性能四阶段事件循环、N… |
| `cap:conf`（兼容 `cap:dotenv` / `cap:ini`） | `DotEnv`, `Ini`, `IniParser` 等 | 统一配置与结构化文本主包：YAML 1.2/TOML… |
| `cap:pack`（兼容 `cap:compress`） | `gzip`, `gunzip`, `deflate` 等 | 流式/全量压缩（Gzip/Zlib/Deflate/Z… |
| `cap:redis` | `RESPType`, `RESPFrame`, `asDouble` 等 | Redis / Valkey RESP2 & RESP… |
| `cap:cli` | `CliArgs`, `CliOption`, `SliceIterator` 等 | 命令行参数（含 17 项 `cap` 全命令声明式参数… |
| `cap:patch` | `UnifiedDiff`, `UnifiedHunkHeader`, `HunkRange` 等 | Unified Diff 解析、Myers 行级差异… |
| `cap:bench` | `HwTimer`, `Stopwatch`, `BenchStats` 等 | 纳秒级微基准测试、分位数统计、Welch t-test… |
| `cap:elf` | `ExeFormat`, `Elf64HeaderInfo`, `ElfInspector` 等 | ELF64 / Mach-O / PE 可执行文件头解… |
| `cap:md` | `Markdown`, `toHtmlEngine`, `toAnsi` 等 | 工业级 MD4C CommonMark / GFM 解… |
| `cap:html` | `CssSelector`, `HtmlRewriter`, `HtmlEscape` 等 | HTML5 实体编解码、白名单内容清洗、CSS 选择器… |
| `cap:css` | `CssTokenizer`, `CssColor`, `CssLength` 等 | 工业级 CSS + LightningCSS 编译器/… |
| `cap:image`（兼容 `cap:png` / `cap:img`） | `Png`, `PngImage`, `PngError` 等 | 工业级全景图像/图形统一主包：PNG/BMP/GIF/… |
| `cap:ffi` | `DynLib`, `FFIType`, `FFISignature` 等 | 跨平台动态库加载（dlopen）、C-ABI 布局计算… |
| `cap:sql` | `SqlType`, `SqlDialect`, `SqlStatement` 等 | 工业级 PostgreSQL v3（全栈端到端零堆线协… |
| `cap:sqlite` | `Database`, `open`, `openMemory` 等 | 内置自包含官方 Public Domain SQLit… |
| `cap:coff` | `CoffError`, `Machine`, `Subsystem` 等 | 零堆分配 Windows PE/COFF 二进制头、节… |
| `cap:macho` | `MachoError`, `MachoKind`, `FatArchInfo` 等 | 零堆分配 macOS/iOS Mach-O 32/64… |
| `cap:pdb` | `PdbError`, `MSF_MAGIC`, `StreamType` 等 | 零堆分配 Windows PDB MSF 7.0 调试… |
| `cap:spirv` | `SpirvError`, `SPIRV_MAGIC`, `OP_*` 等 | 零堆分配 Vulkan/WebGPU SPIR-V 着… |
| `cap:tz` | `TimeZone`, `parse`, `loadFile` 等 | 零依赖解析系统 IANA TZif 时区库（`/usr… |
| `cap:leb128` | `encodeU64`, `decodeU64`, `encodeI64` 等 | 零堆分配无符号/有符号 LEB128 变长整数编解码与… |
| `cap:progress` | `ProgressBar`, `ProgressNode`, `start` 等 | 无闪烁分层终端实时进度条与 RAII 生命周期管理（4… |
| `cap:zon` | `ZonDocument`, `parse`, `get` 等 | 零依赖 ZON 轻量配置格式解析、校验与格式化引擎（4… |
| `cap:rtc` | `RtpHeader`, `parseRtpHeader`, `writeRtpHeader` 等 | 参考 Rust `str0m`（MIT/Apache-… |
| `cap:shell`（兼容 `cap:sh`） | `Lexer`, `Parser`, `Shell` 等 | 工业级跨平台 Shell 引擎（直迁 Bun `she… |
| `cap:io` | `Reader`, `Writer`, `Closer` 等 | 统一流契约、零堆 `ArrayBufferSink`（… |
| `cap:sync` | `Mutex`, `RwLock`, `Atomic` 等 | 锁、单线程排他所有权校验锁、无锁队列、共享智能指针、并… |
| `cap:mem` | `Arena`, `SharedTensor`, `Cow` 等 | 区域内存分配、8B 紧凑可重定位外部偏移切片与零堆字节… |
| `cap:string` | `ZigString`, `WTF8`, `Wtf8` 等 | Bun 对齐全套字符串/Unicode/UTF-8/1… |
| `cap:simd` | `Simd`, `indexOfByte`, `indexOfChar` 等 | 16/32 字节 Highway 级 SIMD 硬件向… |
| `cap:json` | `JsonParser`, `JsonValue`, `JsonBufferPool` 等 | 零拷贝流式 JSON、零反射 JSON Schema… |
| 内置 `RegExp` / `JSON`（无需 `import`） | `RegExp`, `JSON`, `JSON.stringify` 等 | 内联 16 字节 SIMD + `[2048]u8`… |
| `cap:http` | `Method`, `HTTPStatus`, `HeaderBuilder` 等 | 工业级 HTTP/1.1、零堆 RFC 7231 `D… |
| `cap:ws` | `Opcode`, `CloseCode`, `FrameHeader` 等 | RFC 6455 / RFC 7692 WebSock… |
| `cap:router` | `Router<T>`, `RadixNode<T>`, `ParamMap` 等 | 极速零堆静态哈希 + Radix-Tree 路由匹配… |
| `cap:net` | `SocketAddress`, `UdpSocket`, `TcpStream` 等 | TCP/UDP 套接字、IPv4/IPv6 CIDR… |
| `cap:dns` | `DnsHeader`, `DnsRecordType`, `DnsWireCodec` 等 | RFC 1035/2782/3493/3596/848… |
| `cap:s3` | `S3Acl`, `S3StorageClass`, `S3Credentials` 等 | 完备 AWS S3/R2/Spaces/B2/MinI… |
| `cap:sourcemap` | `VLQ`, `Zigzag`, `Varint` 等 | Base64 VLQ/Varint 编解码、高精度映射… |
| `cap:clap` | `parseArgs`, `parseParam`, `parseEx` 等 | 工业级命令行参数解析引擎 |
| `cap:sys` | `Maybe<T>`, `SysError`, `SyscallTag` 等 | Bun 对齐全套 POSIX/Linux 系统调用、异… |
| `cap:blob` | `Blob`, `File`, `BlobStore` 等 | Web 标准 `Blob`/`File` 二进制对象抽… |
| `cap:path` | `path`, `Path`, `PosixPath` 等 | 跨平台路径计算、规范化、环境变量 PATH 迭代与零分… |
| `cap:url` | `URL`, `normalizePath`, `resolve` 等 | WHATWG 标准 URL 相对解析/规范化、查询参数… |
| `cap:ptr` | `Shared<T>`, `AtomicShared<T>`, `Weak<T>` 等 | 工业级智能指针、写时复制包装器、侵入式引用计数与 64… |
| `cap:glob` | `Glob`, `match`, `hasMagic` 等 | 极速 Unix Glob 通配符匹配（递归 `**`… |
| `cap:semver` | `Version`, `hash()`, `PartialVersion` 等 | Semver 2.0.0 规范预发布点分权重梯级、高级… |
| `cap:dotenv` | `EnvParser`, `EnvMap`, `DotenvLoader` 等 | 工业级 `.env` 解析器，支持引号转义、`${VA… |
| `cap:which` | `which`, `whichAll`, `isExecutable` 等 | 跨平台 `PATH`/`PATHEXT` 可执行程序定… |
| `cap:encoding` | `Base64`, `Base64Encoder`, `Base64Decoder` 等 | 高性能 Base64、十六进制、UTF-8/16/16… |
| `cap:crypto` | `sha256`, `sha512`, `Blake3` 等 | 全栈现代密码学（非对称签名/ECDH/KDF/AEAD… |
| `cap:safety` | `thread_id`, `ThreadLock`, `CriticalSection` 等 | 生产级线程安全、竞态检测、ASAN 毒化、CPUID… |
| `cap:security` | `CSRF`, `generate`, `verify` 等 | CSRF 签名、RFC 4226/6238 HOTP/… |
| `cap:errno` | `SystemErrno`, `fromCode`, `toCode` 等 | 跨平台系统错误码映射、Linux/macOS/Free… |
| `cap:logger` | `LogLevel`, `Loc`, `Range` 等 | 诊断日志、结构化 JSON/Logfmt/脱敏日志、W… |
| `cap:napi` | `Napi`, `NapiError`, `NapiStatus` 等 | 纯 CP Node-API v9 C-ABI 布局与原… |
| `cap:bigint` | `BigInt`, `BigInt256` | 任意精度大整数与 EVM/Web3 256 位定宽大数… |
| `cap:bundler` | `BundleTarget`, `OutputFormat`, `ChunkKind` 等 | 静态打包、跨平台目标、输出格式/Windows PE… |
| `cap:resolver` | `DataURL`, `PackageJson`, `TsConfigJson` 等 | 工业级模块解析、目录缓存/软链穿透、零堆元数据 64-… |
| `cap:test_runner` | `TestOrder`, `Prng64`, `TestRunner` 等 | 生产级测试套件编排、零堆调试树 `DebugTreeW… |
| `cap:time` | `TimeUnit`, `Duration`, `WallClock` 等 | 单调/墙钟、RFC 3339/ISO 8601、44… |
| `cap:csv` | `CsvOptions`, `.tsv()`, `CsvRow` 等 | RFC 4180 CSV/TSV 增量流式解析与写入… |
| `cap:uuid` | `Uuid`, `v4`, `v5` 等 | RFC 9562 UUID v4/v5/v7、ULID… |
| `cap:math` | `Vec2`, `Vec3`, `Vec4` 等 | 2D/3D/4D 线性代数、无上限外部切片密集矩阵 S… |
| `cap:rpc` | `JsonRpcId`, `JsonRpcRequest`, `JsonRpcResponse` 等 | JSON-RPC 2.0 编解码、LSP/MCP 增量… |
| `cap:fsm` | `StateMachine`, `addTransition`, `trigger` 等 | 确定性有限状态机、三态熔断器与 AI Agent 并行… |
| `cap:cache` | `LruCache<V>`, `CacheStats`, `BloomFilter` 等 | 高性能 O(1) LRU+TTL 内存缓存、布隆过滤器… |
| `cap:vector` | `VectorIndex`, `upsert`, `remove` 等 | 本地 8-lane SIMD（`@dot`/`@l2s… |
| `cap:text` | `TextChunker`, `splitRecursive`, `splitMarkdown` 等 | AI RAG 文档分块与 Markdown 面包屑分块… |
| `cap:xml` | `XmlParser`, `XmlDocument`, `XmlNode` 等 | XML 1.0 DOM 解析、5 大与数字实体解码、路… |
| `cap:metrics` | `Counter`, `Gauge`, `MetricLabel` 等 | Prometheus / OpenMetrics 指标… |
| `cap:cbor` | `BinValue`, `BinDocument`, `BinError` 等 | RFC 8949 CBOR 与 MessagePack… |
| `cap:cluster` | `ConsistentHashRing`, `MemberState`, `MembershipTable` 等 | 分布式集群一致性哈希分片、SWIM 成员表与防脑裂选主… |
| `cap:geo` | `LatLng`, `BBox`, `Geo` 等 | 地理空间测地线计算、Geohash 8 邻域与内存半径… |
| `cap:graph` | `Graph`, `kHopSubgraph`, `GraphAlgo` 等 | 图论经典算法、GraphRAG 实体子图、零堆 A*/… |
| `cap:crdt` | `VectorClock`, `CausalOrder`, `GCounter` 等 | 分布式无冲突复制数据类型 CRDT 与多 Agent… |
| `cap:color` | `Rgba`, `Hsl`, `Hsv` 等 | 色彩科学、OKLab 感知插值、WCAG 2.1 无障… |
| `cap:email` | `EmailAddress`, `QuotedPrintable`, `EmailBuilder` 等 | RFC 5322/2045/2047 邮件构建解析与… |
| `cap:git` | `GitOid`, `GitObject`, `GitTreeCodec` 等 | 零依赖 Git 对象编解码、`.gitignore`… |
| `cap:tui` | `KeyCode`, `KeyModifiers`, `InputParser` 等 | 终端 UI 布局、ANSI 键盘输入解析与零闪烁最小差… |
| `cap:wasm` | `Leb128`, `WasmValType`, `WasmExternKind` 等 | WebAssembly 1.0 二进制模块解析、规范校… |
| `cap:graphql` | `GqlError`, `GqlTokenKind`, `GqlLexer` 等 | 纯 CP GraphQL 词法、AST 解析、查询复杂… |
| `cap:prompt` | `PromptError`, `PromptRole`, `TrimStrategy` 等 | AI Agent 上下文窗口与多段优先级 Token… |
| `cap:codec` | `CodecError`, `Base58`, `encodeCheck` 等 | 纯 CP 多进制与强校验编码套件（Base58Chec… |
| `cap:yaml` | `YamlError`, `YamlKind`, `YamlDoc` 等 | ` `>`/锚点别名防环/Tab 与重复键拦截）, `… |
| `cap:toml` | `TomlError`, `TomlKind`, `TomlDoc` 等 | TOML v1.0.0 完整配置解析、冲突检测、序列化… |
| `cap:diff` | `DiffError`, `DiffOpKind`, `DiffEdit` 等 | Myers 行词级差异分析、Unified Diff… |
| `cap:ini` | `IniError`, `IniOptions`, `IniLineKind` 等 | 无损保留注释与顺序的 INI/Systemd/Gitc… |
| `cap:jsonpath` | `JsonPathError`, `PathMatch`, `JsonPath` 等 | RFC 9535 JSONPath 编译器、递归下降… |
| `cap:mime` | `MimeError`, `MimeParam`, `MediaType` 等 | RFC 6838 MediaType 解析、30 种二… |
| `cap:agentmem` | `AgentMemError`, `MemoryTier`, `RecallWeights` 等 | AI Agent 分层长期记忆、Token 预算工作记… |
| `cap:raft` | `RaftError`, `RaftRole`, `Follower` 等 | 确定性无网络耦合 Raft 共识核心状态机（选举、复制… |
| `cap:bpe` | `BpeError`, `MergePair`, `BpeTokenizer` 等 | Byte-Level BPE Tokenizer 训练… |
| `cap:bencode` | `BencodeError`, `BencodeKind`, `BencodeValue` 等 | BitTorrent BEP-3 Bencode 严格… |
| `cap:ssh` | `SshError`, `SshKeyType`, `SshPublicKey` 等 | OpenSSH 公钥 Wire 格式解析、`SHA25… |
| `cap:tlv` | `TlvError`, `FrameHeaderKind`, `crc32c` 等 | 二进制 TLV 编解码、黏包/半包流式分帧与 Cast… |
| `cap:paseto` | `PasetoError`, `PasetoPurpose`, `PasetoParsed` 等 | PASETO v4.local & v4.public… |
| `cap:mqtt` | `MqttError`, `MqttPacketType`, `MqttQos` 等 | MQTT 3.1.1 二进制 Wire 报文编解码与增… |
| `cap:syslog` | `SyslogError`, `Facility`, `Severity` 等 | RFC 5424 & RFC 3164 结构化 Sys… |
| `cap:hpack` | `HpackError`, `HeaderField`, `IndexingMode` 等 | RFC 7541 HTTP/2 HPACK 头部压缩编… |
| `cap:geohash` | `GeohashError`, `LatLon`, `BoundingBox` 等 | 零分配 Geohash 空间网格编码、8 邻域拓扑、6… |
| `cap:crdt` | `CrdtErr`, `CausalOrder`, `VectorClock` 等 | 分布式无冲突复制数据类型（State-Based Cv… |
| `cap:cron` | `CronError`, `UtcDateTime`, `CronSchedule` 等 | 纯 CP 5/6 字段 Cron 表达式解析与下次触发… |
| `cap:fsm` | `FsmError`, `TransitionRecord`, `GuardFn` 等 | 确定性有限状态机 DFA、Guard 守卫、拓扑分析与… |
| `cap:cbor` | `CborError`, `CborMapEntry`, `CborTag` 等 | RFC 8949 CBOR 确定性编码与 WebAut… |
| `cap:deflate` | `DeflateError`, `crc32`, `adler32` 等 | 纯 CP RFC 1951 DEFLATE、RFC 1… |
| `cap:png` | `PngError`, `ColorType`, `PngImage` 等 | 纯 CP PNG 图像无损编解码与 Paeth 滤波引… |
| `cap:wav` | `WavError`, `SampleFormat`, `WavAudio` 等 | 纯 CP RIFF/WAVE PCM 音频容器编解码… |
| `cap:bmp` | `BmpError`, `BmpPixelFormat`, `BmpImage` 等 | 纯 CP BMP 位图图像容器编解码与行对齐引擎（支持… |
| `cap:lzw` | `LzwError`, `LzwBitOrder`, `LsbFirst` 等 | 可变位宽 LZW 字典无损压缩与解压引擎（支持 `ca… |
| `cap:gif` | `GifError`, `GifColor`, `DisposalMethod` 等 | 纯 CP GIF87a/GIF89a 静图与多帧动画编… |
| `cap:highway` | `indexOfByte`, `lastIndexOfByte`, `indexOfAnyByte` 等 | 纯 CP `@Vector(32, u8)`/`@Ve… |
| `cap:interchange` | `InterchangeFormat`, `InterchangeError`, `InterchangeKind` 等 | 纯 CP JSON/JSONC/JSON5/TOML/… |
| `cap:unicode` | `UnicodeError`, `isValidUtf8`, `countCodepoints` 等 | 纯 CP RFC 3629 UTF-8/UTF-16… |
| `cap:histogram` | `HistogramError`, `HdrHistogram`, `merge` 等 | 纯 CP HDR 高动态范围延迟直方图、分位统计、EW… |
| `cap:aio` | `ParentDeathWatchdog`, `AioOpKind`, `AioState` 等 | Linux `PR_SET_PDEATHSIG` 父进… |
| `cap:meta` | `isPowerOfTwo`, `nextPowerOfTwo`, `alignUp` 等 | 编译期反射特征探测（适配 Zig 0.17 `@typ… |
| `cap:platform` | `OsKind`, `ArchKind`, `Endianness` 等 | 宿主操作系统、Linux `uname` 真实内核版本… |
| `cap:pkg`（`cap.pkg`/`cap.install`） | `SemverString`, `StringBuf`, `DependencyVersion` 等 | 工业级包管理全栈引擎（直迁 Bun `install/… |
| `cap:oauth2` | `OAuth2Error`, `PkceMethod`, `PkcePair` 等 | 纯 CP RFC 6749/7636 OAuth 2.… |
| `cap:ulid` | `UlidError`, `Ulid`, `fromParts` 等 | 纯 CP Crockford Base32 128-b… |
| `cap:watcher` | `WatchAction`, `WatchEvent`, `WatchFilter` 等 | 纯 CP Linux inotify 文件监控、事件防… |
| `cap:paths` | `PathError`, `PathStyle`, `ParsedPath` 等 | 纯 CP POSIX/Windows 双风格路径解析… |
| `cap:perf` | `PerfClock`, `PerfTimer`, `PerfPhase` 等 | 纯 CP 高分辨率作用域计时、P95/P99 热点聚合… |
| `cap:collections` | `CollectionError`, `BitSet`, `BoundedDeque` 等 | 纯 CP 高性能 BitSet、无堆 BoundedD… |
| `cap:csrf` | `CsrfError`, `CsrfConfig`, `CsrfProtector` 等 | 纯 CP HMAC-SHA256 CSRF 令牌、抗… |
| `cap:wyhash` | `WyhashStateless`, `WyhashStream`, `Wyhash` 等 | 纯 CP Wyhash v4 高速 64-bit 非加… |
| `cap:analytics` | `AnalyticsError`, `DropPolicy`, `PropKind` 等 | 纯 CP 有界环形缓冲遥测收集、背压策略、敏感字段自动… |
| `cap:threading` | `ResetEvent`, `WaitGroup`, `CountDownLatch` 等 | 纯 CP Futex 驱动零堆并发同步原语与泛型互斥保… |
| `cap:base64` | `Base64Variant`, `Base64Error`, `encodedLen` 等 | 纯 CP RFC 4648 Base64/Base64… |
| `cap:libarchive` | `ArchiveFormat`, `ArchiveError`, `ArchiveEntryKind` 等 | 纯 CP 归档魔数探测、Unix Ar、SVR4 CP… |
| `cap:zlib` | `ZlibFormat`, `ZlibLevel`, `ZlibOptions` 等 | 纯 CP RFC 1950 Zlib、RFC 1952… |
| `cap:cose` | `CoseError`, `CoseAlgorithm`, `CoseKeyType` 等 | 纯 CP RFC 9052/9053 COSE_Sig… |
| `cap:webauthn` | `WebAuthnError`, `AuthFlags`, `AttestedCredData` 等 | 纯 CP FIDO2/WebAuthn Passkey… |
| `cap:parquet` | `ParquetError`, `ParquetType`, `ParquetCompression` 等 | 纯 CP Apache Parquet 列式容器读写… |
| `cap:mcp` | `McpError`, `McpCapabilities`, `McpToolDef` 等 | 纯 CP Model Context Protocol… |
| `cap:a2a` | `A2aError`, `TaskState`, `MessageRole` 等 | 纯 CP Agent-to-Agent (A2A) 多… |
| `cap:gossip` | `MemberState`, `MemberInfo`, `GossipUpdate` 等 | 纯 CP Sans-IO SWIM 分布式集群成员发现… |
| `cap:pgwire` | `PgError`, `PgTxStatus`, `PgAuthKind` 等 | 纯 CP PostgreSQL v3.0 Wire P… |
| `cap:quant` | `QuantError`, `ScalarQuantParams`, `Int8Quantized` 等 | 纯 CP AI Embedding 向量量化（Scal… |
| `cap:safetensors` | `SafetensorsError`, `Dtype`, `dtypeSize` 等 | 纯 CP HuggingFace Safetensor… |
| `cap:gguf` | `GGUF_MAGIC`, `GgufError`, `GgufValueType` 等 | 纯 CP GGUF v2/v3 llama.cpp 本… |
| `cap:tensor` | `TensorError`, `Tensor`, `add` 等 | 纯 CP Strided 多维张量视图、广播代数、矩阵… |
| `cap:attention` | `AttnError`, `scaledDotProductAttention`, `groupedQueryAttention` 等 | 纯 CP Transformer SDPA/GQA、无… |
| `cap:onnx` | `OnnxError`, `PbWireReader`, `PbWireWriter` 等 | 纯 CP ONNX ModelProto/GraphP… |
| `cap:dwarf` | `DwarfError`, `DwTag`, `DwAt` 等 | 纯 CP DWARF v4/v5 `.debug_li… |
| `cap:cel` | `CelError`, `CelMapEntry`, `CelValue` 等 | 纯 CP Common Expression Lang… |
| `cap:jq` | `JqError`, `JqTag`, `JqObjEntry` 等 | 纯 CP `jq` JSON 结构化查询、多结果流展开… |
| `cap:otlp` | `OtlpError`, `TraceParent`, `BaggageEntry` 等 | 纯 CP OpenTelemetry OTLP (Tr… |
| `cap:jmespath` | `JmesError`, `JmesValue`, `parseJson` 等 | 纯 CP JMESPath JSON 查询、切片/通配… |
| `cap:capnp` | `CapnpError`, `PointerKind`, `ElementSize` 等 | 纯 CP CapnProto 零拷贝分段消息帧、Str… |
| `cap:xxhash` | `Xxh128`, `xxh32`, `xxh64` 等 | 纯 CP 零堆 xxHash (XXH32/XXH64… |
| `cap:exif` | Transform, Orientation, parseTiff, readJpeg | 纯 CP 零依赖 JPEG APP1/Exif 与大小… |
| `cap:autolink` | AutolinkKind, AutolinkSpan, ScanUrlResult, isListBullet, isListItemMark, isInSet | 纯 CP 零依赖 GFM 宽松 URL/WWW/Ema… |
| `cap:sri` | MAX_DIGEST_LEN, MAX_SRI_ENTRIES, SriError, IntegrityTag, Integrity, IntegrityLis | 零堆分配 W3C SRI §3.3.3 (sha1/s… |
| `cap:thumbhash` | MAX_LEN, ThumbhashError, DecodedImage, encode, decode, approxAspectRatio, averag | Evan Wallace ThumbHash 21–2… |
| `cap:etag` | EntityTag, ParsedETag, ETagListIter, PreconditionResult, isValidEtagChar, isVali | RFC 9110 §8.8.3/§13.1/§13.2… |
| `cap:editorconfig` | matchSectionPattern, IndentStyle, EndOfLine, Charset, EditorConfigProps, resolve | .editorconfig 零分配解析器、花括号/通配… |
| `cap:logfmt` | LogfmtField, LogfmtIter, LogfmtEncoder, getLogfmtValue, getLogfmtInt, getLogfmtB | 结构化 logfmt（key=value）零分配流式解… |
| `cap:slugify` | SlugOptions, deunicodeChar, deunicode, slugifyWithOptions, slugifyWithSep, slugi | 零分配 URL 安全 Slug 生成器与 Latin-… |
| `cap:frontmatter` | FrontmatterKind, FrontmatterSplit, splitFrontmatter, getFrontmatterField | Markdown 文档 YAML (---) / TO… |
| `cap:jsonc` | `JsoncOptions`, `stripJsoncWithOptions`, `stripJsoncWithSpaces` 等 | JSONC 零分配注释/尾逗号剥离器、原地等宽空格替换… |
| `cap:ansi` | `Osc8Link`, `formatOsc8`, `parseOsc8` 等 | ANSI 终端控制序列统一主包（OSC 8 超链接、S… |
| `cap:docker` | `DockerImageRef`, `DockerfileIter` | Docker 镜像引用与 Dockerfile 解析统… |
| `cap:mail` | `Mailbox`, `decodeRfc2047`, `decodeQuotedPrintable` | RFC 5322 邮件地址、RFC 2047 编码字与… |
| `cap:pkg` | `NpmPackageName`, `NpmrcIter`, `PkgExports` 等 | 包管理/NPM 名称/npmrc/exports/ma… |
| `cap:jwt` | `JwtClaims`, `parseJwtClaims` | JWT 声明解析统一主包（同步收敛 `cap:ssh`… |
| `cap:rate` | `Quota`, `RateDecision`, `GcraLimiter` 等 | 工业级 GCRA(`Quota`/`checkAt`/… |
| `cap:pg` | `PgAuth`, `PgCopySasl`, `PgExtendedQuery` 等 | PostgreSQL 线协议全套认证、SASL/COP… |
| `cap:mysql` | `AuthError`, `CapsCapabilities`, `ControlError` 等 | MySQL 线协议全套认证、能力位、握手、预处理语句生… |
| `cap:markdown` | `LineAnalysis`, `MarkdownLink`, `RefDef` | Markdown 行分析、链接与引用定义统一主包（同步… |
| `cap:js` | `JsIdent`, `JsOp`, `UseDirectiveBoundering` 等 | JS 标识符、运算符、外部切片零堆解构绑定 AST 与… |
| `cap:json5` | `Json5TokenKind`, `Json5Token`, `Json5Iter` 等 | 零分配 JSON5 1.0 词法扫描、语法校验与标准… |
| `cap:qpack` | QPACK_NO_INDEX, QpackHeader, QpackClassKind, QpackClass, IntDecodeResult, classi | 吸收自 Bun uws_sys/quic/Header… |
| `cap:h2stream` | H2_MAX_WINDOW_SIZE, H2_DEFAULT_INITIAL_WINDOW, H2_RST_STREAM_FRAME_LEN, H2_WINDO | 吸收自 Bun http/h2_client/Stre… |
| `cap:textstream` | UTF8_REPLACEMENT, isLeadSurrogate, isTrailSurrogate, combineSurrogatePair, encod | 吸收自 Bun runtime/webcore/Tex… |
| `cap:mediaquery` | MediaType, Qualifier, MediaComparison, ColorScheme, ReducedMotion, Orientation,  | 吸收自 Bun css/media_query.cp… |
| `cap:cssmodules` | CssModuleConfig, PatternKind, PatternSegment, Pattern, isValidCssIdent, extractF | 吸收自 Bun css/css_modules.cp… |
| `cap:textdecode` | TextEncoding, DecodeResult, sniffBom, TextDecoder | 吸收自 Bun runtime/webcore/Tex… |
| `cap:bloburl` | BLOB_PREFIX, UUID_STR_LEN, BLOB_URL_MIN_LEN, BlobUuid, ParsedBlobUrl, BlobUrlEnt | 吸收自 Bun runtime/webcore/Obj… |
| `cap:catalog` | CatalogRef, isCatalogProtocol, isValidCatalogName, parseCatalogRef, formatCatalo | 吸收自 Bun install/lockfile/Ca… |
| `cap:byterange` | ByteRange, RangeStatus, RangeResult, RawRangeKind, RawRange, MultiRangeResult, P | 吸收自 Bun runtime/server/Rang… |
| `cap:keyframes` | VendorPrefix, KeyframesNameKind, KeyframesName, SelectorKind, KeyframeSelector,  | 吸收自 Bun css/rules/keyframes… |
| `cap:agent` | `BudgetStatus`, `ModelPricing`, `BudgetLimits` 等 | 纯 CP 零分配 AI Agent 统一运行时聚合主包… |
| `cap:doc` | `PdfDocument`, `PdfPage`, `PageSize` 等 | 统一文档/办公/出版/日程/联系人/订阅与零堆 RFC… |
| `cap:ml` | `svm.cp`, `knn.cp`, `dtree.cp`, `logreg.cp`, `naivebayes.cp`, `lasso.cp`, `isoto | 统一经典机器学习、聚类、图排序、统计优化、稠密矩阵、语… |
| `cap:media` | `AudioError`, `WavAudio`, `WavFloatAudio` 等 | 统一音视频、容器、字幕、音色库（DLS/SF2）、芯片… |
| `cap:ds` | `MatchKind`, `AhoCorasickOptions`, `AcError` 等 | 统一通用数据结构、空间索引、O(1) SparseSe… |
| `cap:binfmt` | `validateMachoStrict`, `validateCoffStrict`, `validatePcapStrict` 等 | 统一二进制/可执行/列存/序列化/科学数据/剖析文件格… |
| `cap:auth` | `PemBundleIter`, `encodePemStrict`, `decodePemInto` 等 | 统一认证、授权、RBAC/Rego 策略引擎、企业认证… |
| `cap:db` | `serializePortable`, `deserializePortable`, `select` 等 | 统一存储引擎、无上限外部切片全栈 LSM-Tree（C… |
| `cap:web` | `App`, `Request`, `RequestArena` 等 | 统一 Web/API/RPC/HTTP/WS 原生多核… |
| `cap:compiler` | `verifySatModel`, `PrefilterKind`, `Prefilter` 等 | 统一语言前端、表达式/SAT 求解、Anti-ReDo… |
| `cap:devops` | `deb.cp`, `rpm.cp`, `apk.cp`, `dmg.cp`, `iso.cp`, `udf.cp`, `vdi.cp`, `vhd.cp`,  | 统一软件包分发、磁盘镜像、闪存/只读文件系统、引导、灰… |
| `cap:gfx` | `TtcHeader`, `TtfHheaMetrics`, `TtfHorizontalMetrics` 等 | 统一图形渲染、字体排版、GPU 纹理、计算几何/碰撞… |
| `cap:iot` | `can.cp`, `modbus.cp`, `dnp3.cp`, `s7.cp`, `knx.cp`, `ihex.cp`, `srec.cp`, `uf2. | 统一工业自动化、车联网、微控制器固件、零配置设备发现与… |
| `cap:science` | `fasta.cp`, `fastq.cp`, `sam.cp`, `vcf.cp`, `gff.cp`, `bed.cp`, `pdb.cp`, `cif.c | 统一生物信息/基因组学、计算化学/晶体结构、生物医学信… |
| `cap:finance` | `billing.cp`, `money.cp`, `rates.cp`, `ledger.cp`, `formula.cp`, `sheet.cp`, `au | 统一金融计费、多币种外汇、复式记账总账、电子表格公式… |
| `cap:obs` | `LatencyBucketHistogram`, `OtlpSpanRecord`, `OtlpSpanBatch` 等 | 统一可观测性指标、SIEM 与访问日志、CI/静态分析… |

## 三、 官方 Web 插件生态 (`@web/*`)

共 **34 个**（位于 `packages/@web/*`，`cap add @web/<name>`）：`官方插件包` · ``@web/cors`` · ``@web/security`` · ``@web/jwt`` · ``@web/ratelimit`` · ``@web/logger`` · ``@web/static`` · ``@web/swagger`` · ``@web/multipart`` · ``@web/compress`` · ``@web/cache`` · ``@web/sensible`` · ``@web/pressure`` · ``@web/basic-auth`` · ``@web/sse`` · ``@web/session`` · ``@web/proxy`` · ``@web/cron`` · ``@web/env`` · ``@web/i18n`` · ``@web/oauth2`` · ``@web/graphql`` · ``@web/mcp`` · ``@web/redis`` · ``@web/db`` · ``@web/s3`` · ``@web/rbac`` · ``@web/flags`` · ``@web/webhook`` · ``@web/circuit-breaker`` · ``@web/audit`` · ``@web/html`` · ``@web/trpc`` · ``@web/a2a``

## 四、 官方 `@cap/*` 生态包分类索引（共 704 包）

> 导入方式：`import { ... } from "@cap/<name>";`（精确导出查 `packages/@cap/<name>/src/index.d.cp`；括号 `←原名` 为已改名映射）。

### 密码学 / 哈希 / 安全 (92)
`accept-header` · `adler32`(←adler2) · `ajv-schema`(←ajv) · `base32-crockford`(←crockford-base32) · `base32-rfc`(←hi-base32) · `base64-url`(←base64url) · `base64ct` · `blake2-crate`(←blake2) · `bs58` · `byteorder`(←untrusted) · `cache-control`(←cache-control-parser) · `chacha-rng`(←rand_chacha) · `chacha20-crate`(←chacha20) · `chacha20-poly1305`(←chacha20poly1305) · `chacha20poly1305` · `commander` · `const-oid` · `cors-headers`(←cors) · `crc16` · `crc32`(←simd-adler32) · `crc32c` · `crc32fast` · `crc64`(←crc64fast) · `crc8` · `cuid2`(←@paralleldrive/cuid2) · `der` · `der-asn1`(←der) · `deranged` · `digest-auth`(←http-digest-client) · `ecdsa-sig`(←ecdsa) · `escape-html` · `fastrand` · `fletcher` · `fnv`(←rustc-hash) · `fnv-hash`(←@sindresorhus/fnv1a) · `heck-case`(←heck) · `helmet-headers`(←helmet) · `hex` · `hex-color`(←hex-rgb) · `hex-dump`(←hexdump-nodejs) · `hkdf-crate`(←hkdf) · `hkdf-sha256`(←futoin-hkdf) · `hmac-crate`(←hmac) · `json-schema`(←ajv) · `jwt-parse`(←jwt-decode) · `ksuid` · `link-header`(←parse-link-header) · `md5-crate`(←md-5) · `murmur3` · `mustache-tpl`(←mustache) · `nanoid` · `oid`(←const-oid) · `on-finished` · `ordered-float` · `pbkdf2-crate`(←pbkdf2) · `pbkdf2-sha256`(←pbkdf2) · `pem`(←rustls-pemfile) · `pem-rfc7468` · `rand-distr`(←rand_distr) · `rate-header`(←governor) · `ripemd160`(←ripemd) · `rustc-hash` · `rustls-pemfile` · `seahash` · `sha1-crate`(←sha1) · `sha2-crate`(←sha2) · `shebang-command` · `shebang-regex` · `shell-escape` · `shell-quote` · `short-uuid` · `sieve-cache` · `siphash`(←siphasher) · `siphash-pure`(←siphash) · `siphasher` · `snowflake`(←@sapphire/snowflake) · `snowflake-id`(←snowflake) · `spdx` · `spdx-expression-parse` · `subtle`(←constant_time_eq) · `subtle-ct`(←subtle) · `tdigest` · `tiny-keccak` · `toidentifier` · `totp-uri`(←totp-rs) · `twox-hash` · `ulid` · `unicode-xid` · `uuid-v7`(←uuidv7) · `zbase32` · `zeroize` · `zod-schema`(←zod)

### 压缩 / 归档 / 编解码 (38)
`astar` · `base16ct` · `base36` · `base62` · `base85`(←z85) · `bincode` · `bitstream`(←bitstream-io) · `bitvec` · `bytemuck-crate`(←bytemuck) · `bytes` · `bytes-buf`(←bytes) · `data-encoding` · `encodeurl` · `flate2-core`(←flate2) · `form-urlencoded`(←querystringify) · `leb128` · `lz4-block`(←lz4_flex) · `lzw`(←weezl) · `magic-bytes`(←file-type) · `nom-bytes`(←winnow) · `percent-encoding` · `pretty-bytes` · `prost-wire`(←google-protobuf) · `punycode` · `quinn-varint`(←quinn-proto) · `rle`(←rle-decode-fast) · `simdutf8` · `snappy-block`(←snap) · `tar-core`(←tar) · `target-triple`(←target-lexicon) · `trunc-utf8`(←cli-truncate) · `utf16-codec`(←utf16) · `utf8-path`(←camino) · `utf8-valid`(←utf8parse) · `varint`(←unsigned-varint) · `zerocopy-crate`(←zerocopy) · `zip-core`(←zip) · `zstd`

### 网络 / HTTP / URL / DNS / 协议 (95)
`accept-lang`(←accept-language-parser) · `accepts` · `amqp-url`(←amqplib) · `ansi-strip`(←strip-ansi) · `at-uri`(←@atproto/syntax) · `bimap` · `cidr-tools` · `content-disposition` · `content-type` · `cookie-crate`(←cookie) · `cookie-jar`(←tough-cookie) · `cookie-parser` · `cookie-sign`(←cookie-signature) · `cookie-signature` · `cors-match`(←tower-http) · `data-uri`(←data-uri-to-buffer) · `dns-packet` · `etag` · `finalhandler` · `formidable` · `forwarded` · `fresh` · `geo-uri` · `git-url`(←git-url-parse) · `graphql-parse`(←graphql) · `grpc-frame`(←tonic) · `hosted-git-info` · `http-date`(←httpdate) · `http-errors` · `http-status`(←http-status-codes) · `http-types`(←http) · `http2-hpack`(←h2) · `indent-strip`(←dedent) · `ip-range`(←ipnet) · `ipaddr`(←ipaddr.js) · `ipnet` · `macaddr` · `magnet-uri`(←magnet-url) · `mailto-uri`(←mailto-link) · `markdown-strip`(←remove-markdown) · `media-type`(←mediatype) · `media-typer` · `mediatype` · `merge-descriptors` · `mime-guess`(←mime_guess) · `mime-sniff`(←istextorbinary) · `mime-types` · `mongo-uri`(←mongodb-connection-string-url) · `mqtt-packet`(←mqttbytes) · `multer` · `multimap` · `multipart` · `multipart-boundary`(←busboy) · `mysql-dsn`(←mysql2) · `negotiator` · `oci-ref`(←oci-distribution) · `parseurl` · `pg-dsn`(←pg-connection-string) · `proxy-addr` · `proxy-proto`(←proxy-protocol-js) · `publicsuffix` · `purl`(←packageurl-js) · `qs` · `qs-lite`(←qs) · `querystring` · `range-parser` · `raw-body` · `redis-url`(←redis) · `robots`(←robots-parser) · `rss` · `rustls-pki-types` · `rustls-webpki` · `s3-uri`(←amazon-s3-uri) · `send` · `serde-qs`(←serde_qs) · `serve-static` · `sitemap` · `skiplist` · `sql-escape`(←sqlstring) · `sqlformat`(←sql-formatter) · `sse-parse`(←eventsource-parser) · `ssh-key` · `statuses` · `strip-bom` · `strip-final-newline` · `strip-indent` · `strip-json-comments` · `tel-uri`(←phonenumber) · `uri-template`(←url-template) · `url-join` · `user-agent-hints`(←sfv) · `vary` · `webhook-sig`(←standardwebhooks) · `whatwg-url`(←url) · `ws-frame`(←ws)

### 路径 / 文件系统 / Glob / 包管理 (49)
`anymatch` · `braces` · `camino` · `chokidar-core`(←chokidar) · `dunce` · `execa` · `fast-glob` · `fill-range` · `find-up` · `glob-match` · `glob-parent` · `glob-to-regex`(←glob-to-regexp) · `globset` · `ignore` · `is-binary-path` · `is-extglob` · `is-glob` · `locate-path` · `make-dir` · `micromatch` · `minimatch` · `normalize-package-data` · `normalize-path` · `normpath` · `npm-run-path` · `npm-spec`(←npm-package-arg) · `path-exists` · `path-key` · `path-to-regexp` · `pathdiff` · `picomatch` · `pkg-dir` · `read-pkg` · `read-pkg-up` · `relative-path` · `resolve-from` · `rimraf-core`(←rimraf) · `semver-crate`(←semver) · `semver-diff` · `semver-range`(←semver) · `shlex`(←shell-quote) · `svg-path`(←parse-svg-path) · `template`(←tinytemplate) · `to-regex-range` · `tsconfig-paths` · `validate-npm-package-license` · `validate-npm-package-name` · `which-bin`(←which) · `wildmatch`

### CLI / 终端 / ANSI / 日志 (46)
`aho-corasick` · `ansi-escapes` · `ansi-regex` · `ansi-styles` · `block-padding` · `boxen` · `cargo-platform` · `chalk` · `clf-log`(←clf-parser) · `cli-spinners` · `cli-truncate` · `color-convert` · `color-hsl`(←palette) · `color-name` · `color-names`(←color-name) · `color-space`(←color-convert) · `colorette` · `css-color`(←color-parse) · `debug` · `enquirer` · `hyperloglog` · `indent-string` · `indicatif` · `is-unicode-supported` · `keep-changelog`(←keep-a-changelog) · `kleur` · `listr2` · `logfmt` · `morgan-log`(←morgan) · `ora-spinner`(←ora) · `pad-str`(←pad) · `picocolors` · `pino-log`(←pino) · `pretty-format` · `pretty-ms` · `proc-args`(←mri) · `quoted-printable` · `slice-ansi` · `string-width` · `supports-color` · `syslog` · `table`(←cli-table3) · `textwrap`(←wrap-ansi) · `wrap-ansi` · `yargs` · `yargs-parser`

### 字符串 / 文本 / 正则 / 命名转换 (58)
`atoi` · `bstr` · `bytecount` · `camelcase`(←ident_case) · `camelcase-keys` · `change-case` · `convert-case`(←convert_case) · `css-select` · `css-units`(←postcss-value-parser) · `decamelize` · `decamelize-keys` · `destroy` · `diff` · `dijkstra`(←pathfinding) · `ejs-tpl`(←ejs) · `escape-string-regexp` · `handlebars-tpl`(←handlebars) · `html-entities`(←he) · `html-escape` · `icu-normalizer`(←icu_normalizer) · `iri-string` · `itoa` · `jaro-winkler` · `jest-diff` · `kebabcase-keys` · `lcs-diff`(←diff-sequences) · `levenshtein`(←fastest-levenshtein) · `markdown-toc`(←github-slugger) · `memchr` · `metaphone` · `natsort`(←natural-compare) · `netstring` · `nom-parse`(←nom) · `pluralize` · `postcss-parse`(←postcss) · `regex-escape` · `regex-lite` · `regex-syntax` · `ryu-float`(←ryu) · `segment-tree` · `similar-diff`(←similar) · `slugify` · `smol-str`(←smol_str) · `snakecase-keys` · `strftime` · `stringprep` · `strsim` · `strum-enum`(←strum) · `tinystr` · `title-case` · `trim-newlines` · `unicase` · `unicode-bidi` · `unicode-ident` · `unicode-properties` · `unicode-seg`(←unicode-segmentation) · `unicode-segmentation` · `unicode-width`

### 集合 / 数据结构 / 对象工具 / 数学 (89)
`approx` · `arrayvec` · `bech32` · `bigdecimal` · `bigint-u256`(←bn.js) · `bitflags` · `bitset`(←fixedbitset) · `bloom`(←fastbloom) · `bloom-filter`(←bloom-filters) · `bumpalo` · `complex-num`(←complex.js) · `cuckoo-filter`(←cuckoofilter) · `decimal`(←decimal.js) · `decimal-fixed`(←rust_decimal) · `dot-prop`(←lodash) · `enumflags2` · `fast-deep-equal` · `fft`(←rustfft) · `fft-radix2`(←fft.js) · `fixed-point`(←fixed) · `fixed-q16`(←fixed) · `fixedbitset` · `float-cmp` · `geo-dist`(←geo) · `geohash` · `geohash-pure`(←ngeohash) · `geojson` · `glam` · `h3o` · `half` · `half-f16`(←half) · `haversine-geo`(←haversine) · `indexmap` · `is-plain-obj` · `kdtree-2d`(←kdbush) · `lru`(←lru-cache) · `lru-slab` · `map-obj` · `mat2`(←glam) · `mat3`(←gl-mat3) · `mat4`(←gl-mat4) · `matchit` · `math-interp`(←lerp) · `mathjs-expr`(←mathjs) · `mint` · `nalgebra` · `ndarray` · `num-complex` · `num-conv` · `num-integer` · `num-iter` · `num-rational` · `num-traits` · `online-stats`(←average) · `p-queue` · `petgraph-core`(←petgraph) · `polyline` · `priority-queue` · `quadtree-2d`(←quadtree_rs) · `quat`(←quaternion) · `quick-lru` · `radix-sort`(←radsort) · `rational`(←fraction.js) · `ray-cast`(←parry3d) · `ring-queue`(←crossbeam-queue) · `ringbuf` · `roaring` · `roaring-bitmap`(←roaring) · `s2` · `slab`(←sharded-slab) · `slab-alloc`(←slab) · `slotmap` · `smallvec` · `stacktrace`(←error-stack-parser) · `statrs` · `stats-basic`(←simple-statistics) · `statsd`(←hot-shots) · `suffix-array`(←suffix) · `text-stats`(←reading-time) · `thin-vec` · `tinyvec` · `toposort`(←topological-sort) · `trie`(←radix_trie) · `ucd-trie` · `vec2`(←victor) · `vec3`(←gl-matrix) · `wkt` · `yocto-queue` · `zerotrie`

### 异步 / 并发 / 限流 / 事件 / 重试 (19)
`backoff` · `barcode`(←jsbarcode) · `block-buffer` · `bmh-search`(←boyer-moore-magiclen) · `debounce`(←lodash.debounce) · `ee-first` · `governor` · `kmp-search`(←kmp) · `ldap-dn`(←ldapjs) · `minisearch`(←lunr) · `mitt` · `p-limit` · `p-locate` · `p-map` · `p-timeout-core`(←p-timeout) · `p-try` · `retry`(←async-retry) · `retry-after` · `retry-backoff`(←p-retry)

### 配置 / 序列化 / Schema / 校验 / 时间 (35)
`cosmiconfig` · `cron-desc`(←cronstrue) · `cron-expr`(←cron) · `cron-next`(←cron) · `cron-parser` · `csv-core` · `csv-line`(←csv-parse) · `disjoint-set`(←disjoint-sets) · `dotenv-expand` · `duration-iso`(←iso8601-duration) · `envsubst`(←dotenv-expand) · `humantime` · `ini` · `is-number` · `iso8601` · `json-pointer`(←jsonpointer) · `json-repair`(←jsonrepair) · `json-rpc`(←jsonrpc-core) · `jsonflat`(←flat) · `lilconfig` · `miniz`(←miniz_oxide) · `ms` · `ndjson` · `onetime` · `parse-duration`(←parse_duration) · `parse-ms` · `rfc3339` · `terser-minify`(←terser) · `timeago`(←timeago.js) · `toml-edit`(←toml_edit) · `toml-writer`(←toml_writer) · `type-is` · `tzfile` · `validator` · `yaml`(←js-yaml)

### 原生 C/C++ 高性能引擎绑定 (39)
`cmark` · `curl` · `dav1d` · `dr_libs` · `duckdb` · `expat`(←libexpat) · `flac` · `freetype` · `harfbuzz` · `libavif` · `libsrtp` · `libvpx` · `libyuv` · `llama`(←llama.cpp) · `lmdb` · `lz4` · `mbedtls` · `miniaudio` · `ogg` · `onnx`(←onnxruntime) · `openblas` · `opencv` · `opus` · `pdfium` · `picohttp`(←picohttpparser) · `rnnoise` · `sdl3`(←SDL) · `snappy` · `sodium`(←libsodium) · `speexdsp` · `stb` · `treesitter`(←tree-sitter) · `usearch` · `usrsctp` · `utf8proc` · `vorbis` · `whisper`(←whisper.cpp) · `xxhash` · `yyjson`

### 其他通用工具包 (144)
`aabb2`(←parry2d) · `aabb3`(←parry3d) · `acorn-parse`(←acorn) · `ahash` · `anstyle` · `anstyle-parse` · `arn-parse`(←@aws-sdk/util-arn-parser) · `ascii-tree`(←archy) · `ascii85` · `basic-auth` · `bcp47`(←bcp-47) · `bearer-auth`(←express-bearer-token) · `bencode`(←bendy) · `bezier-curve`(←bezier-easing) · `binary-extensions` · `borsh` · `bspline`(←b-spline) · `bvh` · `bwt` · `calver` · `card-bin`(←credit-card-type) · `cexpr` · `cliui` · `clsx` · `cobs` · `consistent-hash`(←hashring) · `conventional-commits`(←conventional-commits-parser) · `convex-hull`(←monotone-chain-convex-hull) · `count-min`(←count-min-sketch) · `csp`(←content-security-policy-builder) · `depd` · `detect-newline` · `dice-coeff`(←string-similarity) · `did-parse`(←did-resolver) · `didyoumean` · `docker-ref`(←oci-distribution) · `easing`(←ezing) · `escalade` · `expr`(←expr-eval) · `faker`(←@faker-js/faker) · `fenwick` · `foldhash` · `ghash-gf128`(←universal-hash) · `hamming` · `has-flag` · `hashbrown` · `hdrhistogram` · `home-dir`(←home) · `hosts-file`(←hostile) · `huffman`(←huffman-compress) · `human-number`(←numeral) · `iban`(←iban_validate) · `ical`(←ical-generator) · `icu-locid`(←icu_locid) · `idempotency`(←idempotency-key) · `idna-uts46`(←idna) · `indoc` · `isbn` · `itertools` · `jcs`(←fast-json-stable-stringify) · `kafka-topic`(←kafkajs) · `kalman`(←kalman-filters) · `kalman1d`(←kalmanjs) · `kd-tree`(←kiddo) · `kind-of` · `libm` · `line-col`(←line-column) · `litemap` · `luhn` · `mail-addr`(←addressparser) · `marked-md`(←marked) · `memmem` · `merkle`(←rs_merkle) · `methods` · `morse-code`(←morse-decoder) · `morton`(←morton-encoding) · `natural-cmp`(←natord) · `noise` · `otel-baggage`(←@opentelemetry/core) · `parse5` · `pcg`(←rand_pcg) · `pcg32`(←pcg-random) · `pcre2`(←regex) · `perlin-noise` · `petname`(←unique-names-generator) · `phf-hash`(←phf_generator) · `phone`(←libphonenumber-js) · `pid` · `pid-ctrl`(←node-pid-controller) · `pkcs1` · `pkcs8` · `poly1305` · `polygon2d`(←point-in-polygon) · `porter-stem`(←stemmer) · `powerfmt` · `prettier-fmt`(←prettier) · `prom-text`(←prom-client) · `properties`(←properties-reader) · `ramer-douglas-peucker`(←simplify-js) · `range-set`(←rangemap) · `redact`(←fast-redact) · `redent` · `reed-solomon`(←reed-solomon-erasure) · `rehype-ast`(←rehype) · `resp` · `rfc6979` · `roman-num`(←romans) · `rot13`(←rot13-cipher) · `rrule-parse`(←rrule) · `running-avg`(←moving-averages) · `sec1` · `server-timing` · `shortid` · `slash` · `socks5-frame`(←fast-socks5) · `soundex`(←soundex-code) · `sparkline`(←sparkly) · `spiffe-id`(←spiffe) · `spki` · `spline`(←splines) · `sqids` · `ssri` · `stomp` · `subtitle` · `svg-badge`(←badge-maker) · `syntax-highlight`(←highlight.js) · `traceparent`(←opentelemetry) · `unflatten` · `urn-parse`(←urn-lib) · `useragent`(←ua-parser-js) · `uuencode` · `vcard` · `vlq`(←@jridgewell/sourcemap-codec) · `wcag`(←wcag-contrast) · `webpki-roots` · `widest-line` · `word-count`(←words-count) · `wyhash` · `x509-cert` · `xor-filter`(←xorf) · `xorshift`(←rand_xorshift) · `xoshiro256`(←rand_xoshiro) · `y18n` · `zigzag`
