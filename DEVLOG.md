# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: CSS @page 分页规则与 Position 定位属性（5/5 PASS，文档直写 `doc/lib/css_page_rule.md`）
从 Bun css/rules/page.cp 与 properties/position.cp 吸收 16 种 PageMarginBox、PageSelector 与 Position 属性，零堆分配内联存储。

## 2026-11-21: W3C Subresource Integrity (SRI) 解析与流式校验器（5/5 PASS，文档直写 `doc/lib/sri.md`）
从 Bun install/integrity.cp 吸收 W3C SRI 解析、40 位 Hex SHA-1 与流式分块校验引擎，采用恒定时间摘要比对与零堆分配设计。

## 2026-11-21: CSS Display Level 3 与 Visibility 属性模型（5/5 PASS，文档直写 `doc/lib/css_display.md`）
从 Bun css/properties/display.cp 吸收 CSS Display Level 3 内外流模型与 Visibility 属性，纯 CP 零依赖实现解析、合法性校验与最短规范输出。

## 2026-11-21: PostgreSQL v3 认证握手与大端协议写入器（5/5 PASS，文档直写 `doc/lib/pg_auth.md`）
从 Bun sql/postgres/protocol/Authentication.cp 与 NewWriter.cp 吸收 PG v3 认证握手解码与大端协议写入器，支持 ~init() 确定性释放与严密截断校验。

## 2026-11-21: 小字符串内联优化 Compact String（5/5 PASS，文档直写 `doc/lib/smol_str.md`）
从 Bun string/SmolStr.cp 吸收小字符串内联优化引擎，<=15 字节零堆分配，超长自动倍增转堆，采用幂等 ~init() 确定性 RAII，零泄漏通过全部回归。

## 2026-11-21: HTTP 路由参数映射表与 URLPath 解析器（5/5 PASS，文档直写 `doc/lib/route_params.md`）
从 Bun router/ParamMap.cp 与 http_types/URLPath.cp 吸收路由参数表与 URL 路径解析器（~init() 零泄漏），并将 Bun threading/unbounded_queue.cp 无损合入 cap:lockfree_queue 新增缓存行防伪共享 UnboundedQueue。

## 2026-11-21: 栈上定容动态数组 BoundedArray（5/5 PASS，文档直写 `doc/lib/bounded_array.md`）
从 Bun collections/bounded_array.cp 吸收零堆分配 BoundedArray<T, capacity>，提供完整增删改查与安全溢出/越界错误返回。

## 2026-11-21: 标识符字符频率统计与压缩短名生成器（5/5 PASS，文档直写 `doc/lib/char_freq.md`）
从 Bun js_parser/ast/CharFreq.cp 吸收标识符字符频率统计器并配套零堆分配 NameMinifier，修复了原版小路径数字桶偏移并 100% 回归通过。

## 2026-11-21: 编译期零堆分配静态哈希表（5/5 PASS，文档直写 `doc/lib/static_hashmap.md`）
从 Bun collections/StaticHashMap.cp 吸收编译期静态哈希表，内置 FNV-1a/SplitMix64 哈希与后向移位删除，纯 CP 零依赖。

## 2026-11-21: SourceMap Base64-VLQ 高速查表编解码器（5/5 PASS，文档直写 `doc/lib/vlq.md`）
从 Bun sourcemap/VLQ.cp 吸收 Base64-VLQ 高速查表编解码器，使用 64 位无溢出算术完整覆盖 i32::MIN..i32::MAX 并严格校验非法字符与截断输入。

## 2026-11-21: 全量 Unicode Case Folding 引擎（5/5 PASS，文档直写 `doc/lib/case_fold.md`）
从 Bun md/unicode.cp 吸收压缩范围二分查找表与完整 Unicode Case Folding，纯 CP 零堆分配实现单码点折叠与流式 caseFoldEqual。

