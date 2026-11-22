# CP 标准库按领域分类总览目录 (`cap:*` Catalog)

> 本文档由 `scripts/gen_lib_catalog.py` 自动扫描 `lib/` 全部模块及溯源头生成，100% 覆盖无遗漏、无重复。
> 标签说明：`[Bun]` = 吸收自 `/data/dev/bun/src/` 的 Bun 衍生库；`[Pure-CP]` = 100% 纯 CP 原创零依赖标准库。

## 总体统计与领域分布

- **标准库模块总数**：**844** 个（`[Bun]` **161** 个 / `[Pure-CP]` **683** 个）

| 领域分类 | 英文域 | 模块数 | `[Bun]` | `[Pure-CP]` |
| :--- | :--- | :---: | :---: | :---: |
| [1. 核心运行时、并发与系统原语](#1-核心运行时并发与系统原语) | Core Runtime, Concurrency & OS | 57 | 32 | 25 |
| [2. 基础数据结构、容器与经典算法](#2-基础数据结构容器与经典算法) | Collections, Data Structures & Algorithms | 69 | 12 | 57 |
| [3. Web、HTTP、路由与微服务生态](#3-webhttp路由与微服务生态) | Web, HTTP, Routing & Microservices | 56 | 29 | 27 |
| [4. 网络协议、传输层与分布式通信](#4-网络协议传输层与分布式通信) | Networking, Protocols & Messaging | 95 | 7 | 88 |
| [5. 密码学、哈希与安全认证](#5-密码学哈希与安全认证) | Cryptography, Hashing & Security | 45 | 3 | 42 |
| [6. 存储、数据库、文件系统与可观测性](#6-存储数据库文件系统与可观测性) | Storage, Databases, Filesystems & Observability | 65 | 5 | 60 |
| [7. 压缩、归档与二进制编解码](#7-压缩归档与二进制编解码) | Compression, Archiving & Binary Codecs | 80 | 7 | 73 |
| [8. 文本、文档格式与模板引擎](#8-文本文档格式与模板引擎) | Text, Document Formats & Templating | 74 | 28 | 46 |
| [9. 图像、音视频与多媒体图形](#9-图像音视频与多媒体图形) | Media, Audio, Video & 3D Graphics | 80 | 5 | 75 |
| [10. 编译原理、构建系统与开发者工具链](#10-编译原理构建系统与开发者工具链) | Compilers, Build Systems & DevTools | 60 | 29 | 31 |
| [11. 数值计算、线性代数与微积分](#11-数值计算线性代数与微积分) | Numerical Analysis, Linear Algebra & Calculus | 36 | 2 | 34 |
| [12. 概率统计、机器学习与 AI Agent](#12-概率统计机器学习与-ai-agent) | Statistics, Machine Learning & AI Agents | 44 | 1 | 43 |
| [13. 信号处理、频谱分析与自动控制](#13-信号处理频谱分析与自动控制) | Signal Processing, Spectral Analysis & Control | 8 | 0 | 8 |
| [14. 图论、计算几何与地理空间（GIS）](#14-图论计算几何与地理空间gis) | Graphs, Computational Geometry & Geospatial | 37 | 0 | 37 |
| [15. 工控、硬件固件与垂直行业协议](#15-工控硬件固件与垂直行业协议) | Industrial, Embedded, Firmware & Domain Protocols | 38 | 1 | 37 |

---

## 1. 核心运行时、并发与系统原语

*Core Runtime, Concurrency & OS* — 共 **57** 个模块（`[Bun]`: 32，`[Pure-CP]`: 25）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| `cap:_cap` | `[Bun]` | cap:_cap 标准库模块 |
| `cap:actor` | `[Pure-CP]` | CP native actor runtime, per CHANGELOG |
| [`cap:aio`](./aio.md) | `[Bun]` | cap:aio 是 CP 语言官方内置的Linux 父进程退出看门狗（ParentDeathWatchdog）与零堆分配 POSIX 异步提交/完成环形队列（AioQueue<CAP>）标准库。吸收自 Bun（/data |
| `cap:beam` | `[Pure-CP]` | CP native Bun-BEAM bridge, per CHANGELOG |
| [`cap:bit_ops`](./bit_ops.md) | `[Bun]` | 纯 CP 零开销底层位运算、位域提取/插入、2 的幂对齐与置位迭代器库，吸收自 /data/dev/bun/src/meta/bits.cp。 |
| [`cap:bitset_simd`](./bitset_simd.md) | `[Pure-CP]` | 按字向量化加速动态位集合 |
| [`cap:bun_alloc`](./bun_alloc.md) | `[Pure-CP]` | 高性能自定义内存分配器套件 |
| [`cap:clap_stream`](./clap_stream.md) | `[Bun]` | 纯 CP 零堆分配流式命令行参数解析器，吸收自 /data/dev/bun/src/clap/streaming.cp。 |
| [`cap:conf`](./conf.md) | `[Bun]` | 统一配置交换引擎（YAML 1.2 / TOML 1.0 / INI / JSON5 & `ConfGetter`） |
| [`cap:crash_handler`](./crash_handler.md) | `[Pure-CP]` | cap:crash_handler（以及动态命名空间 cap.crash_handler）提供纯 CP 实现的工业级结构化崩溃诊断报告、栈帧符号格式化、零堆固定容量写入器与环形面包屑缓冲区。设计目标是在信号处理与异常现场 |
| [`cap:cron`](./cron.md) | `[Bun]` | cap:cron 是 CP 语言官方内置的纯 CP 5/6 字段 Cron 表达式解析与 UTC 触发时间计算库，零 C 依赖、零 std.posix、零 @import("bun")。内部采用位掩码（bitmask）紧 |
| [`cap:di`](./di.md) | `[Pure-CP]` | cap:di 是 CP 语言官方内置的纯 CP 依赖注入（IoC）与生命周期管理服务容器，零外部依赖、零 std.posix、零 @import("bun")，专为大型后端服务、CLI 工具与 AI Agent 插件系统 |
| [`cap:dotenv`](./dotenv.md) | `[Bun]` | 零分配 `.env` 环境变量解析、变量插值与多环境级联加载器 |
| [`cap:dotenv_parse`](./dotenv_parse.md) | `[Bun]` | 零堆分配 `.env` 解析器与变量展开器 |
| [`cap:errno`](./errno.md) | `[Bun]` | cap:errno 是 CP 语言官方内置的跨平台操作系统错误码标准库，吸收自 Bun 的系统错误码体系（/data/dev/bun/src/errno/{linux_errno,darwin_errno,windows |
| [`cap:errno_map`](./errno_map.md) | `[Bun]` | 纯 CP 跨平台 POSIX/OS Errno 统一映射与元数据查询库 |
| [`cap:event`](./event.md) | `[Bun]` | cap:event（别名 cap:events、cap:event_loop）统一整合 CP 语言的应用层事件发布/订阅模型（EventBus、EventEmitter、协程信号 Event）与底层高性能异步事件循环内核 |
| [`cap:fallback_alloc`](./fallback_alloc.md) | `[Bun]` | 栈内联缓冲优先 + 堆回退分配器 |
| [`cap:feature`](./feature.md) | `[Pure-CP]` | cap:feature 是 CP 语言官方内置的纯 CP Feature Flags 功能开关、确定性灰度分桶（Consistent Bucketing）、属性靶向规则引擎与多变体 A/B/n 实验分流引擎，零外部依赖、 |
| [`cap:feature_flags`](./feature_flags.md) | `[Pure-CP]` | 纯 CP 特性开关与环境变量强类型解析引擎（吸收自 Bun `bun_core`） |
| [`cap:ffi`](./ffi.md) | `[Bun]` | POSIX 动态库加载、TinyCC 内存编译与 C-ABI 布局计算标准库 |
| [`cap:fs`](./fs.md) | `[Pure-CP]` | cap:fs 是 CP 语言官方内置的高阶文件系统标准库，采用 双模编译期分发（@is_nest） 架构：在 BEAM/Nest 分布式调度模式下，异步文件操作通过 SystemAction.FS_* 挂起当前协程并委托 |
| [`cap:fsm`](./fsm.md) | `[Pure-CP]` | 确定性有限状态机、守卫、拓扑分析与熔断/DAG 引擎 |
| [`cap:health`](./health.md) | `[Pure-CP]` | cap:health 是 CP 语言官方内置的纯 CP 云原生健康检查探针汇聚（Liveness / Readiness / Startup）与优雅停机生命周期协调器（ShutdownCoordinator），零 C 依 |
| [`cap:io`](./io.md) | `[Bun]` | cap:io 是 CP 语言官方内置的统一流式 I/O 标准库，基于 Reader / Writer / Closer / Seeker 四大解耦 Trait 契约，完整移植 Bun src/io/ 的生产级流式与非阻塞 |
| [`cap:lock`](./lock.md) | `[Pure-CP]` | cap:lock 是 CP 语言官方内置的纯 CP 逻辑协调与并发控制标准库，零 C 依赖、零 std.posix、零 @import("bun")，专为多 Agent 工具互斥访问、状态分区并发控制与分布式租约协调打造 |
| [`cap:lockfree_queue`](./lockfree_queue.md) | `[Bun]` | 无锁并发环形队列、侵入式无界队列与有界通道 |
| [`cap:mem`](./mem.md) | `[Bun]` | cap:mem 是 CP 语言官方内置的高性能内存分配器（MaxHeapAllocator、StackFallbackAllocator、BumpArena、Arena）、Mimalloc C-FFI 底层绑定、跨 Ac |
| [`cap:meta`](./meta.md) | `[Bun]` | cap:meta 是 CP 语言官方内置的编译期类型内省（Type Traits）与无分支位操作标准库，吸收自 Bun src/meta/（bits.zig、traits.zig、meta.zig）。该库向编译器工具链、 |
| [`cap:napi`](./napi.md) | `[Bun]` | cap:napi 提供与 Node.js / Bun 完全一致的 Node-API v9 C-ABI 类型布局、完整 24 项状态码与错误消息映射，以及纯 CP 可离线测试的运行时上下文 NapiRuntimeConte |
| [`cap:object_pool`](./object_pool.md) | `[Bun]` | 侵入式单链表空闲对象池 |
| [`cap:padding_checker`](./padding_checker.md) | `[Bun]` | 编译期结构体零未初始化填充校验器 |
| `cap:parallel` | `[Pure-CP]` | CP native async/parallel runtime, per CHANGELOG |
| [`cap:pipeline`](./pipeline.md) | `[Pure-CP]` | cap:pipeline 是 CP 语言官方内置的纯 CP 可组合洋葱模型（Onion Middleware）与多阶段数据处理流水线引擎，零 C 依赖、零 std.posix、零 @import("bun")，专为 We |
| [`cap:pool`](./pool.md) | `[Pure-CP]` | cap:pool 是 CP 语言官方内置的纯 CP 通用有界资源与连接池引擎（ResourcePool），零 C 依赖、零 std.posix、零 @import("bun")，专为数据库连接、HTTP 长连接客户端、g |
| [`cap:ptr`](./ptr.md) | `[Bun]` | cap:ptr 是 CP 语言官方内置的零 GC 高性能智能指针、侵入式/非侵入式引用计数、写时复制（Copy-on-Write）与 64 位打标指针（Tagged Pointer）标准库。完整吸收自 Bun（/data |
| [`cap:queue`](./queue.md) | `[Pure-CP]` | cap:queue 是 CP 语言官方内置的纯原生后台任务队列与死信重试调度引擎，专为百万级 AI Agent 异步任务编排、优先级调度、精确延迟执行、Worker 租约超时回收与死信队列（DLQ）容错打造。 |
| [`cap:rate`](./rate.md) | `[Bun]` | Zero-Alloc Rate Limiting Suite |
| [`cap:ratelimit`](./ratelimit.md) | `[Pure-CP]` | cap:ratelimit 是 CP 语言官方内置的纯 CP 高性能确定性限流算法库，零 C 依赖、零 std.posix、零 @import("bun")。所有时间判定均通过显式单调毫秒时间戳 now_ms: i64  |
| `cap:reflect` | `[Pure-CP]` | CP native stdlib |
| [`cap:retry_backoff`](./retry_backoff.md) | `[Bun]` | - none：确定性纯指数退避 min(base * 2^attempt, max) |
| [`cap:rule`](./rule.md) | `[Pure-CP]` | cap:rule 是 CP 语言官方内置的纯 CP 声明式生产规则引擎（Production Rule Engine），零 C 依赖、零 std.posix、零 @import("bun")，为业务风控反欺诈、动态定价路 |
| [`cap:safety`](./safety.md) | `[Bun]` | cap:safety 是 CP 语言官方内置的线程归属校验（ThreadLock）、可重入读写竞态检测（CriticalSection）、非托管容器分配器一致性守卫（CheckedAllocator）、AddressSa |
| [`cap:simd`](./simd.md) | `[Bun]` | Highway 级 SIMD 硬件向量加速与高速字节扫描标准库 |
| [`cap:slab_pool`](./slab_pool.md) | `[Pure-CP]` | 工业级固定块 Slab 对象池 |
| [`cap:stream`](./stream.md) | `[Bun]` | cap:stream 是 CP 语言面向流式数据处理场景的标准门面模块（Facade），直接与底层 cap:io 共享零开销的 Trait 契约与实现（无任何中间适配器包装损耗），确保通过 cap:stream 与 ca |
| [`cap:supervisor`](./supervisor.md) | `[Pure-CP]` | OTP 监督树状态机与分布式弹性原语 |
| [`cap:sync`](./sync.md) | `[Bun]` | cap:sync（与底层 cap:threading 共享同步原语）是 CP 语言专为多核低延迟并发、用户态快速锁（Futex）、无锁队列（Lock-Free MPMC / Unbounded Batch Queue）、 |
| [`cap:sys`](./sys.md) | `[Bun]` | cap:sys 是 CP 语言官方内置的底层系统调用与操作系统原语标准库，深度吸收 Bun 生产级 src/sys/ 实现，并完全基于 Zig 0.17 原生 std.c.* 与 std.os.linux.* 构建（零  |
| [`cap:tagged_ptr`](./tagged_ptr.md) | `[Bun]` | 64 位高位标签指针与紧凑标签联合体 |
| [`cap:threading`](./threading.md) | `[Pure-CP]` | cap:threading（源码位于 lib/threading/）是纯 CP 实现的工业级并发同步原语标准库，完全基于 std.atomic.Value 与 std.Thread.Futex 构建，零堆分配、无 std |
| [`cap:time`](./time.md) | `[Bun]` | cap:time 是 CP 语言官方内置的工业生产级纯 CP 时间、日历转换、Cron 调度与限流退避标准库，零外部依赖、零 std.posix、零 @import("bun")、零堆分配，适用于 BEAM 调度器、分布 |
| [`cap:timeseries`](./timeseries.md) | `[Pure-CP]` | cap:timeseries 是 CP 语言官方内置的纯 CP 时序数据存储、时间对齐 Rollup 降采样、滑动平均与异常尖峰检测引擎，零 C 依赖、零 std.posix、零 @import("bun")，为系统遥测 |
| [`cap:type_traits`](./type_traits.md) | `[Bun]` | 编译期类型特征自省与指针元数据工具 |
| [`cap:tz`](./tz.md) | `[Pure-CP]` | cap:tz 是 CP 语言官方内置的纯 CP、零 C 依赖 时区转换与夏令时（DST）规则引擎（零 std.posix、零 @import("bun")），专为全球化后端、日程排期（配合 cap:ical / cap: |
| [`cap:watcher`](./watcher.md) | `[Bun]` | 跨平台文件系统监控、路径过滤与事件合并防抖引擎 |
| `cap:worker` | `[Pure-CP]` | CP native worker runtime, per CHANGELOG |

## 2. 基础数据结构、容器与经典算法

*Collections, Data Structures & Algorithms* — 共 **69** 个模块（`[Bun]`: 12，`[Pure-CP]`: 57）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:aho`](./aho.md) | `[Pure-CP]` | Aho-Corasick 多模式字符串匹配自动机 |
| [`cap:aho_corasick`](./aho_corasick.md) | `[Pure-CP]` | 纯 CP Aho-Corasick 多模式串匹配自动机 |
| [`cap:bitset`](./bitset.md) | `[Pure-CP]` | cap:bitset 是 CP 语言官方内置的纯 CP 64-bit 字级动态位图（BitSet）与零堆分配栈上固定位图（FixedBitSet(N)） 引擎，零 C 依赖、零 std.posix、零 @import(" |
| [`cap:bloom_cuckoo`](./bloom_cuckoo.md) | `[Pure-CP]` | 概率集合数据结构标准库 |
| [`cap:bplus`](./bplus.md) | `[Pure-CP]` | cap:bplus 是 CP 语言官方内置的纯 CP B+ 树（B+ Tree）有序键值与叶子链表范围扫描引擎，零 C 依赖、零 std.posix、零 @import("bun")。采用索引池（ArrayList(BP |
| [`cap:bplus_tree`](./bplus_tree.md) | `[Pure-CP]` | 工业级 B+ 树索引与 4KB Slotted Page 标准库 |
| [`cap:bptree`](./bptree.md) | `[Pure-CP]` | 工业级内存有序索引 B+ 树 |
| [`cap:cache`](./cache.md) | `[Pure-CP]` | cap:cache 是 CP 语言官方内置的纯原生高性能内存缓存与概率数据结构库，专为 AI Agent 响应缓存、Prompt/Embedding 缓存、高频去重与热键频次统计打造。包含三大核心组件： |
| [`cap:circ_buf`](./circ_buf.md) | `[Pure-CP]` | 纯 CP 泛型环形缓冲区 (Circular Ring Buffer) |
| [`cap:col`](./col.md) | `[Bun]` | cap:col（兼容映射 cap:collections）是 CP 语言专为百万级 AI Agent 并发、确定性无 GC 内存管理与 CPU 缓存行局部性（Cache-Line Locality）设计的工业级数据结构标 |
| [`cap:collections`](./collections.md) | `[Pure-CP]` | 工业级基础数据结构集合标准库 |
| [`cap:crdt`](./crdt.md) | `[Pure-CP]` | cap:crdt 是 CP 语言官方内置的纯原生状态型无冲突复制数据类型（State-Based CRDT / CvRDT）标准库，专为多 Agent 分布式最终一致性状态同步、离线协作与无中心冲突收敛打造。全模块严格满 |
| [`cap:crdt_pn`](./crdt_pn.md) | `[Pure-CP]` | State-Based CRDTs |
| [`cap:cuckoo`](./cuckoo.md) | `[Pure-CP]` | cap:cuckoo 是 CP 语言官方内置的纯 CP 布谷鸟过滤器（Cuckoo Filter），零 C 依赖、零 std.posix、零 @import("bun")，专为缓存防穿透、去重黑名单、限流白名单与分布式路 |
| [`cap:diff`](./diff.md) | `[Bun]` | cap:diff 是 CP 语言官方内置的纯 CP 文本差异分析与代码合并标准库，零外部依赖、零 std.posix、零 @import("bun")，专为 CP 代码工具链与 AI Coding Agent 设计。既可 |
| [`cap:diff3`](./diff3.md) | `[Pure-CP]` | cap:diff3 是 CP 语言官方内置的纯 CP 三方文本合并（Three-Way Text Merge）与冲突消解标准库，零外部依赖、零 std.posix、零 @import("bun")。基于 Myers $O |
| [`cap:disjoint_set`](./disjoint_set.md) | `[Pure-CP]` | 纯 CP 并查集 (Union-Find / Disjoint Set Union) |
| [`cap:dsu`](./dsu.md) | `[Pure-CP]` | cap:dsu 是 CP 语言官方内置的纯 CP 并查集家族（DisjointSet、WeightedDsu、RollbackDsu） 引擎，零 C 依赖、零 std.posix、零 @import("bun")，专为图 |
| [`cap:exact_matcher`](./exact_matcher.md) | `[Bun]` | 定长整型打包 $O(1)$ 字符串快速匹配器 |
| [`cap:fenwick`](./fenwick.md) | `[Pure-CP]` | 纯 CP 树状数组 (Fenwick / Binary Indexed Tree) |
| [`cap:fuzzy`](./fuzzy.md) | `[Pure-CP]` | 模糊字符串匹配、编辑距离、Jaro-Winkler 相似度与排序检索 |
| [`cap:glob`](./glob.md) | `[Bun]` | 高性能通配符状态机与目录扫描引擎 |
| [`cap:glob_escape`](./glob_escape.md) | `[Bun]` | Glob 通配符转义、静态前缀提取与否定模式分析库 |
| [`cap:globset`](./globset.md) | `[Pure-CP]` | cap:globset 是 CP 语言官方内置的纯 CP 多模式通配符与忽略规则匹配库，零 C 依赖、零 std.posix、零 @import("bun")。底层采用线性时间 NFA 状态集模拟（零堆分配匹配、免疫指数 |
| [`cap:histogram`](./histogram.md) | `[Pure-CP]` | cap:histogram 是 CP 语言官方内置的纯 CP 工业级高动态范围延迟直方图（HDR Histogram）与流式速率统计库，零 C 依赖、零 std.posix、零 @import("bun")。基于 Gil |
| [`cap:hive_array`](./hive_array.md) | `[Bun]` | type, comptime capacity: u16): type` |
| [`cap:hll`](./hll.md) | `[Pure-CP]` | cap:hll 是 CP 语言官方内置的纯 CP HyperLogLog 基数估算与分布式可合并草图库，零 C 依赖、零 std.posix、零 @import("bun")。采用强混合 64-bit SplitMix6 |
| [`cap:hyperloglog`](./hyperloglog.md) | `[Pure-CP]` | 纯 CP HyperLogLog 基数估算引擎 |
| [`cap:interval_tree`](./interval_tree.md) | `[Pure-CP]` | 纯 CP 增强区间树 (Interval Tree) |
| [`cap:intrusive_heap`](./intrusive_heap.md) | `[Bun]` | 零堆分配侵入式配对堆 (Intrusive Pairing Heap) |
| [`cap:kmp`](./kmp.md) | `[Pure-CP]` | 零分配线性时间字符串匹配与回文算法库（KMP / Z-Algorithm / Manacher） |
| [`cap:kmp_z_algo`](./kmp_z_algo.md) | `[Pure-CP]` | 纯 CP KMP / Z-Algorithm / Manacher 线性字符串算法库 |
| [`cap:known_globals`](./known_globals.md) | `[Bun]` | 纯 CP ECMAScript / Web / Node / Bun 已知全局符号与无副作用判定库 |
| [`cap:lfu`](./lfu.md) | `[Pure-CP]` | 零堆分配 LFU 缓存与 W-TinyLFU 4-bit Count-Min Sketch 准入过滤器 |
| [`cap:lru`](./lru.md) | `[Pure-CP]` | cap:lru 是 CP 语言官方内置的纯原生 $O(1)$ LRU 内存缓存库，结合 std.StringHashMapUnmanaged(usize) 与连续槽位池上的侵入式双向链表，支持： |
| [`cap:lru_k`](./lru_k.md) | `[Pure-CP]` | 抗扫描污染 LRU-K 缓存替换引擎 |
| [`cap:lsh`](./lsh.md) | `[Pure-CP]` | 零堆分配局部敏感哈希（LSH）近似最近邻检索库 |
| [`cap:merkle`](./merkle.md) | `[Pure-CP]` | cap:merkle 是 CP 语言官方内置的纯 CP RFC 6962 Merkle 哈希树引擎，零 C 依赖、零 std.posix、零 @import("bun")，基于 cap:crypto 与 std.cryp |
| [`cap:merkle_tree`](./merkle_tree.md) | `[Pure-CP]` | 密码学 SHA-256 Merkle Tree 标准库 |
| [`cap:minhash`](./minhash.md) | `[Pure-CP]` | Zero-Allocation MinHash & LSH Banding Library |
| [`cap:myers_diff`](./myers_diff.md) | `[Bun]` | Myers $O(ND)$ 最短编辑脚本 Diff 引擎 |
| [`cap:nfa_regex`](./nfa_regex.md) | `[Pure-CP]` | Thompson NFA 线性时间无回溯正则表达式标准库 |
| [`cap:patch`](./patch.md) | `[Bun]` | cap:patch 是 CP 语言官方内置的行级 Myers $O(ND)$ 文本差分引擎、完整多文件/多 Hunk Unified Diff 解析器、支持精确/偏移/上下文模糊容错（Fuzz/Offset）的补丁应用引 |
| [`cap:pqueue`](./pqueue.md) | `[Pure-CP]` | 索引优先队列、有界 Top-K 与流式双堆中位数 |
| [`cap:radix`](./radix.md) | `[Pure-CP]` | cap:radix 是 CP 语言官方内置的纯 CP 压缩基数树（Compressed Patricia Radix Tree）与分段前缀路由引擎，零 C 依赖、零 std.posix、零 @import("bun")。 |
| [`cap:regex`](./regex.md) | `[Pure-CP]` | cap:regex 是 CP 语言官方内置的纯 CP、零 C 依赖的 抗 ReDoS 线性时间正则表达式引擎。底层采用 Thompson NFA 字节码编译 + Pike VM 同步多状态步进（带稀疏集合/代数去重），从 |
| [`cap:ringbuf`](./ringbuf.md) | `[Pure-CP]` | cap:ringbuf 是 CP 语言官方内置的纯 CP 有界环形缓冲区（RingBuffer<T>）、高性能字节环形流管道（ByteRing）与条数/时间双触发微批聚合器（MicroBatcher<T>），零 C 依赖 |
| [`cap:ringhash`](./ringhash.md) | `[Pure-CP]` | cap:ringhash 是 CP 语言官方内置的纯 CP 一致性哈希与分布式路由算法库，零 C 依赖、零 std.posix、零 @import("bun")，专为分布式缓存分片、有状态服务路由、多副本容灾与无状态负载 |
| [`cap:roaring`](./roaring.md) | `[Pure-CP]` | cap:roaring 是 CP 语言官方内置的纯 CP Roaring Bitmap（压缩 32 位无符号整数集合） 引擎，零 C 依赖、零 std.posix、零 @import("bun")。按高 16 位 key |
| [`cap:roaring_bitmap`](./roaring_bitmap.md) | `[Pure-CP]` | 工业级 Roaring Bitmap 压缩位图集合引擎 |
| [`cap:rope`](./rope.md) | `[Pure-CP]` | Piece Table / Rope 大文本编辑器数据结构与行号映射标准库 |
| [`cap:rope_text`](./rope_text.md) | `[Pure-CP]` | 平衡二叉树大文本编辑器缓冲库 |
| [`cap:search`](./search.md) | `[Pure-CP]` | cap:search 是 CP 语言官方内置的纯原生全文检索与自动补全搜索引擎库，专为本地知识库、CLI 检索与 AI RAG 召回打造： |
| [`cap:segment_tree`](./segment_tree.md) | `[Pure-CP]` | 纯 CP 懒传播线段树 (Lazy Segment Tree) |
| [`cap:segtree`](./segtree.md) | `[Pure-CP]` | cap:segtree 是 CP 语言官方内置的纯 CP 树状数组（FenwickTree）与惰性传播线段树（LazySegmentTree） 区间查询引擎，零 C 依赖、零 std.posix、零 @import("b |
| [`cap:simhash`](./simhash.md) | `[Pure-CP]` | 64-bit Charikar SimHash 近似去重指纹与 4×16-bit LSH 分段库 |
| [`cap:sketch`](./sketch.md) | `[Pure-CP]` | cap:sketch 是 CP 语言官方内置的纯 CP 流式概率素描算法套件，零 C 依赖、零 std.posix、零 @import("bun")，为大规模遥测监控、实时 UV 去重统计、流式延迟分位数观测与无偏流式抽 |
| [`cap:skip_list`](./skip_list.md) | `[Pure-CP]` | 工业级有序跳表键值引擎 |
| [`cap:skiplist`](./skiplist.md) | `[Pure-CP]` | cap:skiplist 是 CP 语言官方内置的纯 CP 泛型有序跳表 MemTable 与范围扫描索引（Ordered SkipList MemTable & Range Scan Index），零 C 依赖、零 s |
| [`cap:sort`](./sort.md) | `[Bun]` | 工业级自适应排序（PDQSort / BlockSort / HeapSort）与二分检索标准库 |
| [`cap:static_hashmap`](./static_hashmap.md) | `[Bun]` | 编译期/零分配静态哈希表 |
| [`cap:suffix`](./suffix.md) | `[Pure-CP]` | 零堆分配后缀数组与 Kasai LCP 字符串索引库 |
| [`cap:suffix_array`](./suffix_array.md) | `[Pure-CP]` | 后缀数组与 Kasai LCP 标准库 |
| [`cap:suffixarray`](./suffixarray.md) | `[Pure-CP]` | 后缀数组、Kasai LCP 与子串索引引擎 |
| [`cap:tdigest`](./tdigest.md) | `[Pure-CP]` | 纯 CP 流式 T-Digest 分位数估算引擎 |
| [`cap:treap`](./treap.md) | `[Pure-CP]` | 零分配随机平衡树 / 顺序统计 Treap 库 |
| [`cap:trie_radix`](./trie_radix.md) | `[Pure-CP]` | cap:trie_radix（支持 import cap from "cap" 下的 cap.trie_radix 动态挂载）提供纯 CP 实现的工业级路径压缩基数树（Compressed Radix Tree / Pa |
| [`cap:xorfilter`](./xorfilter.md) | `[Pure-CP]` | cap:xorfilter 是 CP 语言官方内置的纯 CP 静态 Xor 过滤器（Xor8 / Xor16 Static Minimal-Space Approximate Set Membership Filter） |
| [`cap:zset`](./zset.md) | `[Pure-CP]` | cap:zset 是 CP 语言官方内置的纯 CP SkipList 跳表（带 span 跨度）+ StringHashMapUnmanaged(f64) 双索引有序集合引擎（类 Redis ZSet），零 C 依赖、零 |

## 3. Web、HTTP、路由与微服务生态

*Web, HTTP, Routing & Microservices* — 共 **56** 个模块（`[Bun]`: 29，`[Pure-CP]`: 27）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:alt_svc`](./alt_svc.md) | `[Bun]` | 纯 CP 零堆分配 RFC 7838 HTTP Alt-Svc 头部解析器与 TTL 缓存（用于 HTTP/3 升级发现），吸收自 /data/dev/bun/src/http/h3_client/AltSvc.cp。 |
| [`cap:atom`](./atom.md) | `[Pure-CP]` | 纯 CP RFC 4287 Atom 1.0 XML Feed 零分配解析与序列化库 |
| [`cap:autolink`](./autolink.md) | `[Bun]` | GFM 宽松自动链接（URL / WWW / Email）扫描器 |
| [`cap:content_range`](./content_range.md) | `[Bun]` | 纯 CP 零堆分配 RFC 9110 §14 HTTP Range 请求头与 Content-Range 响应头解析/格式化库（用于 206 Partial Content 与 416 Range Not Satisfi |
| [`cap:cors_policy`](./cors_policy.md) | `[Bun]` | WHATWG Fetch CORS 策略与预检评估引擎 |
| [`cap:css`](./css.md) | `[Bun]` | cap:css 是 CP 语言官方内置的工业级 CSS 解析、词法分析、颜色空间与数学求值、选择器权重匹配、变量解析、厂商前缀/逻辑属性展开、CSS 嵌套展平与样式表压缩/美化标准库。整体架构吸收自 Bun 内置的 Li |
| [`cap:data_url`](./data_url.md) | `[Bun]` | 纯 CP 零堆分配 RFC 2397 Data URL（data:[<mediatype>][;base64],<data>）解析、解码与最短编码器，吸收自 /data/dev/bun/src/resolver/data |
| [`cap:etag`](./etag.md) | `[Bun]` | 纯 CP 零堆分配 RFC 9110 / RFC 7232 HTTP ETag 解析、强/弱比较、If-None-Match / If-Match 条件请求求值与 FNV-1a 64-bit 内容 ETag 生成库，吸收 |
| [`cap:exe_format`](./exe_format.md) | `[Pure-CP]` | 可执行二进制格式零拷贝探测与解析库 |
| [`cap:feed`](./feed.md) | `[Pure-CP]` | cap:feed 是 CP 语言官方内置的纯 CP 聚合订阅源解析与生成标准库，零外部 C 依赖、零 std.posix、零 @import("bun")，基于 cap:xml 与 cap:json 构建，专为 AI A |
| [`cap:fs_router`](./fs_router.md) | `[Bun]` | 零堆分配文件系统路由解析与匹配引擎 |
| [`cap:gemini`](./gemini.md) | `[Pure-CP]` | Gemini Protocol v0.16.1 与 Gemtext 零分配编解码库 |
| [`cap:gopher`](./gopher.md) | `[Pure-CP]` | RFC 1436 Gopher 协议与 Menu 目录零分配编解码库 |
| [`cap:graphql`](./graphql.md) | `[Pure-CP]` | cap:graphql 是 CP 语言官方内置的纯原生 GraphQL（October 2021 规范）可执行文档词法扫描、递归下降 AST 解析、多维安全护栏与规范化/压缩打印引擎，专为 API 网关、BFF（Back |
| [`cap:grpc_wire`](./grpc_wire.md) | `[Pure-CP]` | gRPC over HTTP/2 线协议标准库 |
| [`cap:h2_encode`](./h2_encode.md) | `[Bun]` | 零堆分配 HTTP/2 出站请求与流控帧编码库 |
| [`cap:h2_frame`](./h2_frame.md) | `[Bun]` | HTTP/2 (RFC 9113) 线协议帧头、设置与优先级编解码器 |
| [`cap:har`](./har.md) | `[Pure-CP]` | W3C HTTP Archive (`.har`) 1.2 零分配解析与性能统计库 |
| [`cap:hpack`](./hpack.md) | `[Pure-CP]` | RFC 7541 HTTP/2 HPACK 头部压缩编解码器 |
| [`cap:html`](./html.md) | `[Bun]` | cap:html 是 CP 语言官方内置的工业生产级 HTML5 解析、内容安全清洗与 AI Agent 网页语义提取标准库，对标 Bun 的 HTMLRewriter（/data/dev/bun/src/lolhtml |
| [`cap:html_asset_scanner`](./html_asset_scanner.md) | `[Bun]` | 零堆分配 HTML 资源引用扫描器 |
| [`cap:http`](./http.md) | `[Bun]` | cap:http 是 CP 语言官方内置的全栈工业级 HTTP 协议子系统，完整吸收自 Bun（/data/dev/bun/src/http/、/data/dev/bun/src/http_types/、/data/de |
| [`cap:http2`](./http2.md) | `[Pure-CP]` | RFC 9113 HTTP/2 二进制分帧、流状态与流控引擎 |
| [`cap:http2_frame`](./http2_frame.md) | `[Pure-CP]` | RFC 9113 HTTP/2 二进制帧与多路复用状态机 |
| [`cap:http3`](./http3.md) | `[Pure-CP]` | RFC 9114 HTTP/3 二进制分帧、流类型与 RFC 9204 QPACK 编解码引擎 |
| [`cap:jwt`](./jwt.md) | `[Bun]` | RFC 7519 JWT Claims Zero-Allocation Parser |
| [`cap:mcp`](./mcp.md) | `[Pure-CP]` | Model Context Protocol (MCP 2025-03-26) 标准库 |
| [`cap:mime`](./mime.md) | `[Bun]` | cap:mime 是 CP 语言官方内置的纯 CP RFC 6838 MediaType 解析/格式化、RFC 9110 HTTP Accept 内容协商（Content Negotiation）与全量扩展名双向数据库， |
| [`cap:multipart`](./multipart.md) | `[Pure-CP]` | RFC 7578 `multipart/form-data` 解析、边界提取与构建库 |
| [`cap:netrc`](./netrc.md) | `[Bun]` | - machine: ([]const u8)?：主机名（default 条目为 null） |
| [`cap:number_format`](./number_format.md) | `[Bun]` | Human Number Grouping, Fixed-Point, Ordinal & Radix Formatter |
| [`cap:oauth`](./oauth.md) | `[Pure-CP]` | cap:oauth 是 CP 语言官方内置的零外部依赖 OAuth 2.0 与 OpenID Connect (OIDC) 认证客户端核心库，完整覆盖 RFC 7636 PKCE (S256) 挑战生成与恒定时间校验、R |
| [`cap:oauth2`](./oauth2.md) | `[Pure-CP]` | cap:oauth2 是 CP 语言官方内置的纯 CP 零 C 依赖 OAuth 2.0 (RFC 6749)、PKCE (RFC 7636)、Token 吊销 (RFC 7009) 与 OpenID Connect ( |
| [`cap:openapi`](./openapi.md) | `[Pure-CP]` | cap:openapi 是 CP 语言官方内置的纯原生 OpenAPI 3.1.0 规范建模、路径模板匹配、参数提取、组件复用、RFC 8259 合规 JSON 导出与请求契约校验引擎，专为 CP 后端 API 网关 与 |
| [`cap:opml`](./opml.md) | `[Pure-CP]` | 零分配 OPML 2.0 大纲/订阅列表解析与序列化标准库 |
| [`cap:pe_format`](./pe_format.md) | `[Bun]` | Windows PE64 可执行文件头与节表解析/校验/构建库 |
| [`cap:picohttp`](./picohttp.md) | `[Pure-CP]` | 纯 CP 零堆分配 HTTP/1.1 & Chunked 流式解析器 |
| [`cap:platform`](./platform.md) | `[Bun]` | cap:platform 是 CP 语言官方内置的跨平台操作系统探测、内核版本解析与系统能力检测标准库，吸收自 Bun src/platform/（linux.zig、darwin.zig）。该库向事件循环、文件 I/O |
| [`cap:rfc7807_problem`](./rfc7807_problem.md) | `[Bun]` | RFC 9457 / RFC 7807 HTTP Problem Details |
| [`cap:robots_txt`](./robots_txt.md) | `[Bun]` | RFC 9309 `robots.txt` 解析与 URL 匹配引擎 |
| [`cap:route_params`](./route_params.md) | `[Bun]` | HTTP 路由参数映射表与 URLPath 零分配解析器 |
| [`cap:router`](./router.md) | `[Bun]` | cap:router 是 CP 语言官方内置的泛型 Radix-Tree（前缀字典树）与 Next.js/Bun 风格 FileSystemRouter 路由标准库，支持按 HTTP 方法隔离路由树、5 级严格优先级匹配 |
| [`cap:rss`](./rss.md) | `[Pure-CP]` | 零分配 RSS 2.0 Feed 解析、迭代与序列化标准库 |
| [`cap:selector`](./selector.md) | `[Pure-CP]` | cap:selector 是 CP 语言官方内置的工业级纯 CP CSS3 选择器编译器与 XML/DOM 树查询引擎，零 C 依赖、零 std.posix、零 @import("bun")，直接无缝对接 cap:xml |
| [`cap:sitemap`](./sitemap.md) | `[Pure-CP]` | cap:sitemap 是 CP 语言官方内置的纯 CP 网页采集与站点地图标准库，零外部 C 依赖、零 std.posix、零 @import("bun")，基于 cap:xml 构建，为网页采集器、AI Agent  |
| [`cap:sri`](./sri.md) | `[Bun]` | W3C Subresource Integrity (SRI) 解析、格式化与流式校验器 |
| [`cap:sse`](./sse.md) | `[Pure-CP]` | W3C Server-Sent Events (`text/event-stream`) 增量解析与编码库 |
| [`cap:structured_headers`](./structured_headers.md) | `[Bun]` | - SfItemKind：integer / decimal_milli / string / token / byte_seq / boolean。 |
| [`cap:uri_normalize`](./uri_normalize.md) | `[Bun]` | RFC 3986 URI Normalization |
| [`cap:url`](./url.md) | `[Bun]` | cap:url 是 CP 语言官方内置的工业级 URL 解析与 URI 编解码标准库，全量对标并吸收自 Bun 原生 URL 运行时实现（/data/dev/bun/src/url/url.cp）。 |
| [`cap:w3c_traceparent`](./w3c_traceparent.md) | `[Bun]` | 字段：version: u8、trace_id: [16]u8、parent_id: [8]u8、trace_flags: u8。 |
| [`cap:webauthn`](./webauthn.md) | `[Pure-CP]` | FIDO2 / WebAuthn Level 3 服务端验证标准库 |
| [`cap:webhook`](./webhook.md) | `[Pure-CP]` | cap:webhook 是 CP 语言官方内置的纯原生 Webhook 安全验签与幂等去重标准库，零外部依赖（0 std.posix、0 @import("bun")），底层直接使用 Zig 标准库 std.crypto |
| [`cap:websocket_frame`](./websocket_frame.md) | `[Pure-CP]` | RFC 6455 WebSocket 帧协议解析、掩码与分片重组引擎 |
| [`cap:whois`](./whois.md) | `[Pure-CP]` | RFC 3912 WHOIS 域名与 IP 注册信息查询协议零分配编解码库 |
| [`cap:ws`](./ws.md) | `[Bun]` | cap:ws 是 CP 语言官方内置的工业生产级 RFC 6455 / RFC 7692 WebSocket 协议引擎，在保留既有高性能帧编解码、SIMD 掩码与握手校验的基础上，新增子协议与扩展头协商、RFC 6455 |

## 4. 网络协议、传输层与分布式通信

*Networking, Protocols & Messaging* — 共 **95** 个模块（`[Bun]`: 7，`[Pure-CP]`: 88）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:amqp`](./amqp.md) | `[Pure-CP]` | cap:amqp 是 CP 语言官方内置的 AMQP 0-9-1（Advanced Message Queuing Protocol） 纯 CP 二进制线协议编解码与增量拆帧标准库。严格遵循 AMQP 0-9-1 大端序 |
| [`cap:aof`](./aof.md) | `[Pure-CP]` | 纯 CP Redis AOF 命令日志零分配流式解析库 |
| [`cap:arp`](./arp.md) | `[Pure-CP]` | RFC 826 / RFC 5227 ARP 零分配编解码库 |
| [`cap:bencode`](./bencode.md) | `[Pure-CP]` | BEP-3 Bencode 编解码与 Torrent 规范化哈希引擎 |
| [`cap:bfd`](./bfd.md) | `[Pure-CP]` | 零分配 RFC 5880 BFD 双向转发检测协议解析库 |
| [`cap:bgp`](./bgp.md) | `[Pure-CP]` | 零分配 RFC 4271 BGP-4 边界网关路由协议解析标准库 |
| [`cap:bimap`](./bimap.md) | `[Pure-CP]` | 双向一对一映射容器 |
| [`cap:bpf`](./bpf.md) | `[Pure-CP]` | RFC 9669 eBPF 指令集零分配编解码与程序校验库 |
| [`cap:cdp`](./cdp.md) | `[Pure-CP]` | Cisco Discovery Protocol v1/v2 零分配编解码库 |
| [`cap:cidr`](./cidr.md) | `[Pure-CP]` | IPv4/IPv6 CIDR 解析、子网切分、超网聚合与 LPM 路由库 |
| [`cap:circuit`](./circuit.md) | `[Pure-CP]` | cap:circuit 是 CP 语言官方内置的纯 CP 分布式弹性容错与过载保护库，零 C 依赖、零 std.posix、零 @import("bun")。所有时间判定均通过确定性单调毫秒时间戳 now_ms: i64 |
| [`cap:cluster`](./cluster.md) | `[Pure-CP]` | cap:cluster 是 CP 语言官方内置的纯原生分布式集群协调与分片路由标准库，专为百万级 AI Agent 集群分片路由、SWIM Gossip 健康成员表与防脑裂选主租约打造。包含三大核心组件： |
| [`cap:coap`](./coap.md) | `[Pure-CP]` | 零分配 RFC 7252 CoAP 物联网轻量 REST 协议编解码库 |
| [`cap:dhcp`](./dhcp.md) | `[Pure-CP]` | 零分配 RFC 2131 / RFC 2132 DHCPv4 线协议编解码库 |
| [`cap:diameter`](./diameter.md) | `[Pure-CP]` | 零分配 RFC 6733 Diameter AAA 协议编解码库 |
| [`cap:dns`](./dns.md) | `[Bun]` | cap:dns 是 CP 语言官方内置的工业生产级域名解析（DNS）协议套件，涵盖 RFC 1035/2782/3596/8484/8659 报文与资源记录编解码、DNS-over-HTTPS（DoH）、POSIX /e |
| [`cap:dot`](./dot.md) | `[Pure-CP]` | 纯 CP Graphviz DOT 有向/无向图零分配解析与序列化库 |
| [`cap:eap`](./eap.md) | `[Pure-CP]` | 零分配 RFC 3748 EAP + IEEE 802.1X EAPOL 线协议解析库 |
| [`cap:email`](./email.md) | `[Pure-CP]` | cap:email 是 CP 语言官方内置的零外部依赖电子邮件协议核心库，完整覆盖 RFC 5322 邮件地址解析与 CRLF 头注入防护、RFC 2045 / RFC 2047 MIME 邮件构建与递归解析（含 Quo |
| [`cap:ftp`](./ftp.md) | `[Pure-CP]` | RFC 959 / RFC 2428 / RFC 3659 FTP 控制通道与 MLSD 零分配编解码库 |
| [`cap:geneve`](./geneve.md) | `[Pure-CP]` | RFC 8926 Geneve 通用网络虚拟化封装零分配编解码库 |
| [`cap:gre`](./gre.md) | `[Pure-CP]` | 零分配 RFC 2784 / 2890 / 2637 GRE 隧道协议编解码库 |
| [`cap:ical`](./ical.md) | `[Pure-CP]` | cap:ical 是 CP 语言官方内置的纯 CP RFC 5545 iCalendar (.ics) 解析、生成与 RRULE 重复规则展开引擎，零外部 C 依赖、零 std.posix、零 @import("bun" |
| [`cap:icmp`](./icmp.md) | `[Pure-CP]` | RFC 792 ICMPv4 & RFC 4443 ICMPv6 零分配编解码库 |
| [`cap:igmp`](./igmp.md) | `[Pure-CP]` | 零分配 RFC 1112 / 2236 / 3376 IGMP v1/v2/v3 组播协议编解码库 |
| [`cap:imap`](./imap.md) | `[Pure-CP]` | RFC 3501 / RFC 9051 IMAP4rev1/2 线协议编解码与增量解析库 |
| [`cap:intern`](./intern.md) | `[Pure-CP]` | cap:intern 是 CP 语言官方内置的纯 CP 高性能字符串驻留池（StringInterner）与严格 1:1 双向唯一映射表（StringBiMap） 引擎，零 C 依赖、零 std.posix、零 @imp |
| [`cap:ipfix`](./ipfix.md) | `[Pure-CP]` | RFC 7011 IPFIX v10 零分配线协议解析库 |
| [`cap:ipsec`](./ipsec.md) | `[Pure-CP]` | 零分配 RFC 4302 AH / RFC 4303 ESP / RFC 7296 IKEv2 解析库 |
| [`cap:irc`](./irc.md) | `[Pure-CP]` | RFC 1459 / RFC 2812 / IRCv3 线协议与 Message Tags 零分配编解码库 |
| [`cap:iscsi`](./iscsi.md) | `[Pure-CP]` | RFC 7143 iSCSI 48B BHS、Login/Text KV 与 CRC32C 零分配解析库 |
| [`cap:isis`](./isis.md) | `[Pure-CP]` | ISO/IEC 10589 / RFC 1195 IS-IS 零分配线协议解析库 |
| [`cap:kafka_wire`](./kafka_wire.md) | `[Pure-CP]` | Apache Kafka Wire Protocol & RecordBatch v2 标准库 |
| [`cap:kerberos`](./kerberos.md) | `[Pure-CP]` | RFC 4120 Kerberos v5 ASN.1 DER 线协议零分配解析库 |
| [`cap:l2tp`](./l2tp.md) | `[Pure-CP]` | 零分配 RFC 2661 L2TPv2 & RFC 3931 L2TPv3 线协议解析库 |
| [`cap:lacp`](./lacp.md) | `[Pure-CP]` | IEEE 802.1AX / 802.3ad LACPDU 零分配线协议编解码库 |
| [`cap:ldap`](./ldap.md) | `[Pure-CP]` | cap:ldap 是 CP 语言官方内置的纯 CP RFC 4515 LDAP Search Filter 解析、规范化、防注入转义、内存求值与 RFC 4514 Distinguished Name (DN) 解析引擎 |
| [`cap:ldif`](./ldif.md) | `[Pure-CP]` | RFC 2849 LDAP Data Interchange Format (`.ldif`) 零分配解析库 |
| [`cap:lldp`](./lldp.md) | `[Pure-CP]` | 零分配 IEEE 802.1AB LLDP 链路层发现协议编解码库 |
| [`cap:llmnr`](./llmnr.md) | `[Pure-CP]` | RFC 4795 LLMNR 链路本地组播名称解析零分配编解码库 |
| [`cap:mail`](./mail.md) | `[Bun]` | Email Mailbox, RFC 2047 Encoded-Word & Quoted-Printable Toolkit |
| [`cap:mdns`](./mdns.md) | `[Pure-CP]` | RFC 6762 Multicast DNS & RFC 6763 DNS-SD 零分配编解码库 |
| [`cap:mpls`](./mpls.md) | `[Pure-CP]` | RFC 3032 MPLS 标签栈 + RFC 4385 PW-CW + RFC 5586 G-ACh 零分配解析库 |
| [`cap:mqtt`](./mqtt.md) | `[Pure-CP]` | 纯 CP 工业级 MQTT v3.1.1 / v5.0 协议库 |
| [`cap:nats`](./nats.md) | `[Pure-CP]` | cap:nats 是 CP 语言官方内置的 NATS（Neural Autonomic Transport System） 纯 CP 线协议编码与增量流式拆帧标准库。严格遵循 NATS 文本控制行 + 二进制载荷定界与  |
| [`cap:nbns`](./nbns.md) | `[Pure-CP]` | RFC 1001 / RFC 1002 NetBIOS Name Service 零分配编解码库 |
| [`cap:net`](./net.md) | `[Bun]` | cap:net 是 CP 语言官方内置的工业生产级网络传输标准库，涵盖 IPv4/IPv6 地址与 CIDR/SSRF 边界校验、Linux 原生套接字选项与 poll 就绪轮询、RAII 风格 SafeSocket 生 |
| [`cap:netflow`](./netflow.md) | `[Pure-CP]` | 零分配 Cisco NetFlow v5 & RFC 3954 NetFlow v9 解析库 |
| [`cap:nfs`](./nfs.md) | `[Pure-CP]` | RFC 5531 ONC RPC v2 & RFC 1813 NFSv3 零分配线协议解析库 |
| [`cap:nntp`](./nntp.md) | `[Pure-CP]` | RFC 3977 NNTP 网络新闻传输协议零分配编解码库 |
| [`cap:ntp`](./ntp.md) | `[Pure-CP]` | RFC 5905 NTPv4 / SNTP 零分配编解码与时钟偏移库 |
| [`cap:ospf`](./ospf.md) | `[Pure-CP]` | RFC 2328 OSPFv2 零分配路由协议解析库 |
| [`cap:pcap`](./pcap.md) | `[Pure-CP]` | cap:pcap 是 CP 语言官方内置的纯 CP .pcap 容器编解码器与轻量无拷贝 L2–L4 网络协议分层解包器。零外部依赖、零 std.posix、零 @import("bun")，完全基于内存字节流运行，不依 |
| [`cap:pcapng`](./pcapng.md) | `[Pure-CP]` | 纯 CP PCAPng 下一代网络抓包容器零分配解析库 |
| [`cap:pim`](./pim.md) | `[Pure-CP]` | RFC 7761 PIM-SM v2 组播协议零分配编解码库 |
| [`cap:pop3`](./pop3.md) | `[Pure-CP]` | RFC 1939 / RFC 2449 POP3 邮局协议 v3 零分配编解码库 |
| [`cap:pppoe`](./pppoe.md) | `[Pure-CP]` | RFC 2516 PPPoE Discovery & Session 零分配编解码库 |
| [`cap:progress_tree`](./progress_tree.md) | `[Bun]` | 纯 CP 零堆分配层级进度树与节流渲染引擎，吸收自 /data/dev/bun/src/bun_core/Progress.cp。 |
| [`cap:ptp`](./ptp.md) | `[Pure-CP]` | IEEE 1588-2008 / 1588-2019 PTPv2/v2.1 零分配线协议编解码库 |
| [`cap:pubsub`](./pubsub.md) | `[Pure-CP]` | cap:pubsub 是 CP 语言官方内置的纯原生进程内通配符主题消息代理（In-Process Wildcard Pub/Sub Broker），专为百万级 AI Agent 本地主题扇出、NATS/MQTT 双协议 |
| [`cap:quic`](./quic.md) | `[Pure-CP]` | cap:quic 是 CP 语言官方内置的纯 CP RFC 9000 QUIC 协议底层线缆编解码引擎，涵盖 §16 变长整数（Varint）、§17 长/短包头（Long/Short Header）以及 §19 核心帧 |
| [`cap:quic_header`](./quic_header.md) | `[Bun]` | 纯 CP QUIC 报文头编解码、QPACK 头分类与 Socket 类型枚举库 |
| [`cap:radius`](./radius.md) | `[Pure-CP]` | RFC 2865 / 2866 RADIUS 认证与计费协议零分配编解码库 |
| [`cap:rdb`](./rdb.md) | `[Pure-CP]` | 零分配 Redis RDB 二进制快照解析库 |
| [`cap:redis`](./redis.md) | `[Bun]` | cap:redis（以及底层同源协议层 lib/valkey/）是 CP 语言官方内置的工业级 Redis / Valkey 序列化协议（RESP2 / RESP3）标准库，全面吸收自 Bun 原生 Valkey 协议实 |
| [`cap:redis_cluster`](./redis_cluster.md) | `[Pure-CP]` | Redis / Valkey Cluster 槽位路由与拓扑管理 |
| [`cap:rip`](./rip.md) | `[Pure-CP]` | RFC 2453 RIPv1/v2 & RFC 2080 RIPng 零分配线协议编解码库 |
| [`cap:rtcp`](./rtcp.md) | `[Pure-CP]` | RFC 3550 RTCP 复合包零分配线协议解析库 |
| [`cap:rtp`](./rtp.md) | `[Pure-CP]` | 纯 CP RFC 3550 RTP/RTCP 报文编解码与序列号抖动缓冲库 |
| [`cap:rtsp`](./rtsp.md) | `[Pure-CP]` | cap:rtsp 是 CP 语言官方内置的 RFC 2326 RTSP/1.0（Real Time Streaming Protocol） 纯 CP 协议编解码、Transport 头解析与 TCP 交织流拆帧标准库。严 |
| [`cap:sctp`](./sctp.md) | `[Pure-CP]` | RFC 4960 SCTP 零分配编解码库 |
| [`cap:sdp`](./sdp.md) | `[Pure-CP]` | cap:sdp 是 CP 语言官方内置的纯 CP RFC 8866 / RFC 4566 SDP (Session Description Protocol) 解析与构建引擎，专为 WebRTC Offer/Answer |
| [`cap:sflow`](./sflow.md) | `[Pure-CP]` | sFlow v5 实时网络采样遥测零分配线协议解析库 |
| [`cap:sip`](./sip.md) | `[Pure-CP]` | cap:sip 是 CP 语言官方内置的纯 CP RFC 3261 SIP (Session Initiation Protocol) 报文解析与构建引擎，专为 VoIP / WebRTC 信令网关、SIP Proxy  |
| [`cap:smb`](./smb.md) | `[Pure-CP]` | MS-SMB2 / SMB3 文件共享协议零分配线协议解析库 |
| [`cap:smtp`](./smtp.md) | `[Pure-CP]` | RFC 5321/5322 SMTP 线协议编解码与 MIME 邮件构建库 |
| [`cap:snmp`](./snmp.md) | `[Pure-CP]` | RFC 1157 / RFC 3416 SNMP v1/v2c 零分配解析库 |
| [`cap:socks`](./socks.md) | `[Pure-CP]` | RFC 1928 / RFC 1929 / SOCKS4/4a 代理协议零分配编解码库 |
| [`cap:socks5`](./socks5.md) | `[Pure-CP]` | RFC 1928 & RFC 1929 SOCKS5 协议编解码库 |
| [`cap:ssdp`](./ssdp.md) | `[Pure-CP]` | UPnP / DLNA SSDP 局域网服务发现零分配编解码库 |
| [`cap:ssh`](./ssh.md) | `[Bun]` | cap:ssh 是 CP 语言官方内置的纯 CP OpenSSH 公钥 Wire 格式解析、标准 SHA256: 指纹计算与 authorized_keys 策略解析引擎，零 C 依赖、零 std.posix、零 @im |
| [`cap:ssh_wire`](./ssh_wire.md) | `[Pure-CP]` | cap:ssh_wire（亦可通过 cap.ssh_wire 访问）提供纯 CP 实现的工业级 SSH-2 Wire 二进制编解码、OpenSSH 公私钥格式解析及 SSHSIG 签名/验签能力： |
| [`cap:sshsig`](./sshsig.md) | `[Pure-CP]` | 零分配 OpenSSH `SSHSIG` v1 文件与 Git Commit 签名解析库 |
| [`cap:stomp`](./stomp.md) | `[Pure-CP]` | STOMP 1.2 协议帧编解码与增量流式解析库 |
| [`cap:stp`](./stp.md) | `[Pure-CP]` | IEEE 802.1D / 802.1w STP & RSTP BPDU 零分配编解码库 |
| [`cap:stun`](./stun.md) | `[Pure-CP]` | 纯 CP RFC 5389 / RFC 8489 STUN 线协议编解码与完整性校验库 |
| [`cap:syslog`](./syslog.md) | `[Pure-CP]` | RFC 5424 & RFC 3164 Syslog 解析与格式化引擎 |
| [`cap:tacacs`](./tacacs.md) | `[Pure-CP]` | 零分配 RFC 8907 TACACS+ AAA 线协议解析库 |
| [`cap:tftp`](./tftp.md) | `[Pure-CP]` | RFC 1350 / RFC 2347-2349 TFTP 零分配编解码库 |
| [`cap:turn`](./turn.md) | `[Pure-CP]` | RFC 8656 TURN 线协议、ChannelData 帧与 XOR-ADDRESS 零分配编解码库 |
| [`cap:valkey`](./valkey.md) | `[Pure-CP]` | RESP2/RESP3 Valkey & Redis Protocol Codec |
| [`cap:vcard`](./vcard.md) | `[Pure-CP]` | cap:vcard 是 CP 语言官方内置的纯 CP RFC 6350 / RFC 2426 vCard（3.0 & 4.0）电子名片通讯录 .vcf 批量解析与规范生成引擎，零外部 C 依赖、零 std.posix、零 |
| [`cap:vrrp`](./vrrp.md) | `[Pure-CP]` | RFC 3768 VRRPv2 & RFC 5798 VRRPv3 零分配线协议解析库 |
| [`cap:vxlan`](./vxlan.md) | `[Pure-CP]` | RFC 7348 VXLAN & VXLAN-GPE 零分配编解码库 |
| [`cap:wireguard`](./wireguard.md) | `[Pure-CP]` | cap:wireguard 提供纯 CP 实现的工业级 WireGuard Noise_IKpsk2 密码学握手原语、TAI64N 单调时间戳与 RFC 6479 2048-bit 防重放滑动窗口标准库。全模块采用零堆分 |

## 5. 密码学、哈希与安全认证

*Cryptography, Hashing & Security* — 共 **45** 个模块（`[Bun]`: 3，`[Pure-CP]`: 42）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:aes_gcm`](./aes_gcm.md) | `[Pure-CP]` | AES-GCM & AES-GCM-SIV 认证加密标准库 |
| [`cap:age`](./age.md) | `[Pure-CP]` | 零分配 age v1 现代文件加密格式解析标准库 |
| [`cap:argon2`](./argon2.md) | `[Pure-CP]` | RFC 9106 Argon2 内存硬化密码哈希与 PHC 字符串标准库 |
| [`cap:asn1`](./asn1.md) | `[Pure-CP]` | cap:asn1 是 CP 语言官方内置的纯 CP ITU-T X.690 ASN.1 DER 编解码与 X.509 v3 证书内省标准库，零 C 依赖、零 std.posix、零 @import("bun")，既可通过 |
| [`cap:audit`](./audit.md) | `[Pure-CP]` | cap:audit 是 CP 语言官方内置的纯 CP 防篡改加密哈希链审计日志（Tamper-Evident Hash-Chained Audit Log），零 C 依赖、零 std.posix、零 @import("b |
| [`cap:blake3`](./blake3.md) | `[Pure-CP]` | BLAKE3 密码学哈希、MAC、KDF 与 XOF 标准库 |
| [`cap:cel`](./cel.md) | `[Pure-CP]` | Common Expression Language 安全策略求值引擎 |
| [`cap:chacha20`](./chacha20.md) | `[Pure-CP]` | 纯 CP RFC 8439 ChaCha20-Poly1305 / XChaCha20 AEAD 加密引擎 |
| [`cap:cose`](./cose.md) | `[Pure-CP]` | COSE (RFC 9052 / RFC 9053) 标准库 |
| [`cap:crc`](./crc.md) | `[Pure-CP]` | 全谱系 CRC-8 / CRC-16 / CRC-32 / CRC-32C / CRC-64 校验和标准库 |
| [`cap:crl`](./crl.md) | `[Pure-CP]` | 零分配 RFC 5280 X.509 证书吊销列表 (CRL) DER 解析库 |
| [`cap:crypto`](./crypto.md) | `[Bun]` | cap:crypto 是 CP 语言官方内置的生产级零 C 依赖全栈现代密码学子系统标准库，结合 Zig 0.17 原生 std.crypto 与 Bun src/boringssl/、src/sha_hmac/ 现代密 |
| [`cap:csrf`](./csrf.md) | `[Pure-CP]` | CSRF 防护标准库 |
| [`cap:der`](./der.md) | `[Pure-CP]` | 零分配 ITU-T X.690 ASN.1 DER 严格编解码标准库 |
| [`cap:ed25519`](./ed25519.md) | `[Pure-CP]` | RFC 8032 Ed25519 数字签名与密钥对标准库 |
| [`cap:fnv`](./fnv.md) | `[Pure-CP]` | FNV-1 / FNV-1a (32/64/128-bit) 与 DJB2 / SDBM / Jenkins OAAT 哈希套件 |
| [`cap:hash`](./hash.md) | `[Bun]` | cap:hash 是 CP 语言官方内置的超高速非加密哈希（Non-Cryptographic Hash Functions）与校验和矩阵标准库。模块吸收自 Bun（/data/dev/bun/src/wyhash/wy |
| [`cap:highway`](./highway.md) | `[Pure-CP]` | 纯 CP SIMD 向量化字节扫描、ASCII 校验与子串检索标准库 |
| [`cap:hkdf_kdf`](./hkdf_kdf.md) | `[Pure-CP]` | 纯 CP RFC 5869 HKDF 密钥派生引擎 |
| [`cap:isbn`](./isbn.md) | `[Pure-CP]` | ISO 2108 ISBN、ISO 3297 ISSN 与 GS1 条码校验转换库 |
| [`cap:jks`](./jks.md) | `[Pure-CP]` | Java KeyStore (`.jks`) v1/v2 零分配二进制解析库 |
| [`cap:minisign`](./minisign.md) | `[Pure-CP]` | 零分配 Minisign & OpenBSD Signify 编解码标准库 |
| [`cap:murmur3`](./murmur3.md) | `[Pure-CP]` | MurmurHash3 x86_32 & x64_128 零堆非加密哈希标准库 |
| [`cap:ntlm`](./ntlm.md) | `[Pure-CP]` | MS-NLMP NTLM v1/v2 认证线协议零分配解析库 |
| [`cap:ocsp`](./ocsp.md) | `[Pure-CP]` | 零分配 RFC 6960 OCSP 请求与响应 DER 解析库 |
| [`cap:otp`](./otp.md) | `[Pure-CP]` | RFC 4226 HOTP, RFC 6238 TOTP 与 `otpauth://` URI 引擎 |
| [`cap:paseto`](./paseto.md) | `[Pure-CP]` | PASETO v4 安全认证令牌标准库 |
| [`cap:pem`](./pem.md) | `[Pure-CP]` | RFC 7468 PEM 证书链与密钥块编解码库 |
| [`cap:pgp`](./pgp.md) | `[Pure-CP]` | 零分配 OpenPGP RFC 4880/9580 Armor & Packet 解析标准库 |
| [`cap:pkcs12`](./pkcs12.md) | `[Pure-CP]` | 零分配 RFC 7292 PKCS#12 (`.p12`/`.pfx`) ASN.1 DER 解析库 |
| [`cap:pkcs7`](./pkcs7.md) | `[Pure-CP]` | 零分配 RFC 5652 CMS / PKCS#7 ASN.1 DER 解析库 |
| [`cap:pkcs8`](./pkcs8.md) | `[Pure-CP]` | 零分配 RFC 5958 PKCS#8 & RFC 5280 SPKI DER 密钥容器解析标准库 |
| [`cap:rbac`](./rbac.md) | `[Pure-CP]` | cap:rbac 是 CP 语言官方内置的纯 CP 企业级 RBAC / IAM 策略控制标准库，零 C 依赖、零 std.posix、零 @import("bun")，专为多租户后端与 AI Agent 工具调用权限门 |
| [`cap:scrypt`](./scrypt.md) | `[Pure-CP]` | RFC 7914 scrypt 内存硬化口令派生与 PHC 编解码 |
| [`cap:security`](./security.md) | `[Bun]` | cap:security 是 CP 语言官方内置的应用安全与鉴权标准库，提供纯 CP 实现的 CSRF 无状态令牌、RFC 4226/6238 HOTP/TOTP 双因素认证与 RFC 4648 Base32、Fetch |
| [`cap:sha_hmac`](./sha_hmac.md) | `[Pure-CP]` | 统一密码学哈希、HMAC、HKDF 与 PBKDF2 标准库 |
| [`cap:shamir`](./shamir.md) | `[Pure-CP]` | GF(2^8) Shamir 门限密钥分片标准库 |
| [`cap:siphash`](./siphash.md) | `[Pure-CP]` | SipHash-2-4 & SipHash-1-3 抗 Hash-Flooding PRF 标准库 |
| [`cap:totp_hotp`](./totp_hotp.md) | `[Pure-CP]` | RFC 4226 (HOTP) & RFC 6238 (TOTP) 一次性动态口令标准库 |
| [`cap:tpm`](./tpm.md) | `[Pure-CP]` | TCG TPM 2.0 报头、远程证明、Quote 与 Public 零分配解析编码库 |
| [`cap:verhoeff`](./verhoeff.md) | `[Pure-CP]` | Zero-Allocation Verhoeff ($D_5$), Damm Quasigroup & ISO/IEC 7064 Check Digit Suite |
| [`cap:wyhash`](./wyhash.md) | `[Pure-CP]` | lib/wyhash/ 提供纯 CP 实现的工业级 Wyhash v4 高速 64-bit 非加密哈希引擎与 WyRand 伪随机数生成器，支持全无状态一次性哈希、任意分块流式增量哈希、Lemire 无偏差范围随机整数生 |
| [`cap:x25519`](./x25519.md) | `[Pure-CP]` | 纯 CP RFC 7748 X25519 椭圆曲线密钥协商引擎 |
| [`cap:x509`](./x509.md) | `[Pure-CP]` | 纯 CP X.509 v3 证书解析、SAN 匹配与链校验引擎 |
| [`cap:xxhash`](./xxhash.md) | `[Pure-CP]` | 工业级 xxHash 与流式分块哈希标准库 |

## 6. 存储、数据库、文件系统与可观测性

*Storage, Databases, Filesystems & Observability* — 共 **65** 个模块（`[Bun]`: 5，`[Pure-CP]`: 60）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:analytics`](./analytics.md) | `[Pure-CP]` | 运行时遥测收集与脱敏批处理标准库 |
| [`cap:apfs`](./apfs.md) | `[Pure-CP]` | Apple File System (APFS) 超级块与 Fletcher-64 零分配解析库 |
| [`cap:arrow`](./arrow.md) | `[Pure-CP]` | 纯 CP Apache Arrow IPC Stream / File 容器帧与 Footer 零分配解析库 |
| [`cap:arrow_ipc`](./arrow_ipc.md) | `[Pure-CP]` | Apache Arrow IPC 列式内存批处理与零拷贝读写标准库 |
| [`cap:avro`](./avro.md) | `[Pure-CP]` | cap:avro 是 CP 语言官方内置的纯 CP Apache Avro Schema 驱动二进制编解码与 Object Container File (OCF "Obj\x01") 容器引擎。零外部依赖、零 std. |
| [`cap:blob`](./blob.md) | `[Bun]` | cap:blob 是 CP 语言官方内置的 Web 标准二进制大对象（Blob）与具名文件（File）抽象库，吸收自 Bun 的 WebCore Blob/Store 子系统（/data/dev/bun/src/runt |
| [`cap:btrfs`](./btrfs.md) | `[Pure-CP]` | Linux Btrfs Zero-Allocation Filesystem Parser |
| [`cap:cef`](./cef.md) | `[Pure-CP]` | 零分配 ArcSight Common Event Format (CEF Rev 25) SIEM 日志解析库 |
| [`cap:cpio`](./cpio.md) | `[Pure-CP]` | 纯 CP 零分配 Unix CPIO (`newc`/`crc`/`odc`) 归档解析库 |
| [`cap:cramfs`](./cramfs.md) | `[Pure-CP]` | Linux CramFS 压缩只读文件系统零分配解析库 |
| [`cap:cypher`](./cypher.md) | `[Pure-CP]` | 纯 CP openCypher / Neo4j 图查询零分配词法扫描与只读安全判定库 |
| [`cap:dmg`](./dmg.md) | `[Pure-CP]` | 零分配 Apple UDIF (`.dmg`) 磁盘镜像解析库 |
| [`cap:erofs`](./erofs.md) | `[Pure-CP]` | Linux EROFS Zero-Allocation Filesystem Parser |
| [`cap:exfat`](./exfat.md) | `[Pure-CP]` | Microsoft exFAT 引导扇区、校验和与目录项零分配解析库 |
| [`cap:ext4`](./ext4.md) | `[Pure-CP]` | Linux ext2 / ext3 / ext4 零分配文件系统解析库 |
| [`cap:f2fs`](./f2fs.md) | `[Pure-CP]` | Linux F2FS 闪存友好文件系统零分配解析库 |
| [`cap:fat`](./fat.md) | `[Pure-CP]` | Microsoft FAT12 / FAT16 / FAT32 BPB 与目录项零分配解析库 |
| [`cap:fits`](./fits.md) | `[Pure-CP]` | 纯 CP 零分配 NASA FITS 2880B HDU 与 80B Card 解析库 |
| [`cap:gguf`](./gguf.md) | `[Pure-CP]` | lib/gguf/（对外协议 cap:gguf 与 cap.gguf）是纯 CP 实现的工业级 GGUF v2/v3（llama.cpp 本地大模型权重与元数据二进制格式）解析、构建与块量化/反量化标准库。 |
| [`cap:gossip`](./gossip.md) | `[Pure-CP]` | Sans-IO SWIM 分布式集群成员发现与节点健康探测标准库 |
| [`cap:gpt`](./gpt.md) | `[Pure-CP]` | UEFI GUID Partition Table & Protective MBR 零分配解析校验库 |
| [`cap:hdf5`](./hdf5.md) | `[Pure-CP]` | 零分配 HDF5 容器 Superblock & OHDR 解析库 |
| [`cap:iso`](./iso.md) | `[Pure-CP]` | 零分配 ISO 9660 光盘镜像解析库 |
| [`cap:jffs2`](./jffs2.md) | `[Pure-CP]` | Linux JFFS2 闪存日志文件系统零分配解析库 |
| [`cap:kv`](./kv.md) | `[Pure-CP]` | cap:kv 是 CP 语言官方内置的零 C 依赖持久化嵌入式键值存储引擎，采用经典 Bitcask Append-Only WAL 架构，专为 AI Agent 本地状态持久化、工具调用缓存与会话检查点（Checkpo |
| [`cap:lsm`](./lsm.md) | `[Pure-CP]` | cap:lsm 是 CP 语言官方内置的纯 CP LSM-Tree（Log-Structured Merge-Tree）高写入吞吐嵌入式存储引擎，零 C 依赖、零 std.posix、零 @import("bun")。核 |
| [`cap:lsm_sst`](./lsm_sst.md) | `[Pure-CP]` | LSM-Tree Sorted String Table (SSTable) 标准库 |
| [`cap:luks`](./luks.md) | `[Pure-CP]` | Linux LUKS1 & LUKS2 磁盘加密报头零分配解析库 |
| [`cap:metrics`](./metrics.md) | `[Pure-CP]` | cap:metrics 是 CP 语言官方内置的纯原生可观测性指标与 Prometheus/OpenMetrics 导出库，专为百万级 AI Agent 并发运行时与云原生服务 /metrics 端点采集打造。核心组件分 |
| [`cap:migrate`](./migrate.md) | `[Pure-CP]` | cap:migrate 是 CP 语言官方内置的纯 CP 确定性 Schema/数据库迁移规划与防篡改校验引擎，零 C 依赖、零 std.posix、零 @import("bun")。它基于 cap:crypto 的 S |
| [`cap:mvcc`](./mvcc.md) | `[Pure-CP]` | 多版本并发控制与快照隔离标准库 |
| [`cap:mysql`](./mysql.md) | `[Bun]` | MySQL 协议与 Wire 编解码标准库 |
| [`cap:netcdf`](./netcdf.md) | `[Pure-CP]` | 零分配 Unidata NetCDF (CDF-1 / CDF-2 / CDF-5) 解析库 |
| [`cap:ntfs`](./ntfs.md) | `[Pure-CP]` | Microsoft NTFS VBR、MFT 记录与 Data Runlist 零分配解析库 |
| [`cap:onnx`](./onnx.md) | `[Pure-CP]` | ONNX 计算图与权重模型二进制解析/构建标准库 |
| [`cap:openmetrics`](./openmetrics.md) | `[Pure-CP]` | cap:openmetrics 是纯 CP 实现的工业级 Prometheus 0.0.4 与 OpenMetrics 1.0.0 文本暴露格式（Text Exposition Format）解析、校验与序列化标准库，支 |
| [`cap:orc`](./orc.md) | `[Pure-CP]` | 纯 CP Apache ORC 列式存储容器零分配解析库 |
| [`cap:otlp`](./otlp.md) | `[Pure-CP]` | OpenTelemetry OTLP 遥测标准库 |
| [`cap:parquet`](./parquet.md) | `[Pure-CP]` | Apache Parquet 列式存储与 Thrift / RLE 编解码库 |
| [`cap:perf`](./perf.md) | `[Pure-CP]` | 性能剖析、作用域计时与 Trace Event 导出 |
| [`cap:pg`](./pg.md) | `[Bun]` | 纯 CP PostgreSQL Wire Protocol v3 全栈协议与类型工具库 |
| [`cap:pgwire`](./pgwire.md) | `[Pure-CP]` | PostgreSQL v3.0 Wire Protocol 标准库 |
| [`cap:prometheus`](./prometheus.md) | `[Pure-CP]` | Prometheus / OpenMetrics 指标注册表、直方图与文本格式编解码库 |
| [`cap:promql`](./promql.md) | `[Pure-CP]` | PromQL 词法与 AST 递归下降解析器 |
| [`cap:qcow2`](./qcow2.md) | `[Pure-CP]` | QEMU QCOW2 v2/v3 虚拟磁盘镜像零分配解析库 |
| [`cap:query`](./query.md) | `[Pure-CP]` | cap:query 是 CP 语言官方内置的纯原生防注入参数化 SQL 查询构建器标准库，专为 PostgreSQL、MySQL 与 SQLite3 的安全动态 SQL 生成和标识符转义打造。全模块零 C 依赖、零 st |
| [`cap:raft`](./raft.md) | `[Pure-CP]` | Sans-IO Raft 分布式共识状态机与日志复制引擎 |
| [`cap:raft_log`](./raft_log.md) | `[Pure-CP]` | Raft 共识日志引擎 |
| [`cap:romfs`](./romfs.md) | `[Pure-CP]` | Linux 内核 `romfs` 只读文件系统零分配解析库 |
| [`cap:s3`](./s3.md) | `[Bun]` | cap:s3 是 CP 语言官方内置的高性能对象存储标准库，全量吸收自 Bun 原生 S3 引擎（/data/dev/bun/src/s3_signing/ 及 runtime/webcore/s3/）。模块完全兼容 A |
| [`cap:safetensors`](./safetensors.md) | `[Pure-CP]` | Safetensors 模型权重格式与 BF16/FP16 转换库 |
| [`cap:sparql`](./sparql.md) | `[Pure-CP]` | 纯 CP W3C SPARQL 1.1 零分配词法扫描与查询分析库 |
| [`cap:sql`](./sql.md) | `[Bun]` | cap:sql 是 CP 语言官方内置的多方言关系型数据库协议与嵌入式存储标准库，直接原版移植自 Bun 原生 SQL 驱动与 bun:sqlite 引擎（/data/dev/bun/src/sql/，10 个文件 4, |
| `cap:sqlite` | `[Pure-CP]` | cap:sqlite binding, per CHANGELOG |
| [`cap:squashfs`](./squashfs.md) | `[Pure-CP]` | Linux SquashFS v4.0 只读文件系统零分配解析库 |
| [`cap:sstable`](./sstable.md) | `[Pure-CP]` | 纯 CP LevelDB / RocksDB SSTable 零分配解析库 |
| [`cap:ubifs`](./ubifs.md) | `[Pure-CP]` | Linux UBI & UBIFS 闪存文件系统零分配解析库 |
| [`cap:udf`](./udf.md) | `[Pure-CP]` | ECMA-167 / ISO 13346 UDF 通用光盘格式零分配解析库 |
| [`cap:vdi`](./vdi.md) | `[Pure-CP]` | VirtualBox VDI 1.1 虚拟磁盘镜像零分配解析库 |
| [`cap:vhd`](./vhd.md) | `[Pure-CP]` | Microsoft VHD 1.0 虚拟硬盘零分配解析库 |
| [`cap:vhdx`](./vhdx.md) | `[Pure-CP]` | Microsoft VHDX 1.0 虚拟硬盘零分配解析库 |
| [`cap:vmdk`](./vmdk.md) | `[Pure-CP]` | VMware VMDK 虚拟磁盘报头与 Descriptor 零分配解析库 |
| [`cap:wal`](./wal.md) | `[Pure-CP]` | Castagnoli CRC32C 预写日志（WAL）帧编解码、崩溃恢复与日志压缩引擎 |
| [`cap:xfs`](./xfs.md) | `[Pure-CP]` | Linux XFS v4/v5 Zero-Allocation Filesystem Parser |
| [`cap:zfs`](./zfs.md) | `[Pure-CP]` | OpenZFS Zero-Allocation On-Disk Format Parser |

## 7. 压缩、归档与二进制编解码

*Compression, Archiving & Binary Codecs* — 共 **80** 个模块（`[Bun]`: 7，`[Pure-CP]`: 73）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:apk`](./apk.md) | `[Pure-CP]` | Android APK ZIP EOCD & Signing Block v2/v3 零分配解析库 |
| [`cap:ar`](./ar.md) | `[Pure-CP]` | 纯 CP Unix `ar` / `.a` / `.deb` 归档容器零分配解析库 |
| [`cap:base32`](./base32.md) | `[Pure-CP]` | 纯 CP RFC 4648 Base32 / Base32hex / Crockford Base32 编解码引擎 |
| [`cap:base58`](./base58.md) | `[Pure-CP]` | Base58, Base58Check 与 BIP-173/350 Bech32/Bech32m 编解码库 |
| [`cap:base58_check`](./base58_check.md) | `[Pure-CP]` | 纯 CP Bitcoin Base58 & Base58Check 双重 SHA-256 编解码引擎 |
| [`cap:base64`](./base64.md) | `[Pure-CP]` | Pure CP Base64 / Base64URL Standard Library |
| [`cap:base64url`](./base64url.md) | `[Pure-CP]` | 纯 CP RFC 4648 Base64 & Base64URL 无填充/自动对齐编解码引擎 |
| [`cap:base85`](./base85.md) | `[Pure-CP]` | Z85, Adobe Ascii85 与 RFC 1924 Base85 编解码库 |
| [`cap:bech32`](./bech32.md) | `[Pure-CP]` | BIP-173 Bech32 & BIP-350 Bech32m 纠错码标准库 |
| [`cap:binary_schema`](./binary_schema.md) | `[Bun]` | 纯 CP 紧凑二进制 Schema 流式编解码引擎 |
| [`cap:bincode`](./bincode.md) | `[Pure-CP]` | 纯 CP Rust `bincode` v1/v2 零分配二进制编解码器 |
| [`cap:bitstream`](./bitstream.md) | `[Pure-CP]` | MSB/LSB 位流读写器与 Exp-Golomb / LEB128 编解码库 |
| [`cap:borsh`](./borsh.md) | `[Pure-CP]` | 纯 CP Borsh 确定性二进制编解码器（零堆分配） |
| [`cap:braille`](./braille.md) | `[Pure-CP]` | Unicode Braille 编解码、Grade-1 盲文转换与 2x4 终端点阵画布 |
| [`cap:brotli`](./brotli.md) | `[Pure-CP]` | 纯 CP RFC 7932 Brotli Meta-Block 编解码库 |
| [`cap:bson`](./bson.md) | `[Pure-CP]` | cap:bson 是 CP 语言官方内置的 MongoDB BSON 1.1（Binary JSON） 高性能编解码标准库，严格遵循 BSON 1.1 小端序二进制规范与 12 字节 ObjectId 布局（[times |
| [`cap:bwt`](./bwt.md) | `[Pure-CP]` | Zero-Allocation Burrows-Wheeler Transform & Move-To-Front Frontend |
| [`cap:bzip2`](./bzip2.md) | `[Pure-CP]` | bzip2 `.bz2` Container & MSB-First Bitstream Zero-Allocation Parser |
| [`cap:cab`](./cab.md) | `[Pure-CP]` | 零分配 Microsoft Cabinet (`.cab`) 归档解析库 |
| [`cap:capnp`](./capnp.md) | `[Pure-CP]` | Cap'n Proto 零拷贝二进制编解码库 |
| [`cap:cbor`](./cbor.md) | `[Pure-CP]` | cap:cbor 是 CP 语言官方内置的高性能紧凑二进制序列化标准库，提供完整的 RFC 8949 CBOR 与 Section 4.2 确定性规范编码（Deterministic Canonical Encoding |
| [`cap:cbor_diag`](./cbor_diag.md) | `[Pure-CP]` | RFC 8949 CBOR 确定性编解码与诊断表示法 |
| [`cap:cuid`](./cuid.md) | `[Pure-CP]` | 纯 CP 零分配 CUID / CUID2 校验、分类与确定性编码库 |
| [`cap:cuid2`](./cuid2.md) | `[Pure-CP]` | 纯 CP 抗碰撞 CUID2 Base36 唯一 ID 生成引擎 |
| [`cap:deb`](./deb.md) | `[Pure-CP]` | 纯 CP Debian `.deb` 软件包与 `DEBIAN/control` 零分配解析库 |
| [`cap:deflate`](./deflate.md) | `[Pure-CP]` | 纯 CP RFC 1951 DEFLATE / RFC 1952 GZIP / RFC 1950 ZLIB 编解码库 |
| [`cap:elias`](./elias.md) | `[Pure-CP]` | 通用前缀整数编码与位流零分配套件 |
| [`cap:encoding`](./encoding.md) | `[Bun]` | cap:encoding 是 CP 语言官方内置的工业级二进制与文本编码转换标准库，全面吸收自 Bun 生产级运行时源码（/data/dev/bun/src/base64/base64.cp 及字符编码基础设施）。 |
| [`cap:flatbuf`](./flatbuf.md) | `[Pure-CP]` | 纯 CP Google FlatBuffers 二进制零拷贝解析库 |
| [`cap:flexbuf`](./flexbuf.md) | `[Pure-CP]` | 纯 CP Google FlexBuffers 无 Schema 零拷贝解析库 |
| [`cap:hex_dump`](./hex_dump.md) | `[Bun]` | Hex Encode/Decode & Canonical HexDump Formatter |
| [`cap:interchange`](./interchange.md) | `[Pure-CP]` | cap:interchange 是 CP 语言官方内置的纯 CP 统一数据交换格式标准库，零外部依赖、零 std.posix、零 @import("bun")，提供统一的 InterchangeValue 模型与 Int |
| [`cap:ion`](./ion.md) | `[Pure-CP]` | 纯 CP Amazon Ion 1.0 二进制格式零分配解析库 |
| [`cap:ksuid`](./ksuid.md) | `[Pure-CP]` | Segment KSUID 20 字节 / 27 字符 Base62 字典序唯一 ID 标准库 |
| [`cap:libarchive`](./libarchive.md) | `[Pure-CP]` | 纯 CP 统一归档格式探测与 Ar / CPIO / USTAR Tar 读写标准库 |
| [`cap:lz4`](./lz4.md) | `[Pure-CP]` | cap:lz4 是 CP 语言官方内置的高性能、零 C 依赖纯 CP 数据压缩标准库，面向内存缓存、WAL 日志压缩（配合 cap:kv）及高速网络帧传输设计，完整实现 LZ4 Block Format（原始块格式）、L |
| [`cap:lzh`](./lzh.md) | `[Pure-CP]` | LZH / LHA `.lzh` 归档格式零分配解析库 |
| [`cap:lzo`](./lzo.md) | `[Pure-CP]` | `lzop` `.lzo` Container & Zero-Allocation LZO1X Decompressor |
| [`cap:lzss`](./lzss.md) | `[Pure-CP]` | 零堆分配 LZSS 滑动窗口压缩与解压库 |
| [`cap:lzw`](./lzw.md) | `[Pure-CP]` | Pure CP Zero-Allocation LZW Codec (GIF LSB & TIFF/PDF MSB) |
| [`cap:marshal`](./marshal.md) | `[Pure-CP]` | 纯 CP Ruby Marshal 4.8 二进制对象零分配解析库 |
| [`cap:morse`](./morse.md) | `[Pure-CP]` | ITU-R M.1677-1 国际摩尔斯电码与时序脉冲库 |
| [`cap:msgpack`](./msgpack.md) | `[Pure-CP]` | cap:msgpack 是 CP 语言官方内置的纯 CP MessagePack 二进制序列化与反序列化引擎。零外部依赖、零 std.posix、零 @import("bun")，既支持 import { ... } f |
| [`cap:msgpack_rpc`](./msgpack_rpc.md) | `[Pure-CP]` | MessagePack-RPC 协议帧与流式多路复用解码标准库 |
| [`cap:nanoid`](./nanoid.md) | `[Pure-CP]` | cap:nanoid 是 CP 语言官方内置的纯 CP 高性能唯一标识符生成与解析库，零 C 依赖、零 std.posix、零 @import("bun")。所有随机数均通过显式注入的确定性 SplitMix64 伪随机 |
| [`cap:nbt`](./nbt.md) | `[Pure-CP]` | 纯 CP Named Binary Tag (NBT) 零分配解析库 |
| [`cap:pack`](./pack.md) | `[Bun]` | cap:pack（在包管理器中同时兼容映射为 cap:compress）是 CP 语言官方内置的工业级无损数据压缩、流式解压、零拷贝归档（Tar / PKZIP）与完整性校验和（CRC32 / Adler32）全栈标准库 |
| [`cap:pickle`](./pickle.md) | `[Pure-CP]` | 纯 CP Python Pickle Protocol 2–5 二进制指令流解析与静态审计库 |
| [`cap:postcard`](./postcard.md) | `[Pure-CP]` | 纯 CP Postcard 紧凑二进制编解码与 COBS 帧定界库 |
| [`cap:protobuf`](./protobuf.md) | `[Pure-CP]` | cap:protobuf 是 CP 语言官方内置的纯原生 Protocol Buffers 二进制线缆编解码与 gRPC 分帧引擎，专为高性能微服务、AI Agent 集群通信与跨节点 RPC 协议打造。 |
| [`cap:punycode`](./punycode.md) | `[Bun]` | RFC 3492 Punycode Codec |
| [`cap:quat`](./quat.md) | `[Pure-CP]` | Zero-Allocation 3D Unit Quaternion, Euler ZYX, Mat3 & SLERP Library |
| [`cap:ransac`](./ransac.md) | `[Pure-CP]` | 零分配确定性 RANSAC 鲁棒模型估计库 |
| [`cap:rar`](./rar.md) | `[Pure-CP]` | RAR4 & RAR5 归档容器格式零分配解析库 |
| [`cap:rle`](./rle.md) | `[Pure-CP]` | 多标准零堆分配游程编码（RLE）编解码套件 |
| [`cap:rlp`](./rlp.md) | `[Pure-CP]` | 纯 CP Ethereum RLP 确定性二进制编解码库 |
| [`cap:rpm`](./rpm.md) | `[Pure-CP]` | 纯 CP Linux RPM v3/v4 软件包零分配头部与 Tag 解析库 |
| [`cap:scale`](./scale.md) | `[Pure-CP]` | 纯 CP Parity SCALE 确定性二进制编解码库 |
| `cap:serde` | `[Bun]` | cap:serde 标准库模块 |
| [`cap:smile`](./smile.md) | `[Pure-CP]` | 零分配 Jackson Smile 1.0 二进制 JSON 解析库 |
| [`cap:snappy`](./snappy.md) | `[Pure-CP]` | Snappy Block Format 高速无损压缩与解压库 |
| [`cap:snowflake`](./snowflake.md) | `[Pure-CP]` | 分布式 64-bit Snowflake ID 生成与 Base62 编码标准库 |
| [`cap:ssz`](./ssz.md) | `[Pure-CP]` | 纯 CP Ethereum SimpleSerialize (SSZ) 零分配编解码与校验库 |
| [`cap:tar`](./tar.md) | `[Pure-CP]` | cap:tar 是 CP 语言官方内置的零 OS 依赖、纯 CP 内存流式 POSIX.1-1988 ustar 归档编解码标准库，面向容器镜像层、包管理器分发归档及内存文件系统快照设计。 |
| [`cap:thrift`](./thrift.md) | `[Pure-CP]` | 零分配 Apache Thrift (`.thrift`) IDL 词法扫描、声明提取与括号校验器 |
| [`cap:tlv`](./tlv.md) | `[Pure-CP]` | ASN.1 BER/DER Tag-Length-Value (TLV) 二进制编解码标准库 |
| [`cap:ubjson`](./ubjson.md) | `[Pure-CP]` | 纯 CP UBJSON Draft 12 零分配二进制编解码器 |
| [`cap:ulid`](./ulid.md) | `[Pure-CP]` | 工业级 ULID 编解码与单调生成器 |
| [`cap:uuencode`](./uuencode.md) | `[Pure-CP]` | POSIX `uuencode`/`uudecode` & `xxencode` Zero-Allocation Codec |
| [`cap:uuid`](./uuid.md) | `[Pure-CP]` | cap:uuid 是 CP 语言官方内置的纯原生分布式唯一标识符引擎，支持 RFC 9562 UUID (v4 / v5 SHA-1 / v7 时间单调有序)、128 位 Crockford Base32 ULID 与  |
| [`cap:uuid_v7`](./uuid_v7.md) | `[Pure-CP]` | 纯 CP RFC 9562 UUID v4 / v5 / 时间有序 v7 引擎 |
| [`cap:varint_leb128`](./varint_leb128.md) | `[Pure-CP]` | 纯 CP 零堆 LEB128 / SLEB128 / ZigZag 变长整数引擎 |
| [`cap:vlq`](./vlq.md) | `[Bun]` | SourceMap Base64-VLQ 查表高速编解码器 |
| [`cap:warc`](./warc.md) | `[Pure-CP]` | 零分配 ISO 28500 Web ARChive (`.warc`) 1.0/1.1 解析库 |
| [`cap:xdr`](./xdr.md) | `[Pure-CP]` | RFC 4506 XDR (External Data Representation) 零分配编解码库 |
| [`cap:xz`](./xz.md) | `[Pure-CP]` | XZ `.xz` Container Format v1.1.0 Zero-Allocation Parser |
| [`cap:yenc`](./yenc.md) | `[Pure-CP]` | yEnc v1.3 二进制转文本编解码与头尾解析库 |
| [`cap:zip`](./zip.md) | `[Pure-CP]` | 纯 CP PKZIP 归档构建与安全解析库 |
| [`cap:zlib`](./zlib.md) | `[Pure-CP]` | cap:zlib 提供纯 CP 实现的工业级 Zlib (RFC 1950)、Gzip (RFC 1952) 与 Raw Deflate (RFC 1951) 统一压缩、解压与校验能力，零外部 C 依赖，内置防压缩炸弹（ |
| [`cap:zstd`](./zstd.md) | `[Pure-CP]` | cap:zstd 是 CP 语言官方内置的高性能、零 C 依赖纯 CP Zstandard 标准库，严格遵循 RFC 8878 (Zstandard Compressed Data Format)，完整支持 XXH64  |

## 8. 文本、文档格式与模板引擎

*Text, Document Formats & Templating* — 共 **74** 个模块（`[Bun]`: 28，`[Pure-CP]`: 46）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:ansi`](./ansi.md) | `[Bun]` | Unified ANSI Terminal Styling, SGR, Hyperlinks & Stripping |
| [`cap:bibtex`](./bibtex.md) | `[Pure-CP]` | 纯 CP BibTeX (`.bib`) 学术文献条目零分配解析与格式化库 |
| [`cap:charset_label`](./charset_label.md) | `[Bun]` | - CharsetKind：utf8、utf16le、utf16be、windows1252、iso8859_2、gbk、gb18030、big5、shift_jis、euc_jp、euc_kr、koi8_r、windo |
| [`cap:clf`](./clf.md) | `[Pure-CP]` | NCSA Common & Apache/Nginx Combined HTTP 访问日志零分配解析库 |
| [`cap:cow_str`](./cow_str.md) | `[Bun]` | 零拷贝借用 / 原子引用计数写时复制字符串 `CowStr` |
| [`cap:csv`](./csv.md) | `[Bun]` | cap:csv 是 CP 语言官方内置的工业生产级纯 CP 表格数据交换与流式处理标准库，零外部依赖、零 std.posix、零 @import("bun")，既可通过 import { ... } from "cap: |
| [`cap:csv_stream`](./csv_stream.md) | `[Bun]` | - raw: []const u8：原始字段切片（引号字段已剥离外层双引号） |
| [`cap:cue`](./cue.md) | `[Pure-CP]` | 零分配 CUE 配置与数据约束语言词法扫描与校验标准库 |
| [`cap:docx`](./docx.md) | `[Pure-CP]` | 纯 CP OOXML DOCX WordprocessingML 与核心元数据零分配解析库 |
| [`cap:epub`](./epub.md) | `[Pure-CP]` | EPUB 2/3 OCF & OPF 零分配解析库 |
| [`cap:frontmatter`](./frontmatter.md) | `[Bun]` | Markdown YAML/TOML Frontmatter 零分配提取器 |
| [`cap:gedcom`](./gedcom.md) | `[Pure-CP]` | GEDCOM 5.5.1 / 7.0 家谱与谱系数据零分配流式解析库 |
| [`cap:grapheme_cluster`](./grapheme_cluster.md) | `[Bun]` | Unicode UAX #29 扩展字素簇边界判定与 UTF-8 迭代器 |
| [`cap:hcl`](./hcl.md) | `[Pure-CP]` | 零分配 HCL / Terraform 词法扫描、块头迭代与结构校验标准库 |
| [`cap:hjson`](./hjson.md) | `[Pure-CP]` | 零分配 Hjson (Human JSON) 解析器、注释剥离器与标准 JSON 转换器 |
| [`cap:human_fmt`](./human_fmt.md) | `[Bun]` | 纯 CP 零堆分配人类可读格式化工具库（字节大小、纳秒耗时、URL 敏感凭据/UUID/令牌脱敏与 CLI 表格边框渲染），吸收自 /data/dev/bun/src/bun_core/fmt.cp。 |
| [`cap:i18n`](./i18n.md) | `[Pure-CP]` | cap:i18n 是 CP 语言官方内置的纯原生国际化（i18n）引擎，与 cap:intl（数值/字节/时长格式化）形成互补，专为全球化 Web 服务、CLI 工具与多语言 AI Agent 应用打造。全模块零 C 依 |
| [`cap:ini`](./ini.md) | `[Bun]` | cap:ini 是 CP 语言官方内置的纯 CP INI / Systemd Unit (.service) / .gitconfig 解析与增量编辑引擎，零外部依赖、零 std.posix、零 @import("bun |
| [`cap:jmespath`](./jmespath.md) | `[Pure-CP]` | JMESPath JSON 查询与投影转换标准库 |
| [`cap:jq`](./jq.md) | `[Pure-CP]` | 纯 CP JSON 结构化查询与管道转换引擎 |
| [`cap:json`](./json.md) | `[Bun]` | cap:json 是 CP 语言官方内置的高性能 JSON 数据交换标准库，对照 Bun 生产级数据交换模块（/data/dev/bun/src/interchange/）构建，实现 16 字节 SIMD 引号/转义/控 |
| [`cap:json5`](./json5.md) | `[Bun]` | 零分配 JSON5 1.0 词法器、校验器与标准 JSON 转换器 |
| [`cap:json5_lexer`](./json5_lexer.md) | `[Bun]` | 零堆分配 JSON5 词法扫描器与标准 JSON 规范化器 |
| [`cap:json_patch`](./json_patch.md) | `[Pure-CP]` | cap:json_patch（同时支持 import cap from "cap" 下的 cap.json_patch 动态挂载）提供纯 CP 原生实现的工业级 JSON 变更协议套件： |
| [`cap:jsonc`](./jsonc.md) | `[Bun]` | Zero-Alloc JSONC Comment & Trailing-Comma Stripper |
| [`cap:jsonl`](./jsonl.md) | `[Pure-CP]` | - JsonlChunkResult：结构体，包含 consumed_bytes: usize、line_count: usize、has_incomplete_tail: bool。 |
| [`cap:jsonpatch`](./jsonpatch.md) | `[Pure-CP]` | cap:jsonpatch 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 RFC 6901 JSON Pointer & RFC 6902 JSON Patch 原子事务引擎（基于 std.json 构建 |
| [`cap:jsonpath`](./jsonpath.md) | `[Pure-CP]` | RFC 9535 JSONPath 标准库 |
| [`cap:jsonschema`](./jsonschema.md) | `[Pure-CP]` | cap:jsonschema 是 CP 语言官方内置的纯 CP JSON Schema (Draft-07 / 2020-12) 校验与路径诊断引擎，提供 Schema 预编译、强类型与结构约束校验、组合逻辑求值、本地  |
| [`cap:kdl`](./kdl.md) | `[Pure-CP]` | 零分配 KDL 配置文档词法扫描、节点头迭代与结构校验器 |
| [`cap:line_offset_table`](./line_offset_table.md) | `[Bun]` | 基于二分查找的源码行列偏移表 |
| [`cap:logfmt`](./logfmt.md) | `[Bun]` | Structured `logfmt` Log Parser & Encoder |
| [`cap:logger`](./logger.md) | `[Bun]` | cap:logger 提供全零堆分配的工业级双引擎能力：编译器/源码级诊断引擎（源自 Bun src/logger）与 生产级结构化日志 & W3C Trace Context / OpenTelemetry 追踪采样引 |
| [`cap:marc`](./marc.md) | `[Pure-CP]` | 零分配 ISO 2709 / MARC 21 图书馆编目二进制解析库 |
| [`cap:markdown`](./markdown.md) | `[Bun]` | 纯 CP CommonMark / GFM 行分析、内联链接与引用定义解析库 |
| [`cap:markdown_table`](./markdown_table.md) | `[Bun]` | GFM Pipe Table Parser & Escaper |
| [`cap:md`](./md.md) | `[Bun]` | cap:md 是 CP 语言官方内置的高完整度 CommonMark + GitHub Flavored Markdown (GFM) 解析与多目标渲染标准库，直接原版移植自 Bun 生产级 MD4C 架构引擎（/dat |
| [`cap:mermaid`](./mermaid.md) | `[Pure-CP]` | 声明式 Mermaid 图表构造与语法安全转义引擎 |
| [`cap:minix`](./minix.md) | `[Pure-CP]` | MINIX v1 / v2 / v3 经典文件系统零分配解析库 |
| [`cap:mobi`](./mobi.md) | `[Pure-CP]` | 零分配 Amazon Mobipocket / Kindle (`.mobi`/`.azw`) 解析库 |
| [`cap:mseed`](./mseed.md) | `[Pure-CP]` | 零分配 IRIS MiniSEED v2.4 地震学时间序列解析库 |
| [`cap:mustache`](./mustache.md) | `[Pure-CP]` | Zero-Allocation Mustache Template Engine |
| [`cap:ndjson`](./ndjson.md) | `[Pure-CP]` | cap:ndjson 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 NDJSON（Newline-Delimited JSON / JSON Lines）流式分帧、严格校验与安全写入引擎（基于 std.j |
| [`cap:nix`](./nix.md) | `[Pure-CP]` | 纯 CP Nix 表达式与 `flake.nix` 零分配词法扫描与属性提取库 |
| [`cap:odt`](./odt.md) | `[Pure-CP]` | OASIS OpenDocument (`.odt`/`.ods`/`.odp`) 零分配解析库 |
| [`cap:pdf`](./pdf.md) | `[Pure-CP]` | cap:pdf 是 CP 语言官方内置的纯 CP PDF 1.4 多页文档生成与结构内省标准库，零外部依赖、零 std.posix、零 @import("bun")，既可通过 import { ... } from "c |
| [`cap:plist`](./plist.md) | `[Pure-CP]` | 纯 CP Apple XML Property List (`.plist`) 零分配解析与序列化库 |
| [`cap:porter`](./porter.md) | `[Pure-CP]` | Martin Porter (1980) 英文词干提取器 |
| [`cap:pptx`](./pptx.md) | `[Pure-CP]` | 纯 CP OOXML PPTX PresentationML 与 DrawingML 零分配解析库 |
| [`cap:ris`](./ris.md) | `[Pure-CP]` | Research Information Systems (`.ris`) 学术文献引文零分配解析库 |
| [`cap:roman`](./roman.md) | `[Pure-CP]` | 规范罗马数字与电子表格 Bijective Base-26 列号转换库 |
| [`cap:ron`](./ron.md) | `[Pure-CP]` | 零分配 RON (Rusty Object Notation) 词法扫描、平衡校验与字段提取标准库 |
| [`cap:rtf`](./rtf.md) | `[Pure-CP]` | 零分配 Rich Text Format (RTF 1.9) 词法与纯文本提取库 |
| [`cap:schema`](./schema.md) | `[Pure-CP]` | cap:schema 是 CP 语言官方内置的纯 CP JSON Schema Draft 2020-12 运行时校验与 AI Agent Tool Calling 参数验证引擎，零外部依赖、零 std.posix、零  |
| [`cap:sexp`](./sexp.md) | `[Pure-CP]` | 纯 CP 零分配 S-Expression 词法扫描、平衡校验与格式化器 |
| [`cap:sheet`](./sheet.md) | `[Pure-CP]` | cap:sheet 是 CP 语言官方内置的纯原生、零 C 依赖电子表格计算引擎，专为 财务报表引擎、数据分析工具 与 低代码计算图 设计。 |
| [`cap:slugify`](./slugify.md) | `[Bun]` | URL-Safe Slug Generator & Validator |
| [`cap:small_string`](./small_string.md) | `[Pure-CP]` | 纯 CP 8 字节短串内联 (SSO) 与去重驻留池（吸收自 Bun `install_types`） |
| [`cap:smol_str`](./smol_str.md) | `[Bun]` | 小字符串内联优化 Compact String |
| [`cap:soundex`](./soundex.md) | `[Pure-CP]` | 语音索引与姓名模糊匹配零分配套件 |
| [`cap:spv`](./spv.md) | `[Pure-CP]` | 纯 CP 零分配 Khronos SPIR-V 二进制着色器解析库 |
| [`cap:string`](./string.md) | `[Bun]` | 工业级字符串引擎、Unicode 码表与可变缓冲标准库 |
| [`cap:table`](./table.md) | `[Pure-CP]` | cap:table 是 CP 语言官方内置的工业级纯 CP 终端与文本表格渲染引擎，零外部依赖、零 std.posix、零 @import("bun")，既可通过 import { ... } from "cap:tab |
| [`cap:template`](./template.md) | `[Pure-CP]` | 纯 CP Mustache 模板引擎（AI Prompt / HTML / 代码生成） |
| [`cap:text`](./text.md) | `[Bun]` | cap:text 是 CP 语言官方内置的纯原生文本处理与 AI Agent 上下文工程标准库，专为 RAG 文档切分、命令面板模糊搜索与 LLM 上下文窗口预算控制打造。包含三大核心子模块： |
| [`cap:toml`](./toml.md) | `[Bun]` | cap:toml 是 CP 语言官方内置的纯 CP TOML v1.0.0 配置解析与序列化引擎，零外部依赖、零 std.posix、零 @import("bun")，全面覆盖 Cargo.toml、pyproject. |
| [`cap:tui`](./tui.md) | `[Pure-CP]` | cap:tui 是 CP 语言官方内置的纯原生终端用户界面（TUI）输入解析、弹性矩形布局与零闪烁差分渲染引擎，专为交互式 CLI AI Agent、状态仪表盘与全屏终端应用打造。全模块零 C 依赖、零 std.posi |
| [`cap:unicode`](./unicode.md) | `[Bun]` | Unicode / UTF-8 / UTF-16 / Grapheme 与终端显示宽度标准库 |
| [`cap:uritemplate`](./uritemplate.md) | `[Pure-CP]` | cap:uritemplate 是 CP 语言官方内置的纯 CP RFC 6570 Level 1~4 URI Template 解析、标准展开（Expand）与变量清单提取引擎，零 C 依赖、零 std.posix、零 |
| [`cap:utf8_dfa`](./utf8_dfa.md) | `[Bun]` | 无分支 UTF-8 DFA 解码器与 ASCII 码点谓词 |
| [`cap:xlsx`](./xlsx.md) | `[Pure-CP]` | 零分配 OOXML SpreadsheetML `.xlsx` 解析库 |
| [`cap:xml`](./xml.md) | `[Bun]` | cap:xml 是 CP 语言官方内置的纯 CP XML 1.0 协议解析与序列化标准库，零外部依赖、零 std.posix、零 @import("bun")，专为 S3 / RSS / Atom / SVG / JUn |
| [`cap:xpath`](./xpath.md) | `[Pure-CP]` | 纯 CP W3C XPath 零分配词法扫描与路径步进解析库 |
| [`cap:yaml`](./yaml.md) | `[Bun]` | cap:yaml 是 CP 语言官方内置的工业生产级纯 CP YAML 1.2 核心子集解析与序列化标准库，零 C 依赖、零 std.posix、零 @import("bun")，既可通过 import { ... }  |

## 9. 图像、音视频与多媒体图形

*Media, Audio, Video & 3D Graphics* — 共 **80** 个模块（`[Bun]`: 5，`[Pure-CP]`: 75）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:aac`](./aac.md) | `[Pure-CP]` | 纯 CP AAC ADTS 音频帧流与 AudioSpecificConfig 零分配解析库 |
| [`cap:abc`](./abc.md) | `[Pure-CP]` | 纯 CP ABC Music Notation v2.1 乐谱零分配解析库 |
| [`cap:audio`](./audio.md) | `[Pure-CP]` | cap:audio 是 CP 语言官方内置的纯原生音频编解码与实时语音 AI 信号处理标准库，专为 ASR / Whisper 16kHz PCM 预处理、TTS RIFF/WAVE 音频封装 与 能量语音活动检测（VA |
| [`cap:avif`](./avif.md) | `[Pure-CP]` | 纯 CP AVIF / MIAF ISO BMFF 图像容器零分配解析库 |
| [`cap:barcode`](./barcode.md) | `[Pure-CP]` | cap:barcode 是 CP 语言官方内置的工业级、纯 CP、零 C 依赖一维条码编码与渲染标准库，适用于物流仓储、零售 POS 与票据生成（可与 cap:xml、cap:svg、cap:qrcode、cap:pdf |
| [`cap:bdf`](./bdf.md) | `[Pure-CP]` | 零分配 Adobe BDF 2.1/2.2 点阵字体解析库 |
| [`cap:bezier`](./bezier.md) | `[Pure-CP]` | 2D 二次/三次贝塞尔曲线零分配计算几何库 |
| [`cap:bmp`](./bmp.md) | `[Pure-CP]` | cap:bmp 是 CP 语言官方内置的纯原生 BMP（Windows Bitmap）位图编解码标准库，支持 24-bit BGR、32-bit BGRA 无损编码与解码，以及 8-bit 调色板索引位图与 top-do |
| [`cap:bounded_array`](./bounded_array.md) | `[Bun]` | 栈上定容动态数组 `BoundedArray<T, capacity>` |
| [`cap:bvh`](./bvh.md) | `[Pure-CP]` | Biovision Hierarchy (`.bvh`) 3D 骨骼动捕零分配解析库 |
| [`cap:code128`](./code128.md) | `[Pure-CP]` | Zero-Allocation ISO/IEC 15417 Code 128 Barcode Codec |
| [`cap:color`](./color.md) | `[Pure-CP]` | cap:color 是 CP 语言官方内置的纯原生色彩科学、WCAG 2.1 无障碍对比度评估与终端 ANSI 配色标准库，面向 UI/浏览器渲染引擎、Design Token 调色板生成与 CLI 终端富文本主题系统打 |
| [`cap:dae`](./dae.md) | `[Pure-CP]` | 纯 CP Khronos COLLADA 1.4/1.5 `.dae` 3D 资产零分配解析库 |
| [`cap:dds`](./dds.md) | `[Pure-CP]` | 纯 CP 零分配 Microsoft DDS GPU 纹理头与 Mip 尺寸计算库 |
| [`cap:dls`](./dls.md) | `[Pure-CP]` | 零分配 MMA Downloadable Sounds (`.dls`) RIFF 音色库解析库 |
| [`cap:ean`](./ean.md) | `[Pure-CP]` | ISO/IEC 15420 EAN-13 / EAN-8 / UPC-A 一维条码编解码库 |
| [`cap:ebml`](./ebml.md) | `[Pure-CP]` | 纯 CP RFC 8794 EBML / Matroska MKV / WebM 零分配解析库 |
| [`cap:exif`](./exif.md) | `[Bun]` | JPEG/TIFF EXIF 方向元数据读取器 |
| [`cap:exr`](./exr.md) | `[Pure-CP]` | 纯 CP OpenEXR HDR 图像容器零分配头部与 Attribute 解析库 |
| [`cap:fbx`](./fbx.md) | `[Pure-CP]` | 零分配 Autodesk FBX Binary (7100–7700) 3D 场景树解析库 |
| [`cap:flac`](./flac.md) | `[Pure-CP]` | 纯 CP FLAC 无损音频流与元数据块零分配解析库 |
| [`cap:flv`](./flv.md) | `[Pure-CP]` | 纯 CP Adobe FLV 流媒体容器头与音视频 Tag 零分配解析库 |
| [`cap:gif`](./gif.md) | `[Pure-CP]` | cap:gif 是 CP 语言官方内置的纯原生 GIF87a / GIF89a 静态图与多帧动图编解码标准库，直接复用 cap:lzw（LzwBitOrder.LsbFirst）完成 GIF 子块 LZW 压缩与解压。 |
| [`cap:glsl`](./glsl.md) | `[Pure-CP]` | 纯 CP OpenGL / Vulkan GLSL 着色器零分配词法与声明扫描库 |
| [`cap:gltf`](./gltf.md) | `[Pure-CP]` | 纯 CP glTF 2.0 Binary (GLB) 零分配容器与 Chunk 解析库 |
| [`cap:heif`](./heif.md) | `[Pure-CP]` | 纯 CP HEIF / HEIC ISO BMFF 图像容器零分配解析库 |
| [`cap:icc`](./icc.md) | `[Pure-CP]` | 纯 CP ICC Color Profile 色彩配置文件零分配解析库 |
| [`cap:ico`](./ico.md) | `[Pure-CP]` | 纯 CP 零分配 Windows ICO / CUR 图标容器解析库 |
| [`cap:id3`](./id3.md) | `[Pure-CP]` | 纯 CP MP3 ID3v1 / ID3v2.3 / ID3v2.4 零分配音频元数据解析库 |
| [`cap:image`](./image.md) | `[Bun]` | cap:image 是 CP 语言官方内置的纯原生图像处理标准库，全面吸收自 Bun 图像处理子系统（/data/dev/bun/src/runtime/image/），集 魔数格式探测、多格式编解码（PNG/BMP/G |
| [`cap:image_probe`](./image_probe.md) | `[Bun]` | 零解码极速图片头探测器 |
| [`cap:it`](./it.md) | `[Pure-CP]` | 纯 CP Impulse Tracker (`.it`) 跟踪器音乐零分配解析库 |
| [`cap:jxl`](./jxl.md) | `[Pure-CP]` | 纯 CP 零分配 JPEG XL 裸码流与 ISOBMFF 容器解析库 |
| [`cap:ktx`](./ktx.md) | `[Pure-CP]` | 纯 CP 零分配 Khronos KTX 1.1 & KTX2 GPU 纹理容器解析库 |
| [`cap:midi`](./midi.md) | `[Pure-CP]` | cap:midi 是 CP 语言官方内置的纯原生 Standard MIDI File（SMF Format 0 / 1 / 2）编解码与时间换算标准库，完整支持 7-bit 大端序可变长数量（VLQ）、MIDI Run |
| [`cap:mkv`](./mkv.md) | `[Pure-CP]` | 纯 CP Matroska / WebM EBML 视频容器零分配解析库 |
| [`cap:mp4`](./mp4.md) | `[Pure-CP]` | 纯 CP ISO BMFF / MP4 容器 Box/Atom 零分配解析库 |
| [`cap:mpeg`](./mpeg.md) | `[Pure-CP]` | 纯 CP MPEG-1/2/2.5 Layer I/II/III（MP3）帧头与 Xing/Info/VBRI 零分配解析库 |
| [`cap:noise`](./noise.md) | `[Pure-CP]` | cap:noise 是 CP 语言官方内置的纯原生、零 C 依赖的 确定性相干噪声（Coherent Noise）与程序化分形地形引擎，面向程序化地形生成（配合 cap:pathfinding / cap:ecs）、图形 |
| [`cap:nsf`](./nsf.md) | `[Pure-CP]` | NES Sound Format (`.nsf` / `.nsfe`) 零分配解析库 |
| [`cap:obj`](./obj.md) | `[Pure-CP]` | 纯 CP Wavefront 3D `.obj` 网格零分配解析库 |
| [`cap:ogg`](./ogg.md) | `[Pure-CP]` | 纯 CP RFC 3533 Ogg 音频/多媒体容器零分配分页解析库 |
| [`cap:opus`](./opus.md) | `[Pure-CP]` | 纯 CP RFC 7845 OpusHead/OpusTags 与 RFC 6716 TOC 零分配解析库 |
| [`cap:pcf`](./pcf.md) | `[Pure-CP]` | 零分配 X11 Portable Compiled Format (`.pcf`) 点阵字体解析库 |
| [`cap:perlin`](./perlin.md) | `[Pure-CP]` | Ken Perlin 2002 改进梯度噪声与分形程序化生成库 |
| [`cap:ply`](./ply.md) | `[Pure-CP]` | 纯 CP 零分配 Stanford PLY 3D 点云与网格头解析库 |
| [`cap:png`](./png.md) | `[Pure-CP]` | cap:png 是 CP 语言官方内置的纯原生 PNG（Portable Network Graphics，RFC 2083）图像编解码标准库，基于 cap:deflate（compressZlib、decompress |
| [`cap:pnm`](./pnm.md) | `[Pure-CP]` | 纯 CP 零分配 Netpbm (P1–P6) 图像编解码库 |
| [`cap:psd`](./psd.md) | `[Pure-CP]` | 纯 CP Adobe Photoshop PSD/PSB 容器与 `8BIM` 资源块零分配解析库 |
| [`cap:psf`](./psf.md) | `[Pure-CP]` | Linux Console PSF1 & PSF2 点阵字体零分配解析库 |
| [`cap:qoi`](./qoi.md) | `[Pure-CP]` | 纯 CP 零分配 QOI (Quite OK Image) 无损图像编解码库 |
| [`cap:qr`](./qr.md) | `[Pure-CP]` | 零堆分配 Householder QR 分解与线性最小二乘库 |
| [`cap:qrcode`](./qrcode.md) | `[Pure-CP]` | cap:qrcode 是 CP 语言官方内置的零外部依赖 ISO/IEC 18004 QR Code Model 2 编码与多格式渲染标准库，支持 Version 1..10 Byte 模式编码、GF(256) Reed |
| [`cap:riff`](./riff.md) | `[Pure-CP]` | 纯 CP RIFF / RIFX 二进制块容器零分配解析库 |
| [`cap:s3m`](./s3m.md) | `[Pure-CP]` | Scream Tracker 3 (`.s3m`) 跟踪器音乐零分配解析库 |
| [`cap:scc`](./scc.md) | `[Pure-CP]` | 纯 CP 零堆分配 Tarjan SCC、拓扑排序、DAG 最长路径与 2-SAT 库 |
| [`cap:sdf`](./sdf.md) | `[Pure-CP]` | MDL Molfile V2000 & SDF 结构数据文件零分配解析库 |
| [`cap:sf2`](./sf2.md) | `[Pure-CP]` | 纯 CP SoundFont 2.0 `.sf2` RIFF `sfbk` 音频音色库零分配解析库 |
| [`cap:simplex`](./simplex.md) | `[Pure-CP]` | 纯 CP 零堆分配两阶段线性规划单纯形求解器库 |
| [`cap:smi`](./smi.md) | `[Pure-CP]` | 零分配 Daylight SMILES / OpenSMILES 分子线性表示解析标准库 |
| [`cap:spc`](./spc.md) | `[Pure-CP]` | 零分配 SNES SPC700 (`.spc`) 芯片音乐解析库 |
| [`cap:spline`](./spline.md) | `[Pure-CP]` | Zero-Allocation Cubic Spline, PCHIP & Catmull-Rom Interpolation Library |
| [`cap:srt`](./srt.md) | `[Pure-CP]` | 零分配 SubRip (`.srt`) 字幕解析、时间戳编解码与格式化标准库 |
| [`cap:stl`](./stl.md) | `[Pure-CP]` | 纯 CP 3D STL 二进制与 ASCII 三角网格零分配解析与编码库 |
| [`cap:svg`](./svg.md) | `[Pure-CP]` | cap:svg 是 CP 语言官方内置的纯 CP 矢量 SVG 1.1 绘图画布与声明式统计图表引擎，零 C 依赖、零 std.posix、零 @import("bun")，专为 CP AI Agent 数据分析、可观测 |
| [`cap:tga`](./tga.md) | `[Pure-CP]` | 纯 CP 零分配 Truevision TGA 图像编解码库 |
| [`cap:thumbhash`](./thumbhash.md) | `[Bun]` | ThumbHash 超紧凑图片占位符编解码器 |
| [`cap:tiff`](./tiff.md) | `[Pure-CP]` | 纯 CP Baseline TIFF 图像编解码库 |
| [`cap:ttf`](./ttf.md) | `[Pure-CP]` | 纯 CP TrueType / OpenType SFNT 零分配字体表解析库 |
| [`cap:tween`](./tween.md) | `[Pure-CP]` | cap:tween 是 CP 语言官方内置的纯原生、零 C 依赖的 运动学缓动插值、关键帧轨道与弹簧阻尼物理动画引擎，面向 UI 动效（配合 cap:tui / cap:svg）、游戏实体运动（配合 cap:ecs）与连 |
| [`cap:usd`](./usd.md) | `[Pure-CP]` | 零分配 Pixar OpenUSD (`.usdc` / `.usda` / `.usdz`) 容器解析库 |
| [`cap:vgm`](./vgm.md) | `[Pure-CP]` | Video Game Music (`.vgm`) 1.00–1.71 零分配解析库 |
| [`cap:vrml`](./vrml.md) | `[Pure-CP]` | VRML97 & Classic X3D 3D 场景零分配解析库 |
| [`cap:vtt`](./vtt.md) | `[Pure-CP]` | 纯 CP W3C WebVTT 字幕与时间轴零分配解析/格式化库 |
| [`cap:wav`](./wav.md) | `[Pure-CP]` | cap:wav 是 CP 语言官方内置的纯原生 RIFF/WAVE 内存字节流编解码与基础音频处理标准库，支持 Pcm8、Pcm16、Pcm24、Pcm32 与 Float32 全格式无损归一化读写、未知扩展块跳过（含奇 |
| [`cap:wgsl`](./wgsl.md) | `[Pure-CP]` | 纯 CP WebGPU WGSL 着色器零分配词法扫描与入口点/绑定提取库 |
| [`cap:woff2`](./woff2.md) | `[Pure-CP]` | 纯 CP W3C WOFF2 网页字体二进制头与表目录零分配解析库 |
| [`cap:x3d`](./x3d.md) | `[Pure-CP]` | Web3D X3D 3.x/4.0 XML 3D 场景零分配解析库 |
| [`cap:xcf`](./xcf.md) | `[Pure-CP]` | 纯 CP GIMP XCF 分层图像零分配解析库 |
| [`cap:xm`](./xm.md) | `[Pure-CP]` | 纯 CP FastTracker II Extended Module (`.xm`) 跟踪器音乐零分配解析库 |

## 10. 编译原理、构建系统与开发者工具链

*Compilers, Build Systems & DevTools* — 共 **60** 个模块（`[Bun]`: 29，`[Pure-CP]`: 31）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:bench`](./bench.md) | `[Bun]` | cap:bench 整合自 Bun src/perf/ 性能剖析引擎与工业生产级微基准统计对比/内存追踪套件，全部核心运行期路径均采用零额外干扰与低开销布局： |
| [`cap:btf`](./btf.md) | `[Pure-CP]` | Linux BPF Type Format (`.BTF`) Zero-Allocation Parser |
| [`cap:bundle_options`](./bundle_options.md) | `[Pure-CP]` | 纯 CP 打包格式、缓存策略与链接模式枚举引擎（吸收自 Bun `options_types`） |
| [`cap:bundler`](./bundler.md) | `[Bun]` | cap:bundler 是 CP 语言官方内置的模块打包器（Bundler）核心模型标准库，提炼自 Bun 原生 Bundler V2、Chunk 拓扑调度、CompileTarget 交叉编译目标解析与 Standal |
| [`cap:caddy`](./caddy.md) | `[Pure-CP]` | 纯 CP Caddyfile 零分配词法扫描与站点块解析库 |
| [`cap:char_freq`](./char_freq.md) | `[Bun]` | 标识符字符频率统计与压缩短名生成器 |
| [`cap:ci_info`](./ci_info.md) | `[Bun]` | 纯 CP 零堆分配 CI/CD 环境与 PR 构建检测库（遵循 ci-info v4 规范），吸收自 /data/dev/bun/src/cli/ci_info.cp 与 codegen/ci_info.ts。 |
| [`cap:clap`](./clap.md) | `[Bun]` | 编译期强类型与零分配流式命令行参数解析标准库 |
| [`cap:cli`](./cli.md) | `[Bun]` | cap:cli 提供零堆分配的完整终端交互工具链： |
| [`cap:coff`](./coff.md) | `[Pure-CP]` | Microsoft COFF / PE-COFF `.obj` Zero-Allocation Parser |
| [`cap:command_tag`](./command_tag.md) | `[Pure-CP]` | 纯 CP CLI 子命令分类与配置加载策略引擎（吸收自 Bun `CommandTag`） |
| [`cap:compile_target`](./compile_target.md) | `[Bun]` | 跨平台构建与分发目标三元组解析库 |
| [`cap:coverage_options`](./coverage_options.md) | `[Pure-CP]` | 纯 CP 代码覆盖率配置、阈值判定与 LCOV 生成器（吸收自 Bun `CodeCoverageOptions`） |
| [`cap:crontab`](./crontab.md) | `[Pure-CP]` | 纯 CP 零堆 Cron 表达式解析与下次触发计算引擎 |
| [`cap:dex`](./dex.md) | `[Pure-CP]` | 纯 CP Android Dalvik Executable (DEX) 零分配解析库 |
| [`cap:docker`](./docker.md) | `[Bun]` | OCI / Docker Image Reference & Dockerfile Parser |
| [`cap:dwarf`](./dwarf.md) | `[Pure-CP]` | DWARF v4/v5 调试信息解析标准库 |
| [`cap:editorconfig`](./editorconfig.md) | `[Bun]` | `.editorconfig` Parser & Property Resolver |
| [`cap:elf`](./elf.md) | `[Bun]` | 跨平台可执行二进制格式嗅探与完整 ELF64 / Mach-O / PE 解析标准库 |
| [`cap:env_path`](./env_path.md) | `[Bun]` | 跨平台 `PATH` 环境变量构建器与零分配迭代器 |
| [`cap:git`](./git.md) | `[Bun]` | cap:git 是 CP 语言官方内置的零外部依赖 Git 底层原语库，专为 AI Coding Agent、CI 工具链与包管理器提供 Git 对象 SHA-1 内容寻址哈希与二进制编解码、完整 .gitignore  |
| [`cap:haproxy`](./haproxy.md) | `[Pure-CP]` | 纯 CP 零分配 HAProxy `.cfg` 负载均衡配置解析库 |
| [`cap:highlight`](./highlight.md) | `[Pure-CP]` | cap:highlight 是 CP 语言官方内置的纯 CP 多语言代码词法高亮标准库，零 C 依赖、零 std.posix、零 @import("bun")，专为 CP CLI 代码审查、文档静态生成（配合 cap:m |
| [`cap:hosted_git_info`](./hosted_git_info.md) | `[Bun]` | 纯 CP 托管 Git 平台 URL 规范化解析与多格式生成器 |
| [`cap:hprof`](./hprof.md) | `[Pure-CP]` | 零分配 JVM HPROF 二进制堆转储解析库 |
| [`cap:idl`](./idl.md) | `[Pure-CP]` | 纯 CP 零分配 W3C WebIDL 词法扫描与顶层声明提取库 |
| [`cap:intl`](./intl.md) | `[Pure-CP]` | cap:intl 是 CP 语言官方内置的纯原生国际化数值格式化与双向单位解析标准库，专为 CLI 仪表盘、配置文件大小/时长解析（如 "1.5 GiB"、"2h30m"）与多地区数值/百分比/序数词/相对时间展示打造。 |
| [`cap:jfr`](./jfr.md) | `[Pure-CP]` | 零分配 JVM Java Flight Recorder (`.jfr`) 解析库 |
| [`cap:js`](./js.md) | `[Bun]` | JavaScript 词法标识符与运算符元信息标准库 |
| [`cap:junit`](./junit.md) | `[Pure-CP]` | JUnit XML CI 测试报告零分配解析与汇总库 |
| [`cap:lcov`](./lcov.md) | `[Pure-CP]` | 纯 CP LCOV `.info` 代码覆盖率零分配流式解析库 |
| [`cap:lcov_report`](./lcov_report.md) | `[Bun]` | 零堆分配 LCOV 代码覆盖率解析与序列化库 |
| [`cap:linear_fifo`](./linear_fifo.md) | `[Bun]` | 纯 CP 零堆分配泛型环形缓冲区 FIFO（StaticLinearFifo(T, capacity)），吸收自 /data/dev/bun/src/collections/linear_fifo.cp。 |
| [`cap:macho`](./macho.md) | `[Pure-CP]` | 纯 CP Apple Mach-O 32/64-bit 与 Fat Universal 二进制解析库 |
| [`cap:makefile_parse`](./makefile_parse.md) | `[Bun]` | GNU `Makefile` 目标、`.PHONY` 与变量零分配解析库 |
| [`cap:nginx`](./nginx.md) | `[Pure-CP]` | 纯 CP 零分配 Nginx `.conf` 词法扫描与块解析校验库 |
| [`cap:path`](./path.md) | `[Bun]` | 跨平台工业级路径解析与缓冲区池标准库 |
| [`cap:path_resolve`](./path_resolve.md) | `[Bun]` | 纯 CP 零堆分配 POSIX 路径规范化、拼接、相对路径、最长公共目录前缀与防穿越校验库，吸收自 /data/dev/bun/src/paths/resolve_path.cp。 |
| [`cap:paths`](./paths.md) | `[Pure-CP]` | cap:paths 是 CP 语言官方内置的纯 CP 跨平台路径解析、规范化与安全边界校验库，零 C 依赖、零 std.posix、零 @import("bun")。核心解析函数（parse、dirname、basena |
| [`cap:pdb`](./pdb.md) | `[Pure-CP]` | RCSB Protein Data Bank (`.pdb`) 零分配 3D 结构解析库 |
| [`cap:pkg`](./pkg.md) | `[Bun]` | CP 工业级包管理核心标准库文档 (`lib/pkg/`) |
| [`cap:resolver`](./resolver.md) | `[Bun]` | cap:resolver 是 CP 语言官方内置的高性能模块解析引擎标准库，完整吸收自 Bun src/resolver/（data_url.zig、package_json.zig、tsconfig_json.zig、 |
| [`cap:rpc`](./rpc.md) | `[Pure-CP]` | cap:rpc 是 CP 语言官方内置的纯 CP 协议引擎标准库（零 std.posix、零 @import("bun")），专为百万级 AI Agent 工具调用、LSP / DAP 语言服务器与分布式 RPC 打造。 |
| [`cap:sarif`](./sarif.md) | `[Pure-CP]` | 零分配 OASIS SARIF 2.1.0 静态代码分析结果解析标准库 |
| [`cap:semver`](./semver.md) | `[Bun]` | Semver 2.0.0 语义化版本解析与区间查询引擎 |
| [`cap:shell`](./shell.md) | `[Bun]` | 嵌入式跨平台 Shell 词法/AST 引擎与进程内/外混合管道执行器 |
| [`cap:sourcemap`](./sourcemap.md) | `[Bun]` | cap:sourcemap 是 CP 语言官方内置的工业级 SourceMap v3 (ECMA-426) 源码位置映射、Base64 VLQ 变长编解码、行偏移 $O(\log N)$ 二分定位、MappingList |
| [`cap:sourcemap_index`](./sourcemap_index.md) | `[Bun]` | 纯 CP 零堆分配 SourceMap v3 Base64-VLQ mappings 解码器与 O(log N) 二分查找定位器，吸收自 /data/dev/bun/src/sourcemap/Mapping.cp。 |
| [`cap:spell`](./spell.md) | `[Pure-CP]` | cap:spell 是 CP 语言官方内置的纯 CP 高速拼写纠错引擎，零 C 依赖、零 std.posix、零 @import("bun")，专为 CP 搜索引擎「Did you mean」、CLI 参数智能纠错与 A |
| [`cap:sse_parser`](./sse_parser.md) | `[Bun]` | WHATWG Server-Sent Events (`text/event-stream`) 解析与格式化库 |
| [`cap:tap`](./tap.md) | `[Pure-CP]` | 零分配 TAP v12/v13/v14 测试协议解析与汇总标准库 |
| `cap:test` | `[Pure-CP]` | cap:test harness, per CHANGELOG |
| [`cap:test_runner`](./test_runner.md) | `[Bun]` | cap:test_runner 是零 JSC 耦合的纯 CP 生产级测试套件编排、快照管理、值格式化与 CI 报告引擎，完整直迁自 Bun src/test_runner/ 核心无 JS 引擎逻辑（Order.cp、Co |
| [`cap:trx`](./trx.md) | `[Pure-CP]` | 零分配 Visual Studio / .NET `.trx` 测试结果解析标准库 |
| [`cap:ts`](./ts.md) | `[Pure-CP]` | 纯 CP ISO/IEC 13818-1 MPEG-TS 传输流零分配解析库 |
| [`cap:tsconfig_paths`](./tsconfig_paths.md) | `[Bun]` | 纯 CP 零堆分配 TSConfig compilerOptions.paths 通配符路径映射解析器与 ${configDir} 模板展开器，吸收自 /data/dev/bun/src/resolver/tsconfi |
| [`cap:wasm`](./wasm.md) | `[Pure-CP]` | cap:wasm 是 CP 语言官方内置的 WebAssembly 1.0 二进制模块解析、校验与构造标准库，专为沙盒插件内省、WASM 工具模块验证与二进制编码打造。零外部依赖、零 std.posix、零 @impor |
| [`cap:wasm_fmt`](./wasm_fmt.md) | `[Pure-CP]` | WebAssembly `.wasm` 二进制解析与构建标准库 |
| [`cap:wat`](./wat.md) | `[Pure-CP]` | 零分配 WebAssembly Text (`.wat`) 词法器、导出提取器与结构校验器 |
| [`cap:which`](./which.md) | `[Bun]` | 跨平台可执行二进制查找与权限校验标准库 |

## 11. 数值计算、线性代数与微积分

*Numerical Analysis, Linear Algebra & Calculus* — 共 **36** 个模块（`[Bun]`: 2，`[Pure-CP]`: 34）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:anneal`](./anneal.md) | `[Pure-CP]` | 零堆分配模拟退火全局优化库（Simulated Annealing） |
| [`cap:bessel`](./bessel.md) | `[Pure-CP]` | 纯 CP 零堆分配柱贝塞尔与修正贝塞尔特殊函数库 |
| [`cap:bfgs`](./bfgs.md) | `[Pure-CP]` | 零堆分配拟牛顿法与梯度优化库 |
| [`cap:bigint`](./bigint.md) | `[Bun]` | cap:bigint 为 CP 提供双轨高精度整数计算引擎： |
| [`cap:builtin_modules`](./builtin_modules.md) | `[Bun]` | 纯 CP Node.js 与 Bun 内置模块全量注册表与目标解析器 |
| [`cap:cholesky`](./cholesky.md) | `[Pure-CP]` | SPD Cholesky ($L L^T$) 与无开方 $L D L^T$ 分解、求解与求逆库 |
| [`cap:code39`](./code39.md) | `[Pure-CP]` | ISO/IEC 16388 Code 39 一维条码编解码与 Mod-43 校验库 |
| [`cap:codec`](./codec.md) | `[Pure-CP]` | cap:codec 是 CP 语言官方内置的纯 CP 多进制与校验编码标准库，零 C 依赖、零 std.posix、零 @import("bun")，既支持 import { ... } from "cap:codec" |
| [`cap:csr`](./csr.md) | `[Pure-CP]` | 零分配 RFC 2986 / PKCS#10 证书签发请求 (CSR) DER 解析库 |
| [`cap:dct`](./dct.md) | `[Pure-CP]` | Zero-Allocation 1D/2D 8×8 DCT-II / IDCT-III, ZigZag & JPEG Quantization DSP Library |
| [`cap:decimal`](./decimal.md) | `[Pure-CP]` | cap:decimal 是 CP 语言官方内置的纯原生 128 位定点十进制算术与 ISO 4217 货币结算标准库，彻底消除 IEEE 754 二进制浮点精度漂移（0.1 + 0.2 == 0.3 精确成立），专为金融 |
| [`cap:decimal_fp`](./decimal_fp.md) | `[Pure-CP]` | 工业级金融定点数精确算术引擎 |
| [`cap:fft`](./fft.md) | `[Pure-CP]` | cap:fft 是 CP 语言官方内置的纯原生、零 C 依赖的 Cooley-Tukey Radix-2 快速傅里叶变换（FFT / IFFT）、经典窗函数、单边实数幅值谱（Parseval 能量守恒）、主频提取与频域快 |
| [`cap:ga`](./ga.md) | `[Pure-CP]` | 纯 CP 零堆分配遗传算法与演化计算库 |
| [`cap:gamma`](./gamma.md) | `[Pure-CP]` | 纯 CP 零堆分配特殊数学函数库 |
| [`cap:gf256`](./gf256.md) | `[Pure-CP]` | Zero-Allocation Galois Field $GF(2^8)$ Arithmetic, Polynomial & Lagrange Library |
| [`cap:golay`](./golay.md) | `[Pure-CP]` | 扩展二元戈莱码 [24, 12, 8] & 完美戈莱码 [23, 12, 7] |
| [`cap:hamming`](./hamming.md) | `[Pure-CP]` | Zero-Allocation Hamming(7,4), SECDED(8,4) & SECDED(72,64) ECC Library |
| [`cap:idna`](./idna.md) | `[Pure-CP]` | RFC 3492 Punycode 与 IDNA 国际化域名标准库 |
| [`cap:interval`](./interval.md) | `[Pure-CP]` | cap:interval 是 CP 语言官方内置的纯 CP 半开区间 [start, end) 代数、规范化不相交区间集合（IntervalSet）与 AVL 平衡增强区间树（IntervalTree<V>），零 C 依 |
| [`cap:linalg`](./linalg.md) | `[Pure-CP]` | cap:linalg 是 CP 语言官方内置的纯原生、零 C 依赖的 通用任意维度（$M \times N$）稠密矩阵与数值线性代数引擎，面向科学计算、AI 数值优化、图形变换与控制工程场景设计。 |
| [`cap:lu`](./lu.md) | `[Pure-CP]` | 零分配 Doolittle 部分选主元 LU 分解与三对角 Thomas 求解库 |
| [`cap:lucene`](./lucene.md) | `[Pure-CP]` | 零分配 Apache Lucene `CodecUtil` 索引文件解析库 |
| [`cap:math`](./math.md) | `[Pure-CP]` | cap:math 是 CP 语言官方内置的纯原生高性能数值计算与 AI 向量检索算子库，覆盖 2D/3D/4D 向量与列主序 4x4 矩阵线性代数、Welford 数值稳定描述统计/分位数/皮尔逊相关与最小二乘线性回归， |
| [`cap:nelder`](./nelder.md) | `[Pure-CP]` | 零堆分配无导数优化与一维极值/求根库 |
| [`cap:newton`](./newton.md) | `[Pure-CP]` | 纯 CP 零分配非线性方程组与 Levenberg-Marquardt 最小二乘库 |
| [`cap:node_builtins`](./node_builtins.md) | `[Pure-CP]` | 纯 CP Node.js & Bun 内建模块映射注册表（吸收自 Bun `resolve_builtins`） |
| [`cap:ode`](./ode.md) | `[Pure-CP]` | Zero-Allocation ODE Initial-Value Problem Solvers (RK2, RK4, Dormand-Prince RK4(5), Velocity Verlet) |
| [`cap:olc`](./olc.md) | `[Pure-CP]` | Zero-Allocation Google Open Location Code (Plus Codes) Geocoder |
| [`cap:optimize`](./optimize.md) | `[Pure-CP]` | cap:optimize 是 CP 语言官方内置的纯原生、零 C 依赖的 数值求根与多维函数最优化引擎，面向科学仿真、参数标定、物理反演与轻量机器学习场景设计。 |
| [`cap:polyfit`](./polyfit.md) | `[Pure-CP]` | Zero-Allocation Polynomial Least-Squares Fitting, Ridge Regression & Root Finding |
| [`cap:pso`](./pso.md) | `[Pure-CP]` | 零堆分配全局启发式优化库（PSO & DE） |
| [`cap:quadrature`](./quadrature.md) | `[Pure-CP]` | 纯 CP 零堆分配高精度数值积分库 |
| [`cap:reed_solomon`](./reed_solomon.md) | `[Pure-CP]` | GF(2^8) Cauchy Reed-Solomon 纠删码标准库 |
| [`cap:svd`](./svd.md) | `[Pure-CP]` | 纯 CP 零堆分配奇异值分解与 Moore-Penrose 广义逆库 |
| [`cap:union_interval`](./union_interval.md) | `[Pure-CP]` | 纯 CP 不相交区间合并集合 (`IntervalSet`) |

## 12. 概率统计、机器学习与 AI Agent

*Statistics, Machine Learning & AI Agents* — 共 **44** 个模块（`[Bun]`: 1，`[Pure-CP]`: 43）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:a2a`](./a2a.md) | `[Pure-CP]` | Agent-to-Agent (A2A) 多智能体协作协议标准库 |
| [`cap:agentmem`](./agentmem.md) | `[Pure-CP]` | cap:agentmem 是 CP 语言官方内置的纯原生、零 C 依赖的分层长期记忆引擎（Hierarchical Agent Memory），专为百万级 AI Agent 运行时的上下文记忆管理、经验巩固与记忆淘汰打造 |
| [`cap:attention`](./attention.md) | `[Pure-CP]` | Transformer Attention, KV-Cache & Token Sampler |
| [`cap:bandit`](./bandit.md) | `[Pure-CP]` | 纯 CP 零堆分配多臂老虎机在线决策库 |
| [`cap:bm25`](./bm25.md) | `[Pure-CP]` | Okapi BM25 全文检索与 RRF 混合融合标准库 |
| [`cap:bpe`](./bpe.md) | `[Pure-CP]` | Byte-Pair Encoding (BPE) LLM Tokenizer & Token Budget Engine |
| [`cap:cusum`](./cusum.md) | `[Pure-CP]` | 零堆分配在线变点检测与流式异常检测库 |
| [`cap:dbscan`](./dbscan.md) | `[Pure-CP]` | 零分配基于密度的空间聚类库（DBSCAN） |
| [`cap:dedup`](./dedup.md) | `[Pure-CP]` | cap:dedup 是 CP 语言官方内置的纯 CP 近重复文档指纹去重引擎，零 C 依赖、零 std.posix、零 @import("bun")，为 AI RAG 语料清洗、网页爬虫去重（配合 cap:feed /  |
| [`cap:dtree`](./dtree.md) | `[Pure-CP]` | 纯 CP 零堆分配 CART 决策树分类与回归库 |
| [`cap:dtw`](./dtw.md) | `[Pure-CP]` | 零堆分配动态时间规整（DTW）与时间序列对齐库 |
| [`cap:ecs`](./ecs.md) | `[Pure-CP]` | cap:ecs 是 CP 语言官方内置的纯原生、零 C 依赖的 Sparse-Set 架构实体组件系统（Entity Component System），面向高性能游戏循环、多 Agent 仿真世界与物理引擎场景设计。 |
| [`cap:ekf`](./ekf.md) | `[Pure-CP]` | 零堆分配非线性状态估计库（EKF & UKF） |
| [`cap:eval`](./eval.md) | `[Pure-CP]` | cap:eval 是 CP 语言官方内置的工业生产级纯 CP AI Agent 与 LLM 评测标准库，零外部依赖、零 std.posix、零 @import("bun")，可与 cap:json、cap:text 无缝 |
| [`cap:expr`](./expr.md) | `[Pure-CP]` | cap:expr 是 CP 语言官方内置的纯 CP 零副作用沙盒表达式与公式求值引擎，专为 AI Agent 安全护栏、特性开关（Feature Flags）、动态计费/配额公式 与 规则策略过滤 打造。 |
| [`cap:gmm`](./gmm.md) | `[Pure-CP]` | 零堆分配高斯混合模型（GMM / EM）聚类与密度估计库 |
| [`cap:guardrail`](./guardrail.md) | `[Pure-CP]` | cap:guardrail 是 CP 语言官方内置的纯 CP、零 C 依赖 AI Agent 安全护栏（Guardrails）引擎，面向百万级 Agent 并发运行时提供高精度的敏感信息扫描脱敏与多级输入/输出策略审查。 |
| [`cap:header_value_iter`](./header_value_iter.md) | `[Bun]` | RFC 9110 HTTP 多值头部与参数迭代器 |
| [`cap:hlc`](./hlc.md) | `[Pure-CP]` | Zero-Allocation Hybrid Logical Clock (HLC) |
| [`cap:hmm`](./hmm.md) | `[Pure-CP]` | 纯 CP 零堆分配离散隐马尔可夫模型（HMM）库 |
| [`cap:hnsw`](./hnsw.md) | `[Pure-CP]` | HNSW 近似最近邻向量图索引标准库 |
| [`cap:holt`](./holt.md) | `[Pure-CP]` | 纯 CP 零分配时间序列指数平滑与预测库 |
| [`cap:huber`](./huber.md) | `[Pure-CP]` | 纯 CP 零堆分配鲁棒统计与 M-估计回归库 |
| [`cap:isotonic`](./isotonic.md) | `[Pure-CP]` | 零堆分配保序回归（PAVA）与概率校准库 |
| [`cap:kalman`](./kalman.md) | `[Pure-CP]` | 1D/2D 卡尔曼滤波器与 Alpha-Beta 跟踪滤波器 |
| [`cap:kmeans`](./kmeans.md) | `[Pure-CP]` | 纯 CP 零堆分配 K-Means++ 聚类库 |
| [`cap:knn`](./knn.md) | `[Pure-CP]` | 零堆分配 K-最近邻（KNN）分类与回归库 |
| [`cap:lasso`](./lasso.md) | `[Pure-CP]` | 纯 CP 零堆分配稀疏正则化回归库（Lasso / Ridge / ElasticNet） |
| [`cap:logreg`](./logreg.md) | `[Pure-CP]` | 纯 CP 零分配逻辑回归库 |
| [`cap:markov`](./markov.md) | `[Pure-CP]` | 零分配离散时间马尔可夫链（DTMC）分析与采样库 |
| [`cap:mcts`](./mcts.md) | `[Pure-CP]` | 零分配蒙特卡洛树搜索（MCTS / UCT & PUCT）决策规划库 |
| [`cap:mds`](./mds.md) | `[Pure-CP]` | 零堆分配多维尺度分析（Classical MDS & SMACOF）降维嵌入库 |
| [`cap:naivebayes`](./naivebayes.md) | `[Pure-CP]` | 纯 CP 零堆分配朴素贝叶斯分类库 |
| [`cap:nmf`](./nmf.md) | `[Pure-CP]` | 纯 CP 零分配非负矩阵分解（NMF）库 |
| [`cap:particle`](./particle.md) | `[Pure-CP]` | 纯 CP 零堆分配序贯蒙特卡洛 / 粒子滤波库 |
| [`cap:pca`](./pca.md) | `[Pure-CP]` | 零分配主成分分析（PCA）降维与对称矩阵 Jacobi 特征值分解库 |
| [`cap:prob`](./prob.md) | `[Pure-CP]` | cap:prob 是 CP 语言官方内置的纯原生、零 C 依赖的 概率分布（PDF/PMF/CDF/PPF 逆分布）、确定性伪随机采样器（Rng）与统计假设检验（比例 Z-test / 均值 Z-test / 95% 置 |
| [`cap:prompt`](./prompt.md) | `[Pure-CP]` | LLM 上下文窗口管理与结构化输出提取引擎 |
| [`cap:quant`](./quant.md) | `[Pure-CP]` | AI Embedding 向量量化标准库 |
| [`cap:rego`](./rego.md) | `[Pure-CP]` | 纯 CP Open Policy Agent (OPA) Rego 策略零分配词法扫描与校验库 |
| [`cap:spectral`](./spectral.md) | `[Pure-CP]` | 纯 CP 零分配谱图聚类与图拉普拉斯分析库 |
| [`cap:svm`](./svm.md) | `[Pure-CP]` | 纯 CP 零堆分配支持向量机（SVM）分类库 |
| [`cap:tensor`](./tensor.md) | `[Pure-CP]` | 工业级多维张量与 AI 算子标准库 |
| [`cap:vector`](./vector.md) | `[Pure-CP]` | cap:vector 是 CP 语言官方内置的纯原生本地语义向量检索与混合 RAG（Retrieval-Augmented Generation）搜索引擎库，专为百万级 AI Agent 本地记忆检索、知识库召回与混合重 |

## 13. 信号处理、频谱分析与自动控制

*Signal Processing, Spectral Analysis & Control* — 共 **8** 个模块（`[Bun]`: 0，`[Pure-CP]`: 8）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:biquad`](./biquad.md) | `[Pure-CP]` | 纯 CP 零分配音频/DSP Biquad IIR 滤波器与 Goertzel 单频检测库 |
| [`cap:fir`](./fir.md) | `[Pure-CP]` | 数字信号处理 FIR 滤波器、窗函数与 RBJ Biquad IIR 滤波器 |
| [`cap:hilbert`](./hilbert.md) | `[Pure-CP]` | 2D Hilbert 空间填充曲线与 2D/3D Morton Z-Order 空间索引库 |
| [`cap:iir`](./iir.md) | `[Pure-CP]` | 纯 CP 零堆分配经典 IIR 滤波器设计库 |
| [`cap:leef`](./leef.md) | `[Pure-CP]` | 零分配 IBM QRadar LEEF 1.0/2.0 SIEM 日志解析库 |
| [`cap:pid`](./pid.md) | `[Pure-CP]` | 工业离散 PID 控制器零分配库 |
| [`cap:savitzky`](./savitzky.md) | `[Pure-CP]` | 纯 CP 零分配 Savitzky-Golay 平滑/微分与 LOESS 回归库 |
| [`cap:wavelet`](./wavelet.md) | `[Pure-CP]` | 离散小波变换 DWT / IDWT（Haar、Daubechies-4、JPEG 2000 CDF 5/3） |

## 14. 图论、计算几何与地理空间（GIS）

*Graphs, Computational Geometry & Geospatial* — 共 **37** 个模块（`[Bun]`: 0，`[Pure-CP]`: 37）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:ais`](./ais.md) | `[Pure-CP]` | 纯 CP ITU-R M.1371 海事船舶 AIS 6-bit 装甲比特流零分配解码库 |
| [`cap:astar`](./astar.md) | `[Pure-CP]` | 2D 网格 A* 最短路径与视线检测零分配库 |
| [`cap:bipartite`](./bipartite.md) | `[Pure-CP]` | 零堆分配二分图与 Hopcroft-Karp 最大匹配库 |
| [`cap:delaunay`](./delaunay.md) | `[Pure-CP]` | 纯 CP 零堆分配 2D Delaunay 三角剖分库 |
| [`cap:dijkstra`](./dijkstra.md) | `[Pure-CP]` | 零分配图最短路径算法库（Dijkstra / Bellman-Ford / Floyd-Warshall） |
| [`cap:earcut`](./earcut.md) | `[Pure-CP]` | 2D 简单多边形耳切法三角剖分与计算几何库 |
| [`cap:geo`](./geo.md) | `[Pure-CP]` | cap:geo 是 CP 语言官方内置的纯原生地理空间计算与 Geohash 邻近检索标准库，专为 LBS 位置服务、地理围栏（Geofencing）、网格索引与半径/最近邻查询打造。全模块零 C 依赖、零 std.po |
| [`cap:geo_quadtree`](./geo_quadtree.md) | `[Pure-CP]` | 纯 CP 2D 空间四叉树索引引擎 |
| [`cap:geohash`](./geohash.md) | `[Pure-CP]` | cap:geohash 是 CP 语言官方内置的纯 CP Geohash 空间网格编码、8 邻域拓扑与 Haversine 球面距离标准库，零 C 依赖、零 std.posix、零 @import("bun")。 |
| [`cap:geohash_3d`](./geohash_3d.md) | `[Pure-CP]` | 纯 CP 3D Morton Z-Order 空间编码与球面测距引擎 |
| [`cap:geojson`](./geojson.md) | `[Pure-CP]` | cap:geojson 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 RFC 7946 GeoJSON 解析序列化、包围盒计算、含孔洞多边形地理围栏判定与 Douglas-Peucker 折线抽稀引擎（基 |
| [`cap:geom2d`](./geom2d.md) | `[Pure-CP]` | cap:geom2d 是 CP 语言官方内置的纯原生、零 C 依赖的 2D 平面计算几何、凸包（Convex Hull）、线段相交与碰撞检测引擎，面向图形渲染（配合 cap:svg）、游戏物理、CAD 与机器人路径规划场 |
| [`cap:gjk`](./gjk.md) | `[Pure-CP]` | 2D GJK 凸多边形碰撞检测与 SAT MTV 物理库 |
| [`cap:gpx`](./gpx.md) | `[Pure-CP]` | 纯 CP GPX 1.0/1.1 航点/轨迹 XML 零分配解析与距离统计库 |
| [`cap:graph`](./graph.md) | `[Pure-CP]` | cap:graph 是 CP 语言官方内置的纯原生图论数据结构与算法库，同时提供： |
| [`cap:hungarian`](./hungarian.md) | `[Pure-CP]` | 纯 CP 零堆分配线性指派与二分图最优匹配库 |
| [`cap:igc`](./igc.md) | `[Pure-CP]` | FAI IGC 航空/滑翔机飞行记录仪零分配解析库 |
| [`cap:kdtree`](./kdtree.md) | `[Pure-CP]` | cap:kdtree 是 CP 语言官方内置的纯原生、零 C 依赖的 多维 K-D Tree 空间索引引擎（KdTree<V>），支持任意正整数维度（dims >= 1），面向高维向量检索、空间最近邻匹配、$K$ 近邻（ |
| [`cap:kml`](./kml.md) | `[Pure-CP]` | 纯 CP OGC KML 2.2 地理标记零分配解析库 |
| [`cap:las`](./las.md) | `[Pure-CP]` | 零分配 ASPRS LAS 1.2–1.4 激光雷达 3D 点云解析库 |
| [`cap:louvain`](./louvain.md) | `[Pure-CP]` | 零堆分配 Louvain 模块度社区发现库 |
| [`cap:maxflow`](./maxflow.md) | `[Pure-CP]` | 零堆分配网络流图算法库 |
| [`cap:mst`](./mst.md) | `[Pure-CP]` | 零堆分配最小生成树与生成森林算法库 |
| [`cap:nmea`](./nmea.md) | `[Pure-CP]` | 零分配 NMEA 0183 GPS/GNSS 导航语句与帧流解析库 |
| [`cap:octree`](./octree.md) | `[Pure-CP]` | 零堆分配 3D 空间八叉树索引库 |
| [`cap:pagerank`](./pagerank.md) | `[Pure-CP]` | CSR 稀疏有向图 PageRank、个性化 PPR 与 HITS 中心性零分配库 |
| [`cap:pathfinding`](./pathfinding.md) | `[Pure-CP]` | cap:pathfinding 是 CP 语言官方内置的纯原生 2D 网格地图寻路与空间规划引擎，专为机器人导航、游戏 AI 与多 Agent 空间规划打造。 |
| [`cap:quadtree`](./quadtree.md) | `[Pure-CP]` | cap:quadtree 是 CP 语言官方内置的纯原生、零 C 依赖的 2D 动态分裂/合并四叉树空间索引引擎（QuadTree），面向 2D 游戏 AOI（Area of Interest）、地图 POI 检索、粒子 |
| [`cap:rrt`](./rrt.md) | `[Pure-CP]` | 零堆分配 2D 采样运动规划库（RRT & RRT*） |
| [`cap:rtree`](./rtree.md) | `[Pure-CP]` | cap:rtree 是 CP 语言官方内置的纯 CP 二维轴对齐包围盒（2D AABB）动态 R-Tree 空间索引引擎，零 C 依赖、零 std.posix、零 @import("bun")。采用节点索引池（std.A |
| [`cap:sat`](./sat.md) | `[Pure-CP]` | cap:sat 是 CP 语言官方内置的纯 CP 布尔可满足性（SAT）求解器与高级约束建模引擎，零 C 依赖、零 std.posix、零 @import("bun")，专为 CP 包依赖冲突解析、任务排班调度、配置合规 |
| [`cap:segy`](./segy.md) | `[Pure-CP]` | 零分配 SEG-Y Rev 1/2 地球物理地震反射解析标准库 |
| [`cap:shp`](./shp.md) | `[Pure-CP]` | 纯 CP ESRI Shapefile (`.shp`) 零分配解析库 |
| [`cap:topojson`](./topojson.md) | `[Pure-CP]` | 零分配 TopoJSON v1.0 拓扑压缩几何解析库 |
| [`cap:voronoi`](./voronoi.md) | `[Pure-CP]` | 零分配 2D 有界 Voronoi 图与 Lloyd 松弛库 |
| [`cap:wkb`](./wkb.md) | `[Pure-CP]` | 纯 CP OGC WKB / PostGIS EWKB 空间几何二进制零分配编解码库 |
| [`cap:wkt`](./wkt.md) | `[Pure-CP]` | cap:wkt 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 OGC Well-Known Text (WKT) 解析/格式化与 ISO Well-Known Binary (WKB) 双向二进制编解码引 |

## 15. 工控、硬件固件与垂直行业协议

*Industrial, Embedded, Firmware & Domain Protocols* — 共 **38** 个模块（`[Bun]`: 1，`[Pure-CP]`: 37）

| 模块 | 来源 | 职责摘要 |
| :--- | :---: | :--- |
| [`cap:acpi`](./acpi.md) | `[Pure-CP]` | ACPI 6.5 RSDP / SDT / RSDT / XSDT / MADT / MCFG 零分配解析库 |
| [`cap:bacnet`](./bacnet.md) | `[Pure-CP]` | ASHRAE 135 BACnet/IP 零分配楼宇自动化协议编解码库 |
| [`cap:bed`](./bed.md) | `[Pure-CP]` | 纯 CP UCSC BED3–BED12 基因组区间零分配解析与重叠计算库 |
| [`cap:billing`](./billing.md) | `[Pure-CP]` | cap:billing 是 CP 语言官方内置的纯原生、零 C 依赖工业级计费与发票生成标准库，专为 AI API Token 计量、SaaS 订阅升降级折算与云资源账单结算打造。全模块使用最小整数货币单位（cents  |
| [`cap:byte_size`](./byte_size.md) | `[Bun]` | Human-Readable Byte-Size Parser & SI/IEC Formatter |
| [`cap:can`](./can.md) | `[Pure-CP]` | ISO 11898 CAN 2.0/FD、SocketCAN、SAE J1939 与 DBC 信号零分配库 |
| [`cap:cif`](./cif.md) | `[Pure-CP]` | 零分配 IUCr CIF 1.1 / mmCIF 晶体学与生物大分子结构解析库 |
| [`cap:dicom`](./dicom.md) | `[Pure-CP]` | 零分配医学影像 DICOM PS3.10 (`.dcm`) 解析库 |
| [`cap:dnp3`](./dnp3.md) | `[Pure-CP]` | IEEE 1815 DNP3 电力与 SCADA 零分配线协议编解码库 |
| [`cap:edf`](./edf.md) | `[Pure-CP]` | 零分配 EDF / EDF+ 生物医学信号解析标准库 |
| [`cap:edid`](./edid.md) | `[Pure-CP]` | VESA EDID 1.3 / 1.4 显示器识别数据块零分配解析库 |
| [`cap:eot`](./eot.md) | `[Pure-CP]` | 零分配 Microsoft Embedded OpenType (`.eot`) Web 字体容器解析库 |
| [`cap:fasta`](./fasta.md) | `[Pure-CP]` | 纯 CP 生物信息学 FASTA DNA/RNA/蛋白质序列零分配解析库 |
| [`cap:fastq`](./fastq.md) | `[Pure-CP]` | 高通量基因测序 FASTQ 零分配解析与质控统计库 |
| [`cap:fdt`](./fdt.md) | `[Pure-CP]` | Devicetree Specification v0.4 (`.dtb`) 零分配解析库 |
| [`cap:fit`](./fit.md) | `[Pure-CP]` | 纯 CP Garmin / ANT+ FIT 运动遥测二进制文件零分配解析库 |
| [`cap:fx`](./fx.md) | `[Pure-CP]` | cap:fx 是 CP 语言官方内置的纯原生定点多币种货币算术与外汇三角换算引擎，采用 i128 最小货币单位（Minor Units）与 256 位无堆中间算术彻底消除 IEEE 754 浮点误差，专为跨境清结算、多币 |
| [`cap:gcode`](./gcode.md) | `[Pure-CP]` | CNC / 3D 打印 G-Code 零分配解析与刀路统计库 |
| [`cap:gff`](./gff.md) | `[Pure-CP]` | 纯 CP GFF3 / GTF2 基因特征注释零分配解析库 |
| [`cap:hid`](./hid.md) | `[Pure-CP]` | USB HID 1.11 Report Descriptor 零分配流式解析库 |
| [`cap:iban`](./iban.md) | `[Pure-CP]` | ISO 13616 IBAN 与 ISO 7064 Mod-97-10 校验及格式化库 |
| [`cap:ihex`](./ihex.md) | `[Pure-CP]` | 纯 CP Intel HEX (I8HEX/I16HEX/I32HEX) 嵌入式固件零分配编解码库 |
| [`cap:inchi`](./inchi.md) | `[Pure-CP]` | IUPAC InChI v1.06 分层结构与 27 字符 InChIKey 零分配解析校验库 |
| [`cap:knx`](./knx.md) | `[Pure-CP]` | ISO/IEC 14543-3-2 KNXnet/IP、cEMI 帧与 DPT9 浮点零分配库 |
| [`cap:ledger`](./ledger.md) | `[Pure-CP]` | cap:ledger 是 CP 语言官方内置的纯 CP 复式记账（Double-Entry Bookkeeping）会计总账引擎，零 C 依赖、零 std.posix、零 @import("bun")，专为金融支付系统、 |
| [`cap:modbus`](./modbus.md) | `[Pure-CP]` | Modbus TCP MBAP & Modbus RTU CRC-16 零分配编解码库 |
| [`cap:mol2`](./mol2.md) | `[Pure-CP]` | Tripos SYBYL `.mol2` 3D 分子结构与力场原子类型零分配解析库 |
| [`cap:multiboot`](./multiboot.md) | `[Pure-CP]` | GNU Multiboot1 & Multiboot2 引导协议零分配解析库 |
| [`cap:pci`](./pci.md) | `[Pure-CP]` | PCI / PCIe 配置空间、BAR 与 Capability 链表零分配解析库 |
| [`cap:rmq`](./rmq.md) | `[Pure-CP]` | O(1) Sparse Table 区间最值/GCD、欧拉序 LCA 与滑动窗口零分配库 |
| [`cap:s7`](./s7.md) | `[Pure-CP]` | Siemens S7comm / TPKT (RFC 1006) / COTP (ISO 8073) 零分配编解码库 |
| [`cap:sam`](./sam.md) | `[Pure-CP]` | 零分配 SAM v1.6 生物信息学序列比对解析库 |
| [`cap:scsi`](./scsi.md) | `[Pure-CP]` | SCSI SPC-5 / SBC-4 CDB、INQUIRY、CAPACITY & Sense 零分配编解码库 |
| [`cap:smbios`](./smbios.md) | `[Pure-CP]` | DMTF SMBIOS 2.1 / 3.0+ Entry Point 与硬件结构表零分配解析库 |
| [`cap:srec`](./srec.md) | `[Pure-CP]` | 零分配 Motorola S-Record (S0–S9) 嵌入式固件编解码库 |
| [`cap:uf2`](./uf2.md) | `[Pure-CP]` | Microsoft UF2 (USB Flashing Format) 零分配固件编解码库 |
| [`cap:vcd`](./vcd.md) | `[Pure-CP]` | 零分配 IEEE 1364 VCD 硬件仿真波形解析库 |
| [`cap:vcf`](./vcf.md) | `[Pure-CP]` | 生物信息学 VCF 4.2/4.3 基因变异零分配解析库 |
