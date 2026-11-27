# CP 智能体标准库与生态极简速查手册 (`cp_agent_lib_ref.md`)

> **更新日期**：2026-11-28 | **自动生成**：`python3 scripts/gen_agent_lib_ref.py`
> **覆盖规模**：内置标准库 **209 通道**（`cap:*`）+ 官方 Web 插件 **34 包**（`@web/*`）+ 官方纯 CP/原生包 **873 包**（`@cap/*`）+ 上游映射 **1262 条**（npm 676 / crates.io 586）。
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
| `cap:ascii` | `isAscii`, `eqIgnoreCase`, `trim` 等 | 零堆 ASCII 校验/分类/大小写/裁剪与字节查找… |
| `cap:bin` | `bytesEq`, `ctEqual`, `findByte` 等 | 零堆 16B 展开二进制扫描、常量时间比较与大小端读写… |
| `cap:fmt` | `BufWriter` 等 | 零堆缓冲格式化写入器与严格整数解析 L0 基座 |
| `cap:utf8` | `isValidUtf8`, `decodeCodepoint`, `encodeCodepoint` 等 | 严格 RFC 3629 UTF-8 校验与码点编解码… |
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
| `cap:cdp` | `CdpError`, `CdpMessageKind`, `CdpRequest` 等 | 零拷贝原生 Chrome DevTools Proto… |

## 三、 官方 Web 插件生态 (`@web/*`)

共 **34 个**（位于 `packages/@web/*`，`cap add @web/<name>`）：`官方插件包` · ``@web/cors`` · ``@web/security`` · ``@web/jwt`` · ``@web/ratelimit`` · ``@web/logger`` · ``@web/static`` · ``@web/swagger`` · ``@web/multipart`` · ``@web/compress`` · ``@web/cache`` · ``@web/sensible`` · ``@web/pressure`` · ``@web/basic-auth`` · ``@web/sse`` · ``@web/session`` · ``@web/proxy`` · ``@web/cron`` · ``@web/env`` · ``@web/i18n`` · ``@web/oauth2`` · ``@web/graphql`` · ``@web/mcp`` · ``@web/redis`` · ``@web/db`` · ``@web/s3`` · ``@web/rbac`` · ``@web/flags`` · ``@web/webhook`` · ``@web/circuit-breaker`` · ``@web/audit`` · ``@web/html`` · ``@web/trpc`` · ``@web/a2a``

## 四、 生态包分类索引（裸包名 + 原生 C/C++ `@cap/*`，共 873 包）

> 导入方式：纯 CP 生态包使用原裸包名 `import { ... } from "<name>";`（精确导出查 `cap brief <name>` 或 `packages/<name>/src/index.d.cp`；原生 C/C++ 包使用 `@cap/<name>`；映射注册表见 `packages/registry.json`）。

### 密码学 / 哈希 / 安全 (118)
@paralleldrive/cuid2, @sapphire/snowflake, @sindresorhus/fnv1a, accept-header, adler2, aes, aes-gcm, base64ct, base64url, blake2, bs58, cache-control-parser, chacha20, chacha20poly1305, commander, const-oid, content-security-policy-builder, crc16, crc32c, crc32fast, crc64fast, crc8, crockford-base32, crypto-bigint, d3-shape, der, deranged, ecdsa, ecdsa-sig-formatter, ed25519, escape-html, fastrand, file-entry-cache, fletcher, fnv, futoin-hkdf, he, heck, helmet, hex, hex-rgb, hexdump-nodejs, hi-base32, hkdf, hmac, http-digest-client, http-range-header, json-schema-traverse, jwt-decode, ksuid, lru-cache, micromark-util-sanitize-uri, miniz_oxide, morse-decoder, murmur3, mustache, nanoid, nohash-hasher, nth-check, oid-registry, on-finished, oorandom, ordered-float, ordered-multimap, parse-link-header, password-hash, pathe, pbkdf2, pcg-random, pem, pem-rfc7468, properties-reader, pure-rand, rand_chacha, rand_distr, rand_hc, rand_pcg, rand_xorshift, rand_xoshiro, ripemd, rot13-cipher, rustc-hash, rustls-pemfile, schema-utils, seahash, sha1, sha2, sharded-slab, shebang-command, shebang-regex, shell-escape, shell-quote, shellexpand, short-uuid, sieve-cache, simd-adler32, siphash, siphasher, sketches-ddsketch, snowflake, spdx, spdx-expression-parse, string_decoder, subtle, tdigest, tiny-keccak, toidentifier, totp-rs, twox-hash, type-check, ulid, unicode-xid, universal-hash, uuidv7, wasm-encoder, zbase32, zeroize, zod-to-json-schema