## 2026-11-21: Unicode 三级压缩查找表生成器与 O(1) 查询引擎（5/5 PASS，文档直写 `doc/lib/unicode_lut.md`）
从 Bun unicode/uucode/lut.cp 吸收三级压缩查找表引擎，使用 ~init() 确定性 RAII 自动释放三层表缓冲，支持自定义上限与全量 0x10FFFF 构建。

## 2026-11-21: MySQL 协议列类型与 ColumnDefinition41 编解码器（5/5 PASS，文档直写 `doc/lib/mysql_field_types.md`）
从 Bun sql/mysql/MySQLTypes.cp 与 ColumnDefinition41.cp 吸收完整 MySQL 列类型与 41 号列元数据报文编解码，采用 ~init() 确定性 RAII，零泄漏通过测试。

## 2026-11-21: GFM 宽松自动链接扫描器（5/5 PASS，文档直写 `doc/lib/autolink.md`）
从 Bun md/autolinks.cp 吸收 GFM 宽松自动链接扫描器，纯 CP 零依赖实现 http/https/ftp/www/email 边界识别与尾部实体/括号裁剪。

## 2026-11-21: HTTP/2 (RFC 9113) 线协议帧头与设置编解码器（5/5 PASS，文档直写 `doc/lib/h2_frame.md`）
从 Bun http/H2FrameParser.cp 吸收 HTTP/2 线协议帧头、设置单元与流优先级大端编解码器，纯 CP 零依赖实现短缓冲边界防护。

## 2026-11-21: 编译期类型特征自省与指针元数据（5/5 PASS，文档直写 `doc/lib/type_traits.md`）
从 Bun meta/traits.cp 与 ptr/meta.cp 吸收编译期类型自省谓词与 PointerMeta.inspect，零运行期开销且完全适配 CP 新语法。

## 2026-11-21: 栈内联缓冲优先 + 堆回退分配器（5/5 PASS，文档直写 `doc/lib/fallback_alloc.md`）
从 Bun bun_alloc/BufferFallbackAllocator.cp 吸收栈内联缓冲优先分配器，支持 ownsBufferPtr/usedBytes/remainingBytes/reset 与跨区域 resize/remap/free 自动路由。

## 2026-11-21: 无分支 UTF-8 DFA 解码器与 ASCII 谓词（5/5 PASS，文档直写 `doc/lib/utf8_dfa.md`）
从 Bun unicode/uucode_lib/src/{utf8,ascii}.cp 吸收无分支 UTF-8 DFA 流式解码器与 ASCII 码点谓词，纯 CP 零依赖实现超长编码/代理对安全替换为 U+FFFD。

## 2026-11-21: CSS 遮罩与裁剪属性模型（5/5 PASS，文档直写 `doc/lib/css_masking.md`）
从 Bun css/properties/masking.cp 吸收完整 CSS 遮罩/裁剪枚举、Porter-Duff 合成模式及 13 项 -webkit- 前缀属性映射，纯 CP 零依赖。

## 2026-11-21: Windows PE64 可执行文件解析与构建器（5/5 PASS，文档直写 `doc/lib/pe_format.md`）
从 Bun exe_format/pe.cp 吸收完整 PE64 可执行文件解析/节表注入/校验和引擎，采用 ~init() RAII 与对齐安全读取，零泄漏通过全量回归。

## 2026-11-21: 紧凑二进制 Schema 流式编解码器（5/5 PASS，文档直写 `doc/lib/binary_schema.md`）
从 Bun analytics/schema.cp 吸收紧凑二进制 Schema 流式编解码引擎，使用 ~init() 确定性 RAII 与 comptime for 枚举严格校验，零泄漏通过全部边界测试。

## 2026-11-21: JPEG/TIFF EXIF 方向元数据读取器（5/5 PASS，文档直写 `doc/lib/exif.md`）
从 Bun runtime/image/exif.cp 吸收 JPEG APP1/Exif 与大小端 TIFF IFD0 0x0112 标签读取器，纯 CP 零依赖实现宽容解析与旋转/翻转变换计算。

