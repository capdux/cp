# CP 标准库按领域分类总览目录 (`cap:*` Catalog)

> 本文档由 `scripts/gen_lib_catalog.py` 自动扫描 `lib/` 全部模块及溯源头生成，100% 覆盖无遗漏、无重复。
> 标签说明：`[Bun]` = 吸收自 `/data/dev/bun/src/` 的 Bun 衍生库；`[Pure-CP]` = 100% 纯 CP 原创零依赖标准库。

## 总体统计与领域分布

- **标准库模块总数**：**281** 个（`[Bun]` **169** 个 / `[Pure-CP]` **112** 个）

| 领域分类 | 英文域 | 模块数 | `[Bun]` | `[Pure-CP]` |
| :--- | :--- | :---: | :---: | :---: |
| [1. 核心运行时、并发与系统原语](#1-核心运行时并发与系统原语) | Core Runtime, Concurrency & OS | 45 | 34 | 11 |
| [2. 基础数据结构、容器与经典算法](#2-基础数据结构容器与经典算法) | Collections, Data Structures & Algorithms | 17 | 12 | 5 |
| [3. Web、HTTP、路由与微服务生态](#3-webhttp路由与微服务生态) | Web, HTTP, Routing & Microservices | 43 | 33 | 10 |
| [4. 网络协议、传输层与分布式通信](#4-网络协议传输层与分布式通信) | Networking, Protocols & Messaging | 13 | 6 | 7 |
| [5. 密码学、哈希与安全认证](#5-密码学哈希与安全认证) | Cryptography, Hashing & Security | 12 | 3 | 9 |
| [6. 存储、数据库、文件系统与可观测性](#6-存储数据库文件系统与可观测性) | Storage, Databases, Filesystems & Observability | 22 | 7 | 15 |
| [7. 压缩、归档与二进制编解码](#7-压缩归档与二进制编解码) | Compression, Archiving & Binary Codecs | 16 | 5 | 11 |
| [8. 文本、文档格式与模板引擎](#8-文本文档格式与模板引擎) | Text, Document Formats & Templating | 33 | 27 | 6 |
| [9. 图像、音视频与多媒体图形](#9-图像音视频与多媒体图形) | Media, Audio, Video & 3D Graphics | 11 | 5 | 6 |
| [10. 编译原理、构建系统与开发者工具链](#10-编译原理构建系统与开发者工具链) | Compilers, Build Systems & DevTools | 38 | 27 | 11 |
| [11. 数值计算、线性代数与微积分](#11-数值计算线性代数与微积分) | Numerical Analysis, Linear Algebra & Calculus | 5 | 3 | 2 |
| [12. 概率统计、机器学习与 AI Agent](#12-概率统计机器学习与-ai-agent) | Statistics, Machine Learning & AI Agents | 10 | 1 | 9 |
| [14. 图论、计算几何与地理空间（GIS）](#14-图论计算几何与地理空间gis) | Graphs, Computational Geometry & Geospatial | 3 | 0 | 3 |
| [16. 其他通用工具与扩展模块](#16-其他通用工具与扩展模块) | Other Utilities & Extensions | 13 | 6 | 7 |

---

## 1. 核心运行时、并发与系统原语

*Core Runtime, Concurrency & OS* — 共 **45** 个模块（`[Bun]`: 34，`[Pure-CP]`: 11）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| `cap:_cap` | `[Bun]` | cap:_cap 标准库模块 |
| `cap:actor` | `[Pure-CP]` | CP native actor runtime, per CHANGELOG |
| [`cap:aio`](./aio.md) | `[Bun]` | cap:aio 是 CP 语言官方内置的Linux 父进程退出看门狗（ParentDeathWatchdog）与零堆分配 POSIX 异步提交/完成环形队列（AioQueue<CAP>）标准库。吸收自 Bun（/data |
| `cap:beam` | `[Pure-CP]` | CP native Bun-BEAM bridge, per CHANGELOG |
| [`cap:bit_ops`](./bit_ops.md) | `[Bun]` | 纯 CP 零开销底层位运算、位域提取/插入、2 的幂对齐与置位迭代器库。 |
| [`cap:bun_alloc`](./bun_alloc.md) | `[Pure-CP]` | 高性能自定义内存分配器套件 |
| [`cap:clap_stream`](./clap_stream.md) | `[Bun]` | 纯 CP 零堆分配流式命令行参数解析器。 |
| [`cap:conf`](./conf.md) | `[Bun]` | 统一配置交换引擎（YAML 1.2 / TOML 1.0 / INI / JSON5 & `ConfGetter`） |
| [`cap:crash_handler`](./crash_handler.md) | `[Pure-CP]` | cap:crash_handler（以及动态命名空间 cap.crash_handler）提供纯 CP 实现的工业级结构化崩溃诊断报告、栈帧符号格式化、零堆固定容量写入器与环形面包屑缓冲区。设计目标是在信号处理与异常现场 |
| [`cap:cron`](./cron.md) | `[Bun]` | cap:cron 是 CP 语言官方内置的纯 CP 5/6 字段 Cron 表达式解析与 UTC 触发时间计算库，零 C 依赖、零 std.posix、零 @import("bun")。内部采用位掩码（bitmask）紧 |
| [`cap:dotenv`](./dotenv.md) | `[Bun]` | 零分配 `.env` 环境变量解析、变量插值与多环境级联加载器 |
| [`cap:dotenv_parse`](./dotenv_parse.md) | `[Bun]` | 零堆分配 `.env` 解析器与变量展开器 |
| [`cap:errno`](./errno.md) | `[Bun]` | cap:errno 是 CP 语言官方内置的跨平台操作系统错误码标准库，吸收自 Bun 的系统错误码体系（/data/dev/bun/src/errno/{linux_errno,darwin_errno,windows |
| [`cap:errno_map`](./errno_map.md) | `[Bun]` | 纯 CP 跨平台 POSIX/OS Errno 统一映射与元数据查询库 |
| [`cap:event`](./event.md) | `[Bun]` | cap:event（别名 cap:events、cap:event_loop）统一整合 CP 语言的应用层事件发布/订阅模型（EventBus、EventEmitter、协程信号 Event）与底层高性能异步事件循环内核 |
| [`cap:fallback_alloc`](./fallback_alloc.md) | `[Bun]` | 栈内联缓冲优先 + 堆回退分配器 |
| [`cap:feature_flags`](./feature_flags.md) | `[Pure-CP]` | 纯 CP 特性开关与环境变量强类型解析引擎（吸收自 Bun `bun_core`） |
| [`cap:ffi`](./ffi.md) | `[Bun]` | POSIX 动态库加载、TinyCC 内存编译与 C-ABI 布局计算标准库 |
| [`cap:fs`](./fs.md) | `[Pure-CP]` | cap:fs 是 CP 语言官方内置的高阶文件系统标准库，采用 双模编译期分发（@is_nest） 架构：在 BEAM/Nest 分布式调度模式下，异步文件操作通过 SystemAction.FS_* 挂起当前协程并委托 |
| [`cap:fsm`](./fsm.md) | `[Pure-CP]` | 确定性有限状态机、守卫、拓扑分析与熔断/DAG 引擎 |
| [`cap:h2stream`](./h2stream.md) | `[Bun]` | 零堆分配 HTTP/2 流状态机与流控窗口库 |
| [`cap:io`](./io.md) | `[Bun]` | cap:io 是 CP 语言官方内置的统一流式 I/O 标准库，基于 Reader / Writer / Closer / Seeker 四大解耦 Trait 契约，完整移植 Bun src/io/ 的生产级流式与非阻塞 |
| [`cap:lockfree_queue`](./lockfree_queue.md) | `[Bun]` | 无锁并发环形队列、侵入式无界队列与有界通道 |
| [`cap:mem`](./mem.md) | `[Bun]` | cap:mem 是 CP 语言官方内置的高性能内存分配器（MaxHeapAllocator、StackFallbackAllocator、BumpArena、Arena）、Mimalloc C-FFI 底层绑定、跨 Ac |
| [`cap:meta`](./meta.md) | `[Bun]` | cap:meta 是 CP 语言官方内置的编译期类型内省（Type Traits）与无分支位操作标准库，吸收自 Bun src/meta/（bits.zig、traits.zig、meta.zig）。该库向编译器工具链、 |
| [`cap:napi`](./napi.md) | `[Bun]` | cap:napi 提供与 Node.js / Bun 完全一致的 Node-API v9 C-ABI 类型布局、完整 24 项状态码与错误消息映射，以及纯 CP 可离线测试的运行时上下文 NapiRuntimeConte |
| [`cap:object_pool`](./object_pool.md) | `[Bun]` | 侵入式单链表空闲对象池 |
| [`cap:padding_checker`](./padding_checker.md) | `[Bun]` | 编译期结构体零未初始化填充校验器 |
| `cap:parallel` | `[Pure-CP]` | CP native async/parallel runtime, per CHANGELOG |
| [`cap:ptr`](./ptr.md) | `[Bun]` | cap:ptr 是 CP 语言官方内置的零 GC 高性能智能指针、侵入式/非侵入式引用计数、写时复制（Copy-on-Write）与 64 位打标指针（Tagged Pointer）标准库。完整吸收自 Bun（/data |
| [`cap:rate`](./rate.md) | `[Bun]` | Zero-Alloc Rate Limiting Suite |
| `cap:reflect` | `[Pure-CP]` | CP native stdlib |
| [`cap:retry_backoff`](./retry_backoff.md) | `[Bun]` | 基于指数退避与 RFC 9110 §10.2.3 Retry-After 规范，提供纯 CP、零外部依赖、零堆分配的重试调度与 Retry-After 解析库。 |
| [`cap:safety`](./safety.md) | `[Bun]` | cap:safety 是 CP 语言官方内置的线程归属校验（ThreadLock）、可重入读写竞态检测（CriticalSection）、非托管容器分配器一致性守卫（CheckedAllocator）、AddressSa |
| [`cap:simd`](./simd.md) | `[Bun]` | Highway 级 SIMD 硬件向量加速与高速字节扫描标准库 |
| [`cap:stream`](./stream.md) | `[Bun]` | cap:stream 是 CP 语言面向流式数据处理场景的标准门面模块（Facade），直接与底层 cap:io 共享零开销的 Trait 契约与实现（无任何中间适配器包装损耗），确保通过 cap:stream 与 ca |
| [`cap:sync`](./sync.md) | `[Bun]` | cap:sync（与底层 cap:threading 共享同步原语）是 CP 语言专为多核低延迟并发、用户态快速锁（Futex）、无锁队列（Lock-Free MPMC / Unbounded Batch Queue）、 |
| [`cap:sys`](./sys.md) | `[Bun]` | cap:sys 是 CP 语言官方内置的底层系统调用与操作系统原语标准库，深度吸收 Bun 生产级 src/sys/ 实现，并完全基于 Zig 0.17 原生 std.c.* 与 std.os.linux.* 构建（零  |
| [`cap:tagged_ptr`](./tagged_ptr.md) | `[Bun]` | 64 位高位标签指针与紧凑标签联合体 |
| [`cap:textstream`](./textstream.md) | `[Bun]` | 零堆分配 WHATWG 流式 UTF-8 编码器（TextEncoderStream） |
| [`cap:threading`](./threading.md) | `[Pure-CP]` | cap:threading（源码位于 lib/threading/）是纯 CP 实现的工业级并发同步原语标准库，完全基于 std.atomic.Value 与 std.Thread.Futex 构建，零堆分配、无 std |
| [`cap:time`](./time.md) | `[Bun]` | cap:time 是 CP 语言官方内置的工业生产级纯 CP 时间、日历转换、Cron 调度与限流退避标准库，零外部依赖、零 std.posix、零 @import("bun")、零堆分配，适用于 BEAM 调度器、分布 |
| [`cap:type_traits`](./type_traits.md) | `[Bun]` | 编译期类型特征自省与指针元数据工具 |
| [`cap:watcher`](./watcher.md) | `[Bun]` | 跨平台文件系统监控、路径过滤与事件合并防抖引擎 |
| `cap:worker` | `[Pure-CP]` | CP native worker runtime, per CHANGELOG |

## 2. 基础数据结构、容器与经典算法

*Collections, Data Structures & Algorithms* — 共 **17** 个模块（`[Bun]`: 12，`[Pure-CP]`: 5）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:bplus_tree`](./bplus_tree.md) | `[Pure-CP]` | 工业级 B+ 树索引与 4KB Slotted Page 标准库 |
| [`cap:cache`](./cache.md) | `[Pure-CP]` | cap:cache 是 CP 语言官方内置的纯原生高性能内存缓存与概率数据结构库，专为 AI Agent 响应缓存、Prompt/Embedding 缓存、高频去重与热键频次统计打造。包含三大核心组件： |
| [`cap:col`](./col.md) | `[Bun]` | cap:col（兼容映射 cap:collections）是 CP 语言专为百万级 AI Agent 并发、确定性无 GC 内存管理与 CPU 缓存行局部性（Cache-Line Locality）设计的工业级数据结构标 |
| [`cap:collections`](./collections.md) | `[Pure-CP]` | 工业级基础数据结构集合标准库 |
| [`cap:crdt`](./crdt.md) | `[Pure-CP]` | cap:crdt 是 CP 语言官方内置的纯原生状态型无冲突复制数据类型（State-Based CRDT / CvRDT）标准库，专为多 Agent 分布式最终一致性状态同步、离线协作与无中心冲突收敛打造。全模块严格满 |
| [`cap:diff`](./diff.md) | `[Bun]` | cap:diff 是 CP 语言官方内置的纯 CP 文本差异分析与代码合并标准库，零外部依赖、零 std.posix、零 @import("bun")，专为 CP 代码工具链与 AI Coding Agent 设计。既可 |
| [`cap:exact_matcher`](./exact_matcher.md) | `[Bun]` | 定长整型打包 $O(1)$ 字符串快速匹配器 |
| [`cap:glob`](./glob.md) | `[Bun]` | 高性能通配符状态机与目录扫描引擎 |
| [`cap:glob_escape`](./glob_escape.md) | `[Bun]` | Glob 通配符转义、静态前缀提取与否定模式分析库 |
| [`cap:histogram`](./histogram.md) | `[Pure-CP]` | cap:histogram 是 CP 语言官方内置的纯 CP 工业级高动态范围延迟直方图（HDR Histogram）与流式速率统计库，零 C 依赖、零 std.posix、零 @import("bun")。基于 Gil |
| [`cap:hive_array`](./hive_array.md) | `[Bun]` | 提供纯 CP、零外部依赖、零堆分配的固定容量位图追踪 Slab 槽池。 |
| [`cap:intrusive_heap`](./intrusive_heap.md) | `[Bun]` | 零堆分配侵入式配对堆 (Intrusive Pairing Heap) |
| [`cap:known_globals`](./known_globals.md) | `[Bun]` | 纯 CP ECMAScript / Web / Node / Bun 已知全局符号与无副作用判定库 |
| [`cap:myers_diff`](./myers_diff.md) | `[Bun]` | Myers $O(ND)$ 最短编辑脚本 Diff 引擎 |
| [`cap:patch`](./patch.md) | `[Bun]` | cap:patch 是 CP 语言官方内置的行级 Myers $O(ND)$ 文本差分引擎、完整多文件/多 Hunk Unified Diff 解析器、支持精确/偏移/上下文模糊容错（Fuzz/Offset）的补丁应用引 |
| [`cap:sort`](./sort.md) | `[Bun]` | 工业级自适应排序（PDQSort / BlockSort / HeapSort）与二分检索标准库 |
| [`cap:static_hashmap`](./static_hashmap.md) | `[Bun]` | 编译期/零分配静态哈希表 |

## 3. Web、HTTP、路由与微服务生态

*Web, HTTP, Routing & Microservices* — 共 **43** 个模块（`[Bun]`: 33，`[Pure-CP]`: 10）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:alt_svc`](./alt_svc.md) | `[Bun]` | RFC 7838 HTTP `Alt-Svc` 头部解析与 HTTP/3 发现缓存 |
| [`cap:autolink`](./autolink.md) | `[Bun]` | GFM 宽松自动链接（URL / WWW / Email）扫描器 |
| [`cap:byterange`](./byterange.md) | `[Bun]` | 零堆分配 HTTP Range 与 Content-Range 解析格式化库 |
| [`cap:content_range`](./content_range.md) | `[Bun]` | 纯 CP 零堆分配 RFC 9110 §14 HTTP Range 请求头与 Content-Range 响应头解析/格式化库（用于 206 Partial Content 与 416 Range Not Satisfi |
| [`cap:cors_policy`](./cors_policy.md) | `[Bun]` | WHATWG Fetch CORS 策略与预检评估引擎 |
| [`cap:css`](./css.md) | `[Bun]` | cap:css 是 CP 语言官方内置的工业级 CSS 解析、词法分析、颜色空间与数学求值、选择器权重匹配、变量解析、厂商前缀/逻辑属性展开、CSS 嵌套展平与样式表压缩/美化标准库。整体架构吸收自 Bun 内置的 Li |
| [`cap:cssmodules`](./cssmodules.md) | `[Bun]` | 纯 CP 零分配 CSS Modules 局部类名哈希与 `composes` 解析库 |
| [`cap:data_url`](./data_url.md) | `[Bun]` | RFC 2397 Data URL 零分配解析与最短编码器 |
| [`cap:etag`](./etag.md) | `[Bun]` | 纯 CP 零堆分配 RFC 9110 / RFC 7232 HTTP ETag 解析、强/弱比较、If-None-Match / If-Match 条件请求求值与 FNV-1a 64-bit 内容 ETag 生成库。 |
| [`cap:exe_format`](./exe_format.md) | `[Pure-CP]` | 可执行二进制格式零拷贝探测与解析库 |
| [`cap:fs_router`](./fs_router.md) | `[Bun]` | 零堆分配文件系统路由解析与匹配引擎 |
| [`cap:graphql`](./graphql.md) | `[Pure-CP]` | cap:graphql 是 CP 语言官方内置的纯原生 GraphQL（October 2021 规范）可执行文档词法扫描、递归下降 AST 解析、多维安全护栏与规范化/压缩打印引擎，专为 API 网关、BFF（Back |
| [`cap:grpc_wire`](./grpc_wire.md) | `[Pure-CP]` | gRPC over HTTP/2 线协议标准库 |
| [`cap:h2_encode`](./h2_encode.md) | `[Bun]` | 零堆分配 HTTP/2 出站请求与流控帧编码库 |
| [`cap:h2_frame`](./h2_frame.md) | `[Bun]` | HTTP/2 (RFC 9113) 线协议帧头、设置与优先级编解码器 |
| [`cap:hpack`](./hpack.md) | `[Pure-CP]` | RFC 7541 HTTP/2 HPACK 头部压缩编解码器 |
| [`cap:html`](./html.md) | `[Bun]` | cap:html 是 CP 语言官方内置的工业生产级 HTML5 解析、内容安全清洗与 AI Agent 网页语义提取标准库，对标 Bun 的 HTMLRewriter（/data/dev/bun/src/lolhtml |
| [`cap:html_asset_scanner`](./html_asset_scanner.md) | `[Bun]` | 零堆分配 HTML 资源引用扫描器 |
| [`cap:http`](./http.md) | `[Bun]` | cap:http 是 CP 语言官方内置的全栈工业级 HTTP 协议子系统，完整吸收自 Bun（/data/dev/bun/src/http/、/data/dev/bun/src/http_types/、/data/de |
| [`cap:http2_frame`](./http2_frame.md) | `[Pure-CP]` | RFC 9113 HTTP/2 二进制帧与多路复用状态机 |
| [`cap:jwt`](./jwt.md) | `[Bun]` | RFC 7519 JWT Claims Zero-Allocation Parser |
| [`cap:keyframes`](./keyframes.md) | `[Bun]` | 纯 CP 零分配 CSS `@keyframes` 解析、压缩与时间轴采样库 |
| [`cap:mcp`](./mcp.md) | `[Pure-CP]` | Model Context Protocol (MCP 2025-03-26) 标准库 |
| [`cap:mediaquery`](./mediaquery.md) | `[Bun]` | 纯 CP 零分配 CSS Media Queries Level 4 解析与求值库 |
| [`cap:mime`](./mime.md) | `[Bun]` | cap:mime 是 CP 语言官方内置的纯 CP RFC 6838 MediaType 解析/格式化、RFC 9110 HTTP Accept 内容协商（Content Negotiation）与全量扩展名双向数据库， |
| [`cap:number_format`](./number_format.md) | `[Bun]` | 千位分组、定点数、英文序数词与多进制格式化库 |
| [`cap:oauth2`](./oauth2.md) | `[Pure-CP]` | cap:oauth2 是 CP 语言官方内置的纯 CP 零 C 依赖 OAuth 2.0 (RFC 6749)、PKCE (RFC 7636)、Token 吊销 (RFC 7009) 与 OpenID Connect ( |
| [`cap:pe_format`](./pe_format.md) | `[Bun]` | Windows PE64 可执行文件头与节表解析/校验/构建库 |
| [`cap:platform`](./platform.md) | `[Bun]` | cap:platform 是 CP 语言官方内置的跨平台操作系统探测、内核版本解析与系统能力检测标准库，吸收自 Bun src/platform/（linux.zig、darwin.zig）。该库向事件循环、文件 I/O |
| [`cap:qpack`](./qpack.md) | `[Bun]` | 零堆分配 HTTP/3 QPACK 静态表与 Field Section 编解码库 |
| [`cap:rfc7807_problem`](./rfc7807_problem.md) | `[Bun]` | RFC 9457 / RFC 7807 HTTP Problem Details 构造与序列化库 |
| [`cap:robots_txt`](./robots_txt.md) | `[Bun]` | RFC 9309 `robots.txt` 解析与 URL 匹配引擎 |
| [`cap:route_params`](./route_params.md) | `[Bun]` | HTTP 路由参数映射表与 URLPath 零分配解析器 |
| [`cap:router`](./router.md) | `[Bun]` | cap:router 是 CP 语言官方内置的泛型 Radix-Tree（前缀字典树）与 Next.js/Bun 风格 FileSystemRouter 路由标准库，支持按 HTTP 方法隔离路由树、5 级严格优先级匹配 |
| [`cap:sri`](./sri.md) | `[Bun]` | W3C Subresource Integrity (SRI) 解析、格式化与流式校验器 |
| [`cap:structured_headers`](./structured_headers.md) | `[Bun]` | 基于 RFC 8941 HTTP Structured Field Values 规范，提供纯 CP、零外部依赖、零堆分配的 SFV 标量项、参数列表解析与序列化器。 |
| [`cap:uri_normalize`](./uri_normalize.md) | `[Bun]` | 基于 RFC 3986 URI 规范化标准，提供纯 CP、零外部依赖、零堆分配的 URI 路径点段归约、百分号编码规范化与默认端口剥离库。 |
| [`cap:url`](./url.md) | `[Bun]` | cap:url 是 CP 语言官方内置的工业级 URL 解析与 URI 编解码标准库，全量对标并吸收自 Bun 原生 URL 运行时实现（/data/dev/bun/src/url/url.cp）。 |
| [`cap:w3c_traceparent`](./w3c_traceparent.md) | `[Bun]` | 基于 W3C Trace Context Level 1 规范，提供纯 CP、零外部依赖、零堆分配的 traceparent 与 tracestate 分布式追踪头解析与格式化库。 |
| [`cap:web`](./web.md) | `[Pure-CP]` | cap:web 是 CP 语言官方内置的纯原生、零外部 C 依赖 Web 与 API 生态标准库，统一覆盖 OpenAPI 3.1 规范校验、MessagePack-RPC、HTTP/1.1/2/3 协议解析与二进制帧编 |
| [`cap:webauthn`](./webauthn.md) | `[Pure-CP]` | FIDO2 / WebAuthn Level 3 服务端验证标准库 |
| [`cap:websocket_frame`](./websocket_frame.md) | `[Pure-CP]` | RFC 6455 WebSocket 帧协议解析、掩码与分片重组引擎 |
| [`cap:ws`](./ws.md) | `[Bun]` | cap:ws 是 CP 语言官方内置的工业生产级 RFC 6455 / RFC 7692 WebSocket 协议引擎，在保留既有高性能帧编解码、SIMD 掩码与握手校验的基础上，新增子协议与扩展头协商、RFC 6455 |

## 4. 网络协议、传输层与分布式通信

*Networking, Protocols & Messaging* — 共 **13** 个模块（`[Bun]`: 6，`[Pure-CP]`: 7）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:bencode`](./bencode.md) | `[Pure-CP]` | BEP-3 Bencode 编解码与 Torrent 规范化哈希引擎 |
| [`cap:cluster`](./cluster.md) | `[Pure-CP]` | cap:cluster 是 CP 语言官方内置的纯原生分布式集群协调与分片路由标准库，专为百万级 AI Agent 集群分片路由、SWIM Gossip 健康成员表与防脑裂选主租约打造。包含三大核心组件： |
| [`cap:dns`](./dns.md) | `[Bun]` | cap:dns 是 CP 语言官方内置的工业生产级域名解析（DNS）协议套件，涵盖 RFC 1035/2782/3596/8484/8659 报文与资源记录编解码、DNS-over-HTTPS（DoH）、POSIX /e |
| [`cap:email`](./email.md) | `[Pure-CP]` | cap:email 是 CP 语言官方内置的零外部依赖电子邮件协议核心库，完整覆盖 RFC 5322 邮件地址解析与 CRLF 头注入防护、RFC 2045 / RFC 2047 MIME 邮件构建与递归解析（含 Quo |
| [`cap:kafka_wire`](./kafka_wire.md) | `[Pure-CP]` | Apache Kafka Wire Protocol & RecordBatch v2 标准库 |
| [`cap:mail`](./mail.md) | `[Bun]` | 电子邮件地址、RFC 2047 编码词与 Quoted-Printable 工具库 |
| [`cap:mqtt`](./mqtt.md) | `[Pure-CP]` | 纯 CP 工业级 MQTT v3.1.1 / v5.0 协议库 |
| [`cap:net`](./net.md) | `[Bun]` | cap:net 是 CP 语言官方内置的工业生产级网络传输标准库，涵盖 IPv4/IPv6 地址与 CIDR/SSRF 边界校验、Linux 原生套接字选项与 poll 就绪轮询、RAII 风格 SafeSocket 生 |
| [`cap:quic_header`](./quic_header.md) | `[Bun]` | 纯 CP QUIC 报文头编解码、QPACK 头分类与 Socket 类型枚举库 |
| [`cap:redis`](./redis.md) | `[Bun]` | cap:redis（以及底层同源协议层 lib/valkey/）是 CP 语言官方内置的工业级 Redis / Valkey 序列化协议（RESP2 / RESP3）标准库，全面吸收自 Bun 原生 Valkey 协议实 |
| [`cap:ssh`](./ssh.md) | `[Bun]` | cap:ssh 是 CP 语言官方内置的纯 CP OpenSSH 公钥 Wire 格式解析、标准 SHA256: 指纹计算与 authorized_keys 策略解析引擎，零 C 依赖、零 std.posix、零 @im |
| [`cap:ssh_wire`](./ssh_wire.md) | `[Pure-CP]` | cap:ssh_wire（亦可通过 cap.ssh_wire 访问）提供纯 CP 实现的工业级 SSH-2 Wire 二进制编解码、OpenSSH 公私钥格式解析及 SSHSIG 签名/验签能力： |
| [`cap:syslog`](./syslog.md) | `[Pure-CP]` | RFC 5424 & RFC 3164 Syslog 解析与格式化引擎 |

## 5. 密码学、哈希与安全认证

*Cryptography, Hashing & Security* — 共 **12** 个模块（`[Bun]`: 3，`[Pure-CP]`: 9）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:auth`](./auth.md) | `[Pure-CP]` | cap:auth 是 CP 语言官方内置的纯原生零 C 依赖认证、授权、访问控制与 PKI/X.509 身份基础设施标准库，覆盖： |
| [`cap:cel`](./cel.md) | `[Pure-CP]` | Common Expression Language 安全策略求值引擎 |
| [`cap:cose`](./cose.md) | `[Pure-CP]` | COSE (RFC 9052 / RFC 9053) 标准库 |
| [`cap:crypto`](./crypto.md) | `[Bun]` | cap:crypto 是 CP 语言官方内置的生产级零 C 依赖全栈现代密码学子系统标准库，结合 Zig 0.17 原生 std.crypto 与 Bun src/boringssl/、src/sha_hmac/ 现代密 |
| [`cap:csrf`](./csrf.md) | `[Pure-CP]` | CSRF 防护标准库 |
| [`cap:hash`](./hash.md) | `[Bun]` | cap:hash 是 CP 语言官方内置的超高速非加密哈希（Non-Cryptographic Hash Functions）与校验和矩阵标准库。模块吸收自 Bun（/data/dev/bun/src/wyhash/wy |
| [`cap:highway`](./highway.md) | `[Pure-CP]` | 纯 CP SIMD 向量化字节扫描、ASCII 校验与子串检索标准库 |
| [`cap:paseto`](./paseto.md) | `[Pure-CP]` | PASETO v4 安全认证令牌标准库 |
| [`cap:security`](./security.md) | `[Bun]` | cap:security 是 CP 语言官方内置的应用安全与鉴权标准库，提供纯 CP 实现的 CSRF 无状态令牌、RFC 4226/6238 HOTP/TOTP 双因素认证与 RFC 4648 Base32、Fetch |
| [`cap:sha_hmac`](./sha_hmac.md) | `[Pure-CP]` | 统一密码学哈希、HMAC、HKDF 与 PBKDF2 标准库 |
| [`cap:wyhash`](./wyhash.md) | `[Pure-CP]` | lib/wyhash/ 提供纯 CP 实现的工业级 Wyhash v4 高速 64-bit 非加密哈希引擎与 WyRand 伪随机数生成器，支持全无状态一次性哈希、任意分块流式增量哈希、Lemire 无偏差范围随机整数生 |
| [`cap:xxhash`](./xxhash.md) | `[Pure-CP]` | 工业级 xxHash 与流式分块哈希标准库 |

## 6. 存储、数据库、文件系统与可观测性

*Storage, Databases, Filesystems & Observability* — 共 **22** 个模块（`[Bun]`: 7，`[Pure-CP]`: 15）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:analytics`](./analytics.md) | `[Pure-CP]` | 运行时遥测收集与脱敏批处理标准库 |
| [`cap:arrow_ipc`](./arrow_ipc.md) | `[Pure-CP]` | Apache Arrow IPC 列式内存批处理与零拷贝读写标准库 |
| [`cap:blob`](./blob.md) | `[Bun]` | cap:blob 是 CP 语言官方内置的 Web 标准二进制大对象（Blob）与具名文件（File）抽象库，吸收自 Bun 的 WebCore Blob/Store 子系统（/data/dev/bun/src/runt |
| [`cap:bloburl`](./bloburl.md) | `[Bun]` | 零堆分配 W3C Blob Object URL 解析与注册表库 |
| [`cap:db`](./db.md) | `[Pure-CP]` | 统一数据库存储引擎、事务并发、缓存与索引标准库 |
| [`cap:devops`](./devops.md) | `[Pure-CP]` | cap:devops 是 CP 语言官方内置的纯原生、零外部 C 依赖基础设施与 DevOps 标准库，统一覆盖 系统软件包（DEB/RPM/APK）、光盘与虚拟化镜像（DMG/ISO 9660/UDF/VDI/VHD） |
| [`cap:gguf`](./gguf.md) | `[Pure-CP]` | lib/gguf/（对外协议 cap:gguf 与 cap.gguf）是纯 CP 实现的工业级 GGUF v2/v3（llama.cpp 本地大模型权重与元数据二进制格式）解析、构建与块量化/反量化标准库。 |
| [`cap:gossip`](./gossip.md) | `[Pure-CP]` | Sans-IO SWIM 分布式集群成员发现与节点健康探测标准库 |
| [`cap:lsm_sst`](./lsm_sst.md) | `[Pure-CP]` | LSM-Tree Sorted String Table (SSTable) 标准库 |
| [`cap:metrics`](./metrics.md) | `[Pure-CP]` | cap:metrics 是 CP 语言官方内置的纯原生可观测性指标与 Prometheus/OpenMetrics 导出库，专为百万级 AI Agent 并发运行时与云原生服务 /metrics 端点采集打造。核心组件分 |
| [`cap:mysql`](./mysql.md) | `[Bun]` | MySQL 协议与 Wire 编解码标准库 |
| [`cap:onnx`](./onnx.md) | `[Pure-CP]` | ONNX 计算图与权重模型二进制解析/构建标准库 |
| [`cap:otlp`](./otlp.md) | `[Pure-CP]` | OpenTelemetry OTLP 遥测标准库 |
| [`cap:parquet`](./parquet.md) | `[Pure-CP]` | Apache Parquet 列式存储与 Thrift / RLE 编解码库 |
| [`cap:perf`](./perf.md) | `[Pure-CP]` | 性能剖析、作用域计时与 Trace Event 导出 |
| [`cap:pg`](./pg.md) | `[Bun]` | 纯 CP PostgreSQL Wire Protocol v3 全栈协议与类型工具库 |
| [`cap:pgwire`](./pgwire.md) | `[Bun]` | PostgreSQL v3.0 Wire Protocol 标准库 |
| [`cap:raft`](./raft.md) | `[Pure-CP]` | Sans-IO Raft 分布式共识状态机与日志复制引擎 |
| [`cap:s3`](./s3.md) | `[Bun]` | cap:s3 是 CP 语言官方内置的高性能对象存储标准库，全量吸收自 Bun 原生 S3 引擎（/data/dev/bun/src/s3_signing/ 及 runtime/webcore/s3/）。模块完全兼容 A |
| [`cap:safetensors`](./safetensors.md) | `[Pure-CP]` | Safetensors 模型权重格式与 BF16/FP16 转换库 |
| [`cap:sql`](./sql.md) | `[Bun]` | cap:sql 是 CP 语言官方内置的多方言关系型数据库协议与嵌入式存储标准库，直接原版移植自 Bun 原生 SQL 驱动与 bun:sqlite 引擎（/data/dev/bun/src/sql/，10 个文件 4, |
| `cap:sqlite` | `[Pure-CP]` | cap:sqlite binding, per CHANGELOG |

## 7. 压缩、归档与二进制编解码

*Compression, Archiving & Binary Codecs* — 共 **16** 个模块（`[Bun]`: 5，`[Pure-CP]`: 11）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:base64`](./base64.md) | `[Pure-CP]` | Pure CP Base64 / Base64URL Standard Library |
| [`cap:binary_schema`](./binary_schema.md) | `[Bun]` | 纯 CP 紧凑二进制 Schema 流式编解码引擎 |
| [`cap:capnp`](./capnp.md) | `[Pure-CP]` | Cap'n Proto 零拷贝二进制编解码库 |
| [`cap:cbor`](./cbor.md) | `[Pure-CP]` | cap:cbor 是 CP 语言官方内置的高性能紧凑二进制序列化标准库，提供完整的 RFC 8949 CBOR 与 Section 4.2 确定性规范编码（Deterministic Canonical Encoding |
| [`cap:deflate`](./deflate.md) | `[Pure-CP]` | 纯 CP RFC 1951 DEFLATE / RFC 1952 GZIP / RFC 1950 ZLIB 编解码库 |
| [`cap:encoding`](./encoding.md) | `[Bun]` | cap:encoding 是 CP 语言官方内置的工业级二进制与文本编码转换标准库，全面吸收自 Bun 生产级运行时源码（/data/dev/bun/src/base64/base64.cp 及字符编码基础设施）。 |
| [`cap:hex_dump`](./hex_dump.md) | `[Bun]` | 零堆分配十六进制编解码与 `hexdump -C` 诊断格式化器 |
| [`cap:interchange`](./interchange.md) | `[Pure-CP]` | cap:interchange 是 CP 语言官方内置的纯 CP 统一数据交换格式标准库，零外部依赖、零 std.posix、零 @import("bun")，提供统一的 InterchangeValue 模型与 Int |
| [`cap:libarchive`](./libarchive.md) | `[Pure-CP]` | 纯 CP 统一归档格式探测与 Ar / CPIO / USTAR Tar 读写标准库 |
| [`cap:lzw`](./lzw.md) | `[Pure-CP]` | Pure CP Zero-Allocation LZW Codec (GIF LSB & TIFF/PDF MSB) |
| [`cap:pack`](./pack.md) | `[Bun]` | cap:pack（在包管理器中同时兼容映射为 cap:compress）是 CP 语言官方内置的工业级无损数据压缩、流式解压、零拷贝归档（Tar / PKZIP）与完整性校验和（CRC32 / Adler32）全栈标准库 |
| `cap:serde` | `[Bun]` | cap:serde 标准库模块 |
| [`cap:tlv`](./tlv.md) | `[Pure-CP]` | ASN.1 BER/DER Tag-Length-Value (TLV) 二进制编解码标准库 |
| [`cap:ulid`](./ulid.md) | `[Pure-CP]` | 工业级 ULID 编解码与单调生成器 |
| [`cap:uuid`](./uuid.md) | `[Pure-CP]` | cap:uuid 是 CP 语言官方内置的纯原生分布式唯一标识符引擎，支持 RFC 9562 UUID (v4 / v5 SHA-1 / v7 时间单调有序)、128 位 Crockford Base32 ULID 与  |
| [`cap:zlib`](./zlib.md) | `[Pure-CP]` | cap:zlib 提供纯 CP 实现的工业级 Zlib (RFC 1950)、Gzip (RFC 1952) 与 Raw Deflate (RFC 1951) 统一压缩、解压与校验能力，零外部 C 依赖，内置防压缩炸弹（ |

## 8. 文本、文档格式与模板引擎

*Text, Document Formats & Templating* — 共 **33** 个模块（`[Bun]`: 27，`[Pure-CP]`: 6）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:ansi`](./ansi.md) | `[Bun]` | Unified ANSI Terminal Styling, SGR, Hyperlinks & Stripping |
| [`cap:charset_label`](./charset_label.md) | `[Bun]` | WHATWG Encoding 字符集标签规范化与 BOM 嗅探库 |
| [`cap:cow_str`](./cow_str.md) | `[Bun]` | 零拷贝借用 / 原子引用计数写时复制字符串 `CowStr` |
| [`cap:csv`](./csv.md) | `[Bun]` | cap:csv 是 CP 语言官方内置的工业生产级纯 CP 表格数据交换与流式处理标准库，零外部依赖、零 std.posix、零 @import("bun")，既可通过 import { ... } from "cap: |
| [`cap:frontmatter`](./frontmatter.md) | `[Bun]` | Markdown YAML/TOML Frontmatter 零分配提取器 |
| [`cap:grapheme_cluster`](./grapheme_cluster.md) | `[Bun]` | Unicode UAX #29 扩展字素簇边界判定与 UTF-8 迭代器 |
| [`cap:human_fmt`](./human_fmt.md) | `[Bun]` | 人类可读格式化、敏感 URL 脱敏与 CLI 表格渲染库 |
| [`cap:ini`](./ini.md) | `[Bun]` | cap:ini 是 CP 语言官方内置的纯 CP INI / Systemd Unit (.service) / .gitconfig 解析与增量编辑引擎，零外部依赖、零 std.posix、零 @import("bun |
| [`cap:jmespath`](./jmespath.md) | `[Pure-CP]` | JMESPath JSON 查询与投影转换标准库 |
| [`cap:jq`](./jq.md) | `[Pure-CP]` | 纯 CP JSON 结构化查询与管道转换引擎 |
| [`cap:json`](./json.md) | `[Bun]` | cap:json 是 CP 语言官方内置的高性能 JSON 数据交换标准库，对照 Bun 生产级数据交换模块（/data/dev/bun/src/interchange/）构建，实现 16 字节 SIMD 引号/转义/控 |
| [`cap:json5`](./json5.md) | `[Bun]` | 零分配 JSON5 1.0 词法器、校验器与标准 JSON 转换器 |
| [`cap:json5_lexer`](./json5_lexer.md) | `[Bun]` | 零堆分配 JSON5 词法扫描器与标准 JSON 规范化器 |
| [`cap:json_patch`](./json_patch.md) | `[Pure-CP]` | cap:json_patch（同时支持 import cap from "cap" 下的 cap.json_patch 动态挂载）提供纯 CP 原生实现的工业级 JSON 变更协议套件： |
| [`cap:jsonc`](./jsonc.md) | `[Bun]` | Zero-Alloc JSONC Comment & Trailing-Comma Stripper |
| [`cap:jsonpath`](./jsonpath.md) | `[Pure-CP]` | RFC 9535 JSONPath 标准库 |
| [`cap:line_offset_table`](./line_offset_table.md) | `[Bun]` | 基于二分查找的源码行列偏移表 |
| [`cap:logfmt`](./logfmt.md) | `[Bun]` | Structured `logfmt` Log Parser & Encoder |
| [`cap:logger`](./logger.md) | `[Bun]` | cap:logger 提供全零堆分配的工业级双引擎能力：编译器/源码级诊断引擎（源自 Bun src/logger）与 生产级结构化日志 & W3C Trace Context / OpenTelemetry 追踪采样引 |
| [`cap:markdown`](./markdown.md) | `[Bun]` | 纯 CP CommonMark / GFM 行分析、内联链接与引用定义解析库 |
| [`cap:md`](./md.md) | `[Bun]` | cap:md 是 CP 语言官方内置的高完整度 CommonMark + GitHub Flavored Markdown (GFM) 解析与多目标渲染标准库，直接原版移植自 Bun 生产级 MD4C 架构引擎（/dat |
| [`cap:slugify`](./slugify.md) | `[Bun]` | URL 安全 Slug 生成器与规范校验器 |
| [`cap:small_string`](./small_string.md) | `[Pure-CP]` | 纯 CP 8 字节短串内联 (SSO) 与去重驻留池（吸收自 Bun `install_types`） |
| [`cap:smol_str`](./smol_str.md) | `[Bun]` | 小字符串内联优化 Compact String |
| [`cap:string`](./string.md) | `[Bun]` | 工业级字符串引擎、Unicode 码表与可变缓冲标准库 |
| [`cap:text`](./text.md) | `[Bun]` | cap:text 是 CP 语言官方内置的纯原生文本处理与 AI Agent 上下文工程标准库，专为 RAG 文档切分、命令面板模糊搜索与 LLM 上下文窗口预算控制打造。包含三大核心子模块： |
| [`cap:textdecode`](./textdecode.md) | `[Bun]` | 零堆分配 WHATWG 流式文本解码器（TextDecoder） |
| [`cap:toml`](./toml.md) | `[Bun]` | cap:toml 是 CP 语言官方内置的纯 CP TOML v1.0.0 配置解析与序列化引擎，零外部依赖、零 std.posix、零 @import("bun")，全面覆盖 Cargo.toml、pyproject. |
| [`cap:tui`](./tui.md) | `[Pure-CP]` | cap:tui 是 CP 语言官方内置的纯原生终端用户界面（TUI）输入解析、弹性矩形布局与零闪烁差分渲染引擎，专为交互式 CLI AI Agent、状态仪表盘与全屏终端应用打造。全模块零 C 依赖、零 std.posi |
| [`cap:unicode`](./unicode.md) | `[Bun]` | Unicode / UTF-8 / UTF-16 / Grapheme 与终端显示宽度标准库 |
| [`cap:utf8_dfa`](./utf8_dfa.md) | `[Bun]` | 无分支 UTF-8 DFA 解码器与 ASCII 码点谓词 |
| [`cap:xml`](./xml.md) | `[Bun]` | cap:xml 是 CP 语言官方内置的纯 CP XML 1.0 协议解析与序列化标准库，零外部依赖、零 std.posix、零 @import("bun")，专为 S3 / RSS / Atom / SVG / JUn |
| [`cap:yaml`](./yaml.md) | `[Bun]` | cap:yaml 是 CP 语言官方内置的工业生产级纯 CP YAML 1.2 核心子集解析与序列化标准库，零 C 依赖、零 std.posix、零 @import("bun")，既可通过 import { ... }  |

## 9. 图像、音视频与多媒体图形

*Media, Audio, Video & 3D Graphics* — 共 **11** 个模块（`[Bun]`: 5，`[Pure-CP]`: 6）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:bmp`](./bmp.md) | `[Pure-CP]` | cap:bmp 是 CP 语言官方内置的纯原生 BMP（Windows Bitmap）位图编解码标准库，支持 24-bit BGR、32-bit BGRA 无损编码与解码，以及 8-bit 调色板索引位图与 top-do |
| [`cap:bounded_array`](./bounded_array.md) | `[Bun]` | 栈上定容动态数组 `BoundedArray<T, capacity>` |
| [`cap:color`](./color.md) | `[Pure-CP]` | cap:color 是 CP 语言官方内置的纯原生色彩科学、WCAG 2.1 无障碍对比度评估与终端 ANSI 配色标准库，面向 UI/浏览器渲染引擎、Design Token 调色板生成与 CLI 终端富文本主题系统打 |
| [`cap:exif`](./exif.md) | `[Bun]` | JPEG/TIFF EXIF 方向元数据读取器 |
| [`cap:gif`](./gif.md) | `[Pure-CP]` | cap:gif 是 CP 语言官方内置的纯原生 GIF87a / GIF89a 静态图与多帧动图编解码标准库，直接复用 cap:lzw（LzwBitOrder.LsbFirst）完成 GIF 子块 LZW 压缩与解压。 |
| [`cap:image`](./image.md) | `[Bun]` | cap:image 是 CP 语言官方内置的纯原生图像处理标准库，全面吸收自 Bun 图像处理子系统（/data/dev/bun/src/runtime/image/），集 魔数格式探测、多格式编解码（PNG/BMP/G |
| [`cap:image_probe`](./image_probe.md) | `[Bun]` | 零解码极速图片头探测器 |
| [`cap:media`](./media.md) | `[Pure-CP]` | cap:media 是 CP 语言官方内置的纯原生、零外部 C 依赖音视频与多媒体标准库，涵盖 音频编解码与 Voice AI DSP/VAD、主流音视频二进制容器解析（MP4/MKV/WebM/FLV/Ogg/EBML |
| [`cap:png`](./png.md) | `[Pure-CP]` | cap:png 是 CP 语言官方内置的纯原生 PNG（Portable Network Graphics，RFC 2083）图像编解码标准库，基于 cap:deflate（compressZlib、decompress |
| [`cap:thumbhash`](./thumbhash.md) | `[Bun]` | ThumbHash 超紧凑图片占位符编解码器 |
| [`cap:wav`](./wav.md) | `[Pure-CP]` | cap:wav 是 CP 语言官方内置的纯原生 RIFF/WAVE 内存字节流编解码与基础音频处理标准库，支持 Pcm8、Pcm16、Pcm24、Pcm32 与 Float32 全格式无损归一化读写、未知扩展块跳过（含奇 |

## 10. 编译原理、构建系统与开发者工具链

*Compilers, Build Systems & DevTools* — 共 **38** 个模块（`[Bun]`: 27，`[Pure-CP]`: 11）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:bench`](./bench.md) | `[Bun]` | cap:bench 整合自 Bun src/perf/ 性能剖析引擎与工业生产级微基准统计对比/内存追踪套件，全部核心运行期路径均采用零额外干扰与低开销布局： |
| [`cap:bundle_options`](./bundle_options.md) | `[Pure-CP]` | 纯 CP 打包格式、缓存策略与链接模式枚举引擎（吸收自 Bun `options_types`） |
| [`cap:bundler`](./bundler.md) | `[Bun]` | cap:bundler 是 CP 语言官方内置的模块打包器（Bundler）核心模型标准库，提炼自 Bun 原生 Bundler V2、Chunk 拓扑调度、CompileTarget 交叉编译目标解析与 Standal |
| [`cap:char_freq`](./char_freq.md) | `[Bun]` | 标识符字符频率统计与压缩短名生成器 |
| [`cap:clap`](./clap.md) | `[Bun]` | 编译期强类型与零分配流式命令行参数解析标准库 |
| [`cap:cli`](./cli.md) | `[Bun]` | cap:cli 提供零堆分配的完整终端交互工具链： |
| [`cap:command_tag`](./command_tag.md) | `[Pure-CP]` | 纯 CP CLI 子命令分类与配置加载策略引擎（吸收自 Bun `CommandTag`） |
| [`cap:compile_target`](./compile_target.md) | `[Bun]` | 跨平台构建与分发目标三元组解析库 |
| [`cap:compiler`](./compiler.md) | `[Pure-CP]` | cap:compiler 是 CP 语言官方内置的纯原生、零外部 C 依赖语言前端与虚拟机标准库，统一覆盖 安全表达式求值、DPLL 布尔可满足性 (SAT) 求解、Anti-ReDoS Pike VM 正则引擎、S-E |
| [`cap:coverage_options`](./coverage_options.md) | `[Pure-CP]` | 纯 CP 代码覆盖率配置、阈值判定与 LCOV 生成器（吸收自 Bun `CodeCoverageOptions`） |
| [`cap:docker`](./docker.md) | `[Bun]` | OCI / Docker 镜像引用与 Dockerfile 指令流式解析库 |
| [`cap:dwarf`](./dwarf.md) | `[Pure-CP]` | DWARF v4/v5 调试信息解析标准库 |
| [`cap:editorconfig`](./editorconfig.md) | `[Bun]` | 零堆分配 `.editorconfig` 解析与文件属性计算库 |
| [`cap:elf`](./elf.md) | `[Bun]` | 跨平台可执行二进制格式嗅探与完整 ELF64 / Mach-O / PE 解析标准库 |
| [`cap:env_path`](./env_path.md) | `[Bun]` | 跨平台 `PATH` 环境变量构建器与零分配迭代器 |
| [`cap:gfx`](./gfx.md) | `[Pure-CP]` | cap:gfx 将原本分散在根目录的 25 个纯 CP 图形与游戏引擎基础小模块统一收敛至 lib/gfx/，通过 lib/gfx/index.cp 提供零符号冲突的统一导出（import { ... } from "c |
| [`cap:git`](./git.md) | `[Bun]` | cap:git 是 CP 语言官方内置的零外部依赖 Git 底层原语库，专为 AI Coding Agent、CI 工具链与包管理器提供 Git 对象 SHA-1 内容寻址哈希与二进制编解码、完整 .gitignore  |
| [`cap:hosted_git_info`](./hosted_git_info.md) | `[Bun]` | 纯 CP 托管 Git 平台 URL 规范化解析与多格式生成器 |
| [`cap:js`](./js.md) | `[Bun]` | JavaScript 词法标识符与运算符元信息标准库 |
| [`cap:lcov_report`](./lcov_report.md) | `[Bun]` | 零堆分配 LCOV 代码覆盖率解析与序列化库 |
| [`cap:makefile_parse`](./makefile_parse.md) | `[Bun]` | GNU `Makefile` 目标、`.PHONY` 与变量零分配解析库 |
| [`cap:path`](./path.md) | `[Bun]` | 跨平台工业级路径解析与缓冲区池标准库 |
| [`cap:path_resolve`](./path_resolve.md) | `[Bun]` | 纯 CP 零堆分配 POSIX 路径规范化、拼接、相对路径、最长公共目录前缀与防穿越校验库。 |
| [`cap:paths`](./paths.md) | `[Pure-CP]` | cap:paths 是 CP 语言官方内置的纯 CP 跨平台路径解析、规范化与安全边界校验库，零 C 依赖、零 std.posix、零 @import("bun")。核心解析函数（parse、dirname、basena |
| [`cap:pkg`](./pkg.md) | `[Bun]` | CP 工业级包管理核心标准库文档 (`lib/pkg/`) |
| [`cap:resolver`](./resolver.md) | `[Bun]` | cap:resolver 是 CP 语言官方内置的高性能模块解析引擎标准库，完整吸收自 Bun src/resolver/（data_url.zig、package_json.zig、tsconfig_json.zig、 |
| [`cap:rpc`](./rpc.md) | `[Pure-CP]` | cap:rpc 是 CP 语言官方内置的纯 CP 协议引擎标准库（零 std.posix、零 @import("bun")），专为百万级 AI Agent 工具调用、LSP / DAP 语言服务器与分布式 RPC 打造。 |
| [`cap:semver`](./semver.md) | `[Bun]` | Semver 2.0.0 语义化版本解析与区间查询引擎 |
| [`cap:shell`](./shell.md) | `[Bun]` | 嵌入式跨平台 Shell 词法/AST 引擎与进程内/外混合管道执行器 |
| [`cap:sourcemap`](./sourcemap.md) | `[Bun]` | cap:sourcemap 是 CP 语言官方内置的工业级 SourceMap v3 (ECMA-426) 源码位置映射、Base64 VLQ 变长编解码、行偏移 $O(\log N)$ 二分定位、MappingList |
| [`cap:sourcemap_index`](./sourcemap_index.md) | `[Bun]` | 纯 CP 零堆分配 SourceMap v3 Base64-VLQ mappings 解码器与 O(log N) 二分查找定位器。 |
| [`cap:sse_parser`](./sse_parser.md) | `[Bun]` | WHATWG Server-Sent Events (`text/event-stream`) 解析与格式化库 |
| `cap:test` | `[Pure-CP]` | cap:test harness, per CHANGELOG |
| [`cap:test_runner`](./test_runner.md) | `[Bun]` | cap:test_runner 是零 JSC 耦合的纯 CP 生产级测试套件编排、快照管理、值格式化与 CI 报告引擎，完整直迁自 Bun src/test_runner/ 核心无 JS 引擎逻辑（Order.cp、Co |
| [`cap:tsconfig_paths`](./tsconfig_paths.md) | `[Bun]` | TSConfig `paths` 通配符映射与 `${configDir}` 展开器 |
| [`cap:wasm`](./wasm.md) | `[Pure-CP]` | cap:wasm 是 CP 语言官方内置的 WebAssembly 1.0 二进制模块解析、校验与构造标准库，专为沙盒插件内省、WASM 工具模块验证与二进制编码打造。零外部依赖、零 std.posix、零 @impor |
| [`cap:wasm_fmt`](./wasm_fmt.md) | `[Pure-CP]` | WebAssembly `.wasm` 二进制解析与构建标准库 |
| [`cap:which`](./which.md) | `[Bun]` | 跨平台可执行二进制查找与权限校验标准库 |

## 11. 数值计算、线性代数与微积分

*Numerical Analysis, Linear Algebra & Calculus* — 共 **5** 个模块（`[Bun]`: 3，`[Pure-CP]`: 2）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:bigint`](./bigint.md) | `[Bun]` | cap:bigint 为 CP 提供双轨高精度整数计算引擎： |
| [`cap:builtin_modules`](./builtin_modules.md) | `[Bun]` | 纯 CP Node.js 与 Bun 内置模块全量注册表与目标解析器 |
| [`cap:codec`](./codec.md) | `[Bun]` | cap:codec 是 CP 语言官方内置的纯 CP 多进制与校验编码标准库，零 C 依赖、零 std.posix、零 @import("bun")，既支持 import { ... } from "cap:codec" |
| [`cap:math`](./math.md) | `[Pure-CP]` | cap:math 是 CP 语言官方内置的纯原生高性能数值计算与 AI 向量检索算子库，覆盖： |
| [`cap:node_builtins`](./node_builtins.md) | `[Pure-CP]` | 纯 CP Node.js & Bun 内建模块映射注册表（吸收自 Bun `resolve_builtins`） |

## 12. 概率统计、机器学习与 AI Agent

*Statistics, Machine Learning & AI Agents* — 共 **10** 个模块（`[Bun]`: 1，`[Pure-CP]`: 9）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:a2a`](./a2a.md) | `[Pure-CP]` | Agent-to-Agent (A2A) 多智能体协作协议标准库 |
| [`cap:agent`](./agent.md) | `[Pure-CP]` | AI Agent 核心运行时标准库 |
| [`cap:agentmem`](./agentmem.md) | `[Pure-CP]` | cap:agentmem 是 CP 语言官方内置的纯原生、零 C 依赖的分层长期记忆引擎（Hierarchical Agent Memory），专为百万级 AI Agent 运行时的上下文记忆管理、经验巩固与记忆淘汰打造 |
| [`cap:attention`](./attention.md) | `[Pure-CP]` | Transformer Attention, KV-Cache & Token Sampler |
| [`cap:bpe`](./bpe.md) | `[Pure-CP]` | Byte-Pair Encoding (BPE) LLM Tokenizer & Token Budget Engine |
| [`cap:header_value_iter`](./header_value_iter.md) | `[Bun]` | RFC 9110 HTTP 多值头部与参数迭代器 |
| [`cap:prompt`](./prompt.md) | `[Pure-CP]` | LLM 上下文窗口管理与结构化输出提取引擎 |
| [`cap:quant`](./quant.md) | `[Pure-CP]` | AI Embedding 向量量化标准库 |
| [`cap:tensor`](./tensor.md) | `[Pure-CP]` | 工业级多维张量与 AI 算子标准库 |
| [`cap:vector`](./vector.md) | `[Pure-CP]` | cap:vector 是 CP 语言官方内置的纯原生本地语义向量检索与混合 RAG（Retrieval-Augmented Generation）搜索引擎库，专为百万级 AI Agent 本地记忆检索、知识库召回与混合重 |

## 14. 图论、计算几何与地理空间（GIS）

*Graphs, Computational Geometry & Geospatial* — 共 **3** 个模块（`[Bun]`: 0，`[Pure-CP]`: 3）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:geo`](./geo.md) | `[Pure-CP]` | cap:geo 是 CP 语言官方内置的纯原生地理空间计算与 Geohash 邻近检索标准库，专为 LBS 位置服务、地理围栏（Geofencing）、网格索引与半径/最近邻查询打造。全模块零 C 依赖、零 std.po |
| [`cap:geohash`](./geohash.md) | `[Pure-CP]` | cap:geohash 是 CP 语言官方内置的纯 CP Geohash 空间网格编码、8 邻域拓扑与 Haversine 球面距离标准库，零 C 依赖、零 std.posix、零 @import("bun")。 |
| [`cap:graph`](./graph.md) | `[Pure-CP]` | cap:graph 是 CP 语言官方内置的纯原生图论数据结构与算法库，同时提供： |

## 16. 其他通用工具与扩展模块

*Other Utilities & Extensions* — 共 **13** 个模块（`[Bun]`: 6，`[Pure-CP]`: 7）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:binfmt`](./binfmt.md) | `[Pure-CP]` | 二进制容器、可执行格式、科学数据与线协议序列化标准库 |
| [`cap:byte_size`](./byte_size.md) | `[Bun]` | 人类可读字节大小解析与 SI/IEC 格式化库 |
| [`cap:catalog`](./catalog.md) | `[Bun]` | 纯 CP 零分配 Monorepo 依赖目录协议与解析库 |
| [`cap:ci_info`](./ci_info.md) | `[Bun]` | CI/CD 运行环境与 PR 构建检测库 |
| [`cap:doc`](./doc.md) | `[Bun]` | 统一文档、出版、日程、联系人、订阅与流式表格标准库 |
| [`cap:ds`](./ds.md) | `[Pure-CP]` | cap:ds 是 CP 语言官方内置的纯原生高性能通用数据结构与算法集合，覆盖： |
| [`cap:finance`](./finance.md) | `[Pure-CP]` | cap:finance 是 CP 语言官方内置的纯原生零 C 依赖金融、计费、复式记账、商业规则与合规审计标准库，覆盖： |
| [`cap:iot`](./iot.md) | `[Pure-CP]` | 统一工业自动化、车联网、嵌入式固件、设备发现与链路遥测标准库 |
| [`cap:linear_fifo`](./linear_fifo.md) | `[Bun]` | 零堆分配泛型环形 FIFO 缓冲区 |
| [`cap:ml`](./ml.md) | `[Pure-CP]` | cap:ml 是 CP 语言官方内置的纯原生、零堆分配（Zero-Alloc）经典机器学习、聚类、降维、稳健回归、近似检索与序贯决策标准库。所有算法均以显式工作区或切片缓冲运行，无外部 C 依赖，专为边缘 AI Agen |
| [`cap:obs`](./obs.md) | `[Pure-CP]` | cap:obs 是 CP 语言官方内置的纯原生零 C 依赖可观测性、指标、安全日志、诊断报告与内核类型追踪标准库，覆盖： |
| [`cap:progress_tree`](./progress_tree.md) | `[Bun]` | 零分配层级进度树与节流渲染引擎 |
| [`cap:science`](./science.md) | `[Pure-CP]` | cap:science 是 CP 语言官方内置的零分配纯原生科学数据格式与工程控制标准库，覆盖： |