### 压缩 / 归档 / 编解码 (37)
astar, base16ct, base36, base62, bincode, bitstream-io, bitvec, bytemuck, bytes, bytesize, chardet, data-encoding, encodeurl, flate2, google-protobuf, html-encoding-sniffer, iconv-lite, leb128, lz4_flex, morton-encoding, mqttbytes, percent-encoding, postcard, pretty-bytes, punycode, rle-decode-fast, simdutf8, tar, target-lexicon, unsigned-varint, urlencoding, utf16, utf16_iter, utf8parse, zerocopy, zip, zstd

### 网络 / HTTP / URL / DNS / 协议 (100)
@aws-sdk/util-arn-parser, accept-language-parser, accepts, amazon-s3-uri, amqplib, bimap, bowser, browserslist, busboy, cidr-tools, content-disposition, content-type, cookie, cookie-parser, cookie-signature, cors, cssesc, data-uri-to-buffer, dns-packet, dompurify, etag, fast-uri, finalhandler, formidable, forwarded, fresh, geo-uri, git-url-parse, graphql, hosted-git-info, http, http-content-range, http-errors, http-status-codes, httpdate, https-proxy-agent, humanize-url, ipaddr.js, ipnet, macaddr, magnet-url, matrixmultiply, media-typer, mediatype, merge-descriptors, mime-types, mime_guess, minipass, mongodb-connection-string-url, multer, multimap, multipart, mysql2, negotiator, normalize-url, packageurl-js, parseurl, pg-connection-string, prepend-http, proxy-addr, proxy-from-env, proxy-protocol-js, publicsuffix, qs, querystring, querystringify, range-parser, raw-body, redis, robots-parser, rss, rustls-pki-types, rustls-webpki, send, serde_qs, serialize-javascript, serve-static, set-cookie-parser, sitemap, skiplist, sql-formatter, sqlstring, ssh-key, standardwebhooks, statuses, strip-ansi, strip-bom, strip-final-newline, strip-indent, strip-json-comments, tough-cookie, tower-http, ua-parser-js, unicode-script, url, url-join, url-template, vary, whatwg-mimetype, ws

### 路径 / 文件系统 / Glob / 包管理 (61)
anymatch, bcp-47, braces, camino, chokidar, d3-path, did-resolver, dunce, env-paths, execa, fast-glob, fill-range, find-up, glob-match, glob-parent, glob-to-regexp, globset, ignore, is-binary-path, is-extglob, is-glob, is-path-inside, jsonpath-rust, locate-path, make-dir, micromatch, minimatch, module-details-from-path, normalize-package-data, normalize-path, normpath, npm-package-arg, npm-run-path, parse-svg-path, path-clean, path-exists, path-key, path-parse, path-slash, path-to-regexp, pathdiff, pathfinding, picomatch, pkg-dir, pkg-types, read-pkg, read-pkg-up, relative-path, resolve-from, resolve-pkg-maps, rimraf, semver, semver-diff, shlex, tinytemplate, to-regex-range, tsconfig-paths, validate-npm-package-license, validate-npm-package-name, which, wildmatch