## 2026-11-21: QUIC 报文头、VarInt 与 QPACK/SocketKind 协议集（5/5 PASS，文档直写 `doc/lib/quic_header.md`）
从 Bun uws_sys/quic/Header.cp 与 SocketKind.cp 吸收 QUIC v1/v2 长短头、RFC 9000 VarInt、QPACK 静态表分类器与 23 种 SocketKind 枚举。

## 2026-11-21: CSS 文本、列表与滤镜混合属性（5/5 PASS，文档直写 `doc/lib/css_text_list.md`）
从 Bun css/properties/{text,list,effects}.cp 吸收文本排版、14 种列表标记、11 种滤镜与 16 种混合模式枚举及位标志，100% 纯 CP 零依赖。

## 2026-11-21: PostgreSQL v3 线协议字段描述与启动报文（5/5 PASS，文档直写 `doc/lib/pg_wire_types.md`）
从 Bun `sql/postgres/protocol/{FieldDescription,StartupMessage}.cp` 吸收 PG v3 启动报文与列元数据编解码器，采用 CP `~init()` RAII 析构与显式借用，零泄漏通过全量边界测试。

## 2026-11-21: CSS 几何与溢出布局值类型（5/5 PASS，文档直写 `doc/lib/css_geometry.md`）
从 Bun `css/values/{ratio,rect}.cp` 与 `css/properties/overflow.cp` 吸收 CSS 几何与溢出值类型，完整覆盖四向简写展开、宽高比归约与 CSS 格式化输出。

## 2026-11-21: JS/TS AST 运算符与优先级表（5/5 PASS，文档直写 `doc/lib/js_op.md`）
从 Bun `js_parser/ast/Op.cp` 吸收完整 JS/TS 运算符集合（50+ 运算符）、Pratt 解析优先级表及属性判定方法，100% 纯 CP 零依赖。

## 2026-11-21: 结构体零填充校验器（5/5 PASS，文档直写 `doc/lib/padding_checker.md`）
从 Bun `install/padding_checker.cp` 吸收并升级为 CP 新语法（`comptime for (field of s.fields)`），提供编译期结构体 padding 检测与断言，保障二进制协议与持久化布局零未初始化间隙。

## 2026-11-21: 新建 `cap:mysql_proto` 纯 CP MySQL Wire Protocol 能力位、服务端状态标志与 226 项 Collation/字符集查找表（5/5 PASS，文档直写 `doc/lib/mysql_proto.md`）
子智能体吸收自 bun/src/sql/mysql/{Capabilities,StatusFlags,protocol/CharacterSet}.cp。

## 2026-11-21: 新建 `cap:http_status` 纯 CP 零分配 HTTP 状态码 100-599 全量短语表与 RFC 9110 语义分类器（5/5 PASS，文档直写 `doc/lib/http_status.md`）
子智能体吸收自 bun/src/runtime/server/HTTPStatusText.cp，补齐 RFC 9110 缓存/重试/Body 判定。

## 2026-11-21: 新建 `cap:intrusive_heap` 纯 CP 零堆分配侵入式配对最小堆（均摊 O(1) 插入/合并，O(log N) 弹出/任意删除）（5/5 PASS，文档直写 `doc/lib/intrusive_heap.md`）
吸收自 bun/src/io/heap.cp，采用新 CP 裸指针 *T! 与严格无别名借用风格重构。

## 2026-11-21: 新建 `cap:exact_matcher` 纯 CP 定长整型打包（1/2/4/8/12/16 字节）O(1) 无分支字符串快速匹配器（5/5 PASS，文档直写 `doc/lib/exact_matcher.md`）
吸收自 bun/src/string/immutable/exact_size_matcher.cp，修正 str.len == max_bytes 时 matchLower 漏转小写问题。