### CLI / 终端 / ANSI / 日志 (55)
aho-corasick, ansi-align, ansi-escapes, ansi-regex, ansi-styles, argparse, block-padding, boxen, cargo-platform, chalk, cli-spinners, cli-table3, cli-truncate, cli-width, color-convert, color-name, color-parse, color_quant, colorette, debug, debugid, dedent, enquirer, fast-json-stable-stringify, figures, getopts, humansize, hyperloglog, indent-string, indicatif, is-unicode-supported, keep-a-changelog, kleur, listr2, logfmt, markdown-table, mri, ora, pad, picocolors, pino, pretty-format, pretty-ms, progress, quoted-printable, slice-ansi, string-width, supports-color, syslog, termtree, textwrap, topological-sort, wrap-ansi, yargs, yargs-parser

### 字符串 / 文本 / 正则 / 命名转换 (83)
ajv-formats, atoi, bstr, bytecount, camelcase-keys, change-case, cjs-module-lexer, convert_case, cronstrue, css-select, css-what, cssparser, cssstyle, d3-format, d3-time-format, damerau-levenshtein, decamelize, decamelize-keys, destroy, diff, diff-sequences, ejs, emoji-regex, es-module-lexer, escape-string-regexp, estree-walker, fastest-levenshtein, github-slugger, handlebars, html-escape, humanize-string, icu_normalizer, ident_case, iri-string, is-fullwidth-code-point, itoa, jaro-winkler, jest-diff, kebabcase-keys, lexical-parse-float, lexical-write-float, lz-string, magic-string, memchr, metaphone, netstring, nom, normalize-line-endings, oci-distribution, pluralize, postcss, postcss-value-parser, property-information, regex, regex-escape, regex-lite, regex-syntax, remove-markdown, ryu, segment-tree, slugify, smol_str, snakecase-keys, split2, sprintf-js, strftime, string-similarity, stringprep, strnum, strsim, strum, tinystr, title-case, trim-newlines, unicase, unicode-bidi, unicode-ident, unicode-linebreak, unicode-properties, unicode-segmentation, unicode-width, winnow, xml-name-validator

### 集合 / 数据结构 / 对象工具 / 数学 (82)
approx, arrayvec, balanced-match, bech32, bigdecimal, bitflags, bloom-filters, bumpalo, clean-stack, complex.js, crossbeam-queue, cuckoofilter, d3-array, decimal.js, enumflags2, error-stack-parser, extract-stack, fast-deep-equal, fastbloom, fft.js, fixed, fixedbitset, float-cmp, geo, geohash, geojson, gl-mat3, gl-mat4, gl-matrix, glam, h3o, half, haversine, id-arena, indexmap, is-plain-obj, lodash, lodash.debounce, lru-slab, map-obj, matchit, mathjs, mint, nalgebra, ndarray, ngeohash, nibble_vec, num-complex, num-conv, num-integer, num-iter, num-rational, num-traits, object-hash, object-inspect, p-queue, p-settle, petgraph, polyline, postgres-interval, priority-queue, quadtree_rs, quaternion, quick-lru, radix_trie, ringbuf, roaring, rust_decimal, rustfft, s2, slab, slotmap, smallvec, stack-utils, statrs, thin-vec, tinyvec, ucd-trie, vec_map, wkt, yocto-queue, zerotrie

### 异步 / 并发 / 限流 / 事件 / 重试 (24)
@jridgewell/sourcemap-codec, archy, async-retry, backoff, block-buffer, chrome-trace-event, convert-source-map, ee-first, eventsource-parser, express-rate-limit, governor, jest-docblock, jsbarcode, mitt, p-all, p-limit, p-locate, p-map, p-retry, p-timeout, p-try, retry-after, space-separated-tokens, webpack-sources

### 配置 / 序列化 / Schema / 校验 / 时间 (43)
ajv, constant_time_eq, cosmiconfig, cron, cron-parser, csv-core, csv-parse, d3-time, disjoint-sets, dotenv-expand, env_filter, envsubst, humantime, iban_validate, ini, is-number, is-potential-custom-element-name, iso8601, iso8601-duration, js-yaml, jsonpointer, jsonrepair, jsonrpc-core, lilconfig, ms, ndjson, onetime, parse-json, parse-ms, parse_duration, postgres-date, reading-time, resolv-conf, rfc3339, ron, timeago.js, toml_edit, toml_writer, type-is, tzfile, validator, zod, zod-validation-error

### 原生 C/C++ 高性能引擎绑定 (39)
@cap/cmark, @cap/curl, @cap/dav1d, @cap/dr_libs, @cap/duckdb, @cap/expat, @cap/flac, @cap/freetype, @cap/harfbuzz, @cap/libavif, @cap/libsrtp, @cap/libvpx, @cap/libyuv, @cap/llama, @cap/lmdb, @cap/lz4, @cap/mbedtls, @cap/miniaudio, @cap/ogg, @cap/onnx, @cap/openblas, @cap/opencv, @cap/opus, @cap/pdfium, @cap/picohttp, @cap/rnnoise, @cap/sdl3, @cap/snappy, @cap/sodium, @cap/speexdsp, @cap/stb, @cap/treesitter, @cap/usearch, @cap/usrsctp, @cap/utf8proc, @cap/vorbis, @cap/whisper, @cap/xxhash, @cap/yyjson

### 其他通用工具包 (231)
@atproto/syntax, @faker-js/faker, @opentelemetry/core, abbrev, acorn, addressparser, ahash, anstyle, anstyle-parse, ascii, ascii85, average, b-spline, badge-maker, basic-auth, bendy, bezier-easing, binary-extensions, bit_field, bl, bn.js, borsh, boyer-moore-magiclen, bvh, bwt, calver, cast, cbc, ccount, cesu8, cexpr, character-entities, ciborium-ll, class-variance-authority, clf-parser, cliui, clsx, cobs, codespan-reporting, combine, commondir, compressible, conventional-commits-parser, count-min-sketch, credit-card-type, ctr, curve25519-dalek, d3-ease, d3-interpolate, d3-scale, depd, detect-newline, didyoumean, dlv-list, doctrine, dom-accessibility-api, dom-serializer, email_address, error-ex, escalade, esquery, esutils, expr-eval, express-bearer-token, ezing, fast-fifo, fast-redact, fast-socks5, fastq, fenwick, file-type, filenamify, flat, flatted, foldhash, fraction.js, h2, hamming, has-flag, hashbrown, hashring, hdrhistogram, highlight.js, home, hostile, hot-shots, huffman-compress, ical-generator, icu_locid, idempotency-key, idna, ieee754, immer, indoc, isbn, istanbul-lib-coverage, istextorbinary, itertools, js-tokens, jsesc, kafkajs, kalman-filters, kalmanjs, kdbush, kiddo, kind-of, kmp, language-tags, ldapjs, lerp, levn, libm, libphonenumber-js, line-column, litemap, litrs, luhn, lunr, mailto-link, marked, md-5, memmem, methods, monotone-chain-convex-hull, morgan, moving-averages, natord, natural-compare, node-pid-controller, noise, nopt, number_prefix, numeral, opentelemetry, optionator, palette, parry2d, parry3d, parse5, pep440_rs, perlin-noise, phf_generator, phonenumber, pid, pkcs1, pkcs8, playwright, plist, plotters-svg, point-in-polygon, poly1305, potential_utf, powerfmt, predicates, prettier, prom-client, puppeteer, quinn-proto, radsort, rangemap, redent, reed-solomon-erasure, rehype, rend, requires-port, resp, rfc6979, romans, rrule, rs_merkle, salsa20, saxes, scroll, sct, sec1, server-timing, sfv, shortid, similar, simple-statistics, simplify-js, slash, smart-buffer, smawk, snap, soundex-code, sparkly, spiffe, spki, splines, sqids, ssri, stemmer, stomp, stylis, subtitle, suffix, tapable, terser, test-exclude, tinybench, titleize, tonic, trough, ufo, unflatten, unique-names-generator, unist-util-visit, untrusted, urn-lib, utf-8, uuencode, vcard, version-compare, vfile-message, victor, vte, wcag-contrast, webpki-roots, weezl, widest-line, words-count, wordwrap, wyhash, x509-cert, xorf, y18n, yallist, yn, z85, zigzag