## 2026-11-21: 新建 `cap:myers_diff` 纯 CP Myers O(ND) 最短编辑脚本行级/字节级 Diff 引擎（含尾随逗号容错与 ~init() 自动释放）（5/5 PASS，文档直写 `doc/lib/myers_diff.md`）
吸收自 bun/src/runtime/node/assert/myers_diff.cp，改写为 Zig 0.17 原生与 CP ~init() RAII 风格。

## 2026-11-21: 新建 `cap:feature_flags` 纯 CP 特性开关与环境变量强类型容错解析引擎（吸收自 Bun `src/bun_core/{feature_flags,env_var}.cp`，支持 `cap.feature_flags` 自动动态挂载，详见 `doc/lib/feature_flags.md`）（5/5 PASS，文档直写 `doc/lib/feature_flags.md`）
- **吸收改造**：剥离进程全局环境依赖，提供纯函数式大小写不敏感布尔/整数解析与默认值回退。

## 2026-11-21: 新建 `cap:coverage_options` 纯 CP 代码覆盖率配置、阈值判定与 LCOV 格式化引擎（吸收自 Bun `src/options_types/CodeCoverageOptions.cp`，支持 `cap.coverage_options` 自动动态挂载，详见 `doc/lib/coverage_options.md`）（5/5 PASS，文档直写 `doc/lib/coverage_options.md`）
- **吸收改造**：补充 `CoverageMetrics` 比例计算与 `formatLcovRecord` 标准 LCOV 输出，通过零泄漏测试。

## 2026-11-21: 新建 `cap:small_string` 纯 CP 8 字节短串内联 (SSO) 与 Wyhash 去重驻留池引擎（吸收自 Bun `src/install_types/{SemverString,ExternalString,SlicedString}.cp`，支持 `cap.small_string` 自动动态挂载，详见 `doc/lib/small_string.md`）（5/5 PASS，文档直写 `doc/lib/small_string.md`）
- **吸收改造**：去除 `Lockfile`/`bun` 耦合，保留 8 字节 `extern struct` 小串零堆内联布局与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:mime_type` 纯 CP MIME 类型语义分类与零堆扩展名/路径反查引擎（吸收自 Bun `src/http_types/MimeType.cp`，支持 `cap.mime_type` 自动动态挂载，详见 `doc/lib/mime_type.md`）（5/5 PASS，文档直写 `doc/lib/mime_type.md`）
- **吸收改造**：剥离 `bun.strings` 耦合，支持 `; charset=` 剥离、前缀 `.` 归一化与大小写不敏感扩展名查表。

## 2026-11-21: 新建 `cap:command_tag` 纯 CP CLI 32 种子命令判别、遥测码、NPM 判定与配置加载策略引擎（吸收自 Bun `src/options_types/CommandTag.cp`，支持 `cap.command_tag` 自动动态挂载，详见 `doc/lib/command_tag.md`）（5/5 PASS，文档直写 `doc/lib/command_tag.md`）
- **吸收改造**：去除 `cli.zig` 依赖，补充 `StaticStringMap` 反查表，实现零堆 CLI 子命令策略判定。

## 2026-11-21: 新建 `cap:bundle_options` 纯 CP 打包输出格式、全局缓存策略、离线模式与包链接模式枚举引擎（吸收自 Bun `src/options_types/` 与 `src/install_types/`，支持 `cap.bundle_options` 自动动态挂载，详见 `doc/lib/bundle_options.md`）（5/5 PASS，文档直写 `doc/lib/bundle_options.md`）
- **吸收改造**：采用 `std.StaticStringMap` 实现零堆字符串映射与完整策略矩阵校验。

## 2026-11-21: 新建 `cap:node_builtins` 纯 CP Node.js & Bun 内建模块映射与规范化注册表（吸收自 Bun `src/resolve_builtins/HardcodedModule.cp`，支持 `cap.node_builtins` 自动动态挂载，详见 `doc/lib/node_builtins.md`）（5/5 PASS，文档直写 `doc/lib/node_builtins.md`）
- **吸收改造**：采用编译期 `std.StaticStringMap` 重构 `bun.ComptimeStringMap`，零堆分配实现目标隔离与前缀强制校验。

## 2026-11-21: 新建 `cap:http_types` 纯 CP HTTP 核心类型、ETag 弱匹配、Fetch 枚举与零堆 URLPath 解析引擎（吸收自 Bun `src/http_types/*.cp`，支持 `cap.http_types` 自动动态挂载，详见 `doc/lib/http_types.md`）（5/5 PASS，文档直写 `doc/lib/http_types.md`）
- **吸收改造**：去除 `bun` 宿主耦合，采用 `std.StaticStringMap` 与 `std.hash.XxHash64` 重写，通过全 5 组深度测试。

## 2026-11-21: 新建 `cap:union_interval` 纯 CP 不相交区间合并集合引擎（自动合并重叠/相邻区间、删除自动分裂与 $O(\log N)$ 二分点查，支持 `cap.union_interval` 自动动态挂载，详见 `doc/lib/union_interval.md`）（5/5 PASS，文档直写 `doc/lib/union_interval.md`）
- **核心设计**：线性归并合并重叠区间与二分点查，通过中段分裂/边界裁剪与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:varint_leb128` 纯 CP 零堆 ULEB128 / SLEB128 / ZigZag 32/64 位变长整数编解码引擎（支持 10 字节溢出与截断防护，支持 `cap.varint_leb128` 自动动态挂载，详见 `doc/lib/varint_leb128.md`）（5/5 PASS，文档直写 `doc/lib/varint_leb128.md`）
- **核心设计**：零堆流式位编码与符号扩展解码，通过 DWARF ULEB128/SLEB128 官方向量与溢出/EOF 拦截测试。

## 2026-11-21: 新建 `cap:decimal_fp` 纯 CP 金融定点数 Decimal 精确算术引擎（`i128` 尾数 + `0..18` 标度，零浮点漂移、4 种舍入模式与字符串往返，支持 `cap.decimal_fp` 自动动态挂载，详见 `doc/lib/decimal_fp.md`）（5/5 PASS，文档直写 `doc/lib/decimal_fp.md`）
- **核心设计**：采用 `i128` 尾数标度对齐算术与银行家舍入 `HalfEven`，通过 `"0.1"+"0.2"=="0.3"` 及溢出/除零拦截验证。

## 2026-11-21: 新建 `cap:base64url` 纯 CP RFC 4648 Base64 与 JWT/JWK 专用无填充/自适应 Base64URL 编解码引擎（支持 `cap.base64url` 自动动态挂载，详见 `doc/lib/base64url.md`）（5/5 PASS，文档直写 `doc/lib/base64url.md`）
- **核心设计**：封装 Zig 0.17 `std.base64` 标准与 URL 安全无填充编解码器，通过 RFC 4648 Section 10 向量与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:bitset_simd` 纯 CP 按字向量化加速动态位集合引擎（基于 `u64` 字块 `@popCount`/`@ctz` 批量位运算、`setRange`/`nextSetBit` 及集合并交差，支持 `cap.bitset_simd` 自动动态挂载，详见 `doc/lib/bitset_simd.md`）（5/5 PASS，文档直写 `doc/lib/bitset_simd.md`）
- **核心设计**：按 64 位字块执行向量化位运算与 `@ctz` 跳位迭代，通过跨字边界与 `std.testing.allocator` 零泄漏验证。


## 2026-11-21: CP 所有权/借用/五态解构/唯一双重幂等析构 `~init()` 体系重构与饱和赋值热修
- **核心原理与问题根因**：
  1. 下游 Bun 迁移暴露原 `cpc` 的 3 类 RAII 缺陷：① `type_has_release` 错误解包裸指针 `*T!` 与借用导致提前销毁；② `stmt.rs` 存在“文件内任意类有 `release` 则对所有未知类型局部变量注入 `defer release()`”的兜底逻辑，且字段读取 `const h = _.handlers` 未标记借用导致浅拷贝被误析构；③ 手动调用 `release()` 未同步置位 `__x_moved` 引发 Double-Free/Double-Unlock。
  2. 新体系以 `~init()` 作为唯一析构语法（无参无返回值），结合 Layer 1 作用域 `__x_moved` 与 Layer 2 非 C-ABI 实例 `__cp_dropped` 实现双重幂等（保留 `SharedTensor` 等引用计数类型特殊放行）；同时将直接赋值与解构赋值统一至“默认只读、`*` 可变借用、`move` 显式转移”，并在 `Pattern::Object` 落地整体 vs 局部成员精细分离借用（同步与状态机 `ctx` 类型推导同步升级）。
  3. 同步解决 `bun_agy` 热修工单 `task-bun-20261121-sat-assign-op`（补齐 Zig 风格饱和/环绕复合赋值 `+|=`, `-|=`, `*|=` , `<<|=`, `+%=`, `-%=`, `*%=`）。
- **验证方案**：构建 Release 版 `cpc`，运行 8 组正向深测（`test_raii_deinit_ownership_deep.cp`）、9 组编译期负向安全拦截与 6 组既有核心回归套件，全部 100% 通过。

## 2026-11-21: 彻底取消 `release` 析构语义（`~init()` 唯一化）与 `CP_PITFALLS.md` 全量对齐
- **设计决策**：用户确认 `release` 无论无参或多参均应作为普通业务方法，不再承担析构职责，以彻底消除与 Bun/业务代码的方法名冲突。编译器后端收口至唯一 `__cp_deinit` 钩子（用户侧对应 `~init()`，提前释放对应 `drop(x)` / `x.~init()`），并同步修复 `call.rs` 历史上对多参 `.release(a, b)` 的截断问题。
- **验证与广播**：发布 Release 版 `cpc`，`test_raii_deinit_ownership_deep.cp`（8/8 PASS）与核心所有权/生命周期/Trait 回归全通过；同步更新 `cp_agent_ref.md` 与两份下游 `CP_PITFALLS.md` 并向 `bun_agy`/`capsurf_agy` 发出广播 `syntax-raii-20261121-v2`。

## 2026-11-21: `cpc fix` & 诊断全面对齐新语法 + 全仓库一键迁移落地
- **关键修复**：
  1. 发现 `parser/decl.rs:1195` 对 `E0121` 注入了 `explicit_edits` 且未检查前导 `*`，导致 `cpc fix` 将 `*this` 误修为 `**_`；同时 `diagnostic.rs:471` 对 `E0201` 仍按旧语法插入前置 `*param`，一并彻底修复并新增 `param: move/own T` 自动前移修复。
  2. `scripts/migrate_cp_syntax.py` 引入 `zig { ... }` 块深度追踪与 `_ =(?!>)` 负向前瞻，避免误伤 Zig 逃逸块与 `_ =>` 匹配分支；对 `lib/mem/allocator.cp` 区分了引用计数 `SharedTensor.release()` 与 RAII 分配器 `BumpArena`/`MaybeOwnedBuf`/`MaxHeapAllocator` 的 `~init()`。
- **验证**：Release `cpc` 构建通过，`cpc fix` 6 项自动化断言全通过，`test_bun_absorbed_assets`、`test_string_unicode`、`test_collections_bun`、`test_blob`、`test_col_complete`、`test_mem_bun`、`test_string_simd_sort`、`test_shell_napi_elf_logger_deep`、`test_http_router_sql_deep`、`test_net`、`test_raii_deinit_ownership_deep` 全部 100% PASS。
