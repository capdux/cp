# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 栈上定容动态数组 BoundedArray（5/5 PASS，自动动态挂载 `cap.bounded_array`）
新增 cap:bounded_array 标准库（lib/bounded_array/、doc/lib/bounded_array.md、cap/test/test_bounded_array_production_deep.cp），2/2 回归通过。

## [2026-11-21] - 标识符字符频率统计与压缩短名生成器（5/5 PASS，自动动态挂载 `cap.char_freq`）
新增 cap:char_freq 标准库（lib/char_freq/、doc/lib/char_freq.md、cap/test/test_char_freq_production_deep.cp），3/3 回归通过。

## [2026-11-21] - 编译期零堆分配静态哈希表（5/5 PASS，自动动态挂载 `cap.static_hashmap`）
新增 cap:static_hashmap 标准库（lib/static_hashmap/、doc/lib/static_hashmap.md、cap/test/test_static_hashmap_production_deep.cp），6/6 回归通过。

## [2026-11-21] - SourceMap Base64-VLQ 高速查表编解码器（5/5 PASS，自动动态挂载 `cap.vlq`）
新增 cap:vlq 标准库（lib/vlq/、doc/lib/vlq.md、cap/test/test_vlq_production_deep.cp），6/6 回归通过。

## [2026-11-21] - 全量 Unicode Case Folding 引擎（5/5 PASS，自动动态挂载 `cap.case_fold`）
新增 cap:case_fold 标准库（lib/case_fold/、doc/lib/case_fold.md、cap/test/test_case_fold_production_deep.cp），3/3 回归通过。

## [2026-11-21] - Unicode 三级压缩查找表生成器与 O(1) 查询引擎（5/5 PASS，自动动态挂载 `cap.unicode_lut`）
新增 cap:unicode_lut 标准库（lib/unicode_lut/、doc/lib/unicode_lut.md、cap/test/test_unicode_lut_production_deep.cp），6/6 回归通过（全量 1,114,112 码点 >100x 压缩，零泄漏）。

## [2026-11-21] - MySQL 协议列类型与 ColumnDefinition41 编解码器（5/5 PASS，自动动态挂载 `cap.mysql_field_types`）
新增 cap:mysql_field_types 标准库（lib/mysql_field_types/、doc/lib/mysql_field_types.md、cap/test/test_mysql_field_types_production_deep.cp），5/5 回归通过（零泄漏）。

## [2026-11-21] - GFM 宽松自动链接扫描器（5/5 PASS，自动动态挂载 `cap.autolink`）
新增 cap:autolink 标准库（lib/autolink/、doc/lib/autolink.md、cap/test/test_autolink_production_deep.cp），4/4 回归通过。

## [2026-11-21] - HTTP/2 (RFC 9113) 线协议帧头与设置编解码器（5/5 PASS，自动动态挂载 `cap.h2_frame`）
新增 cap:h2_frame 标准库（lib/h2_frame/、doc/lib/h2_frame.md、cap/test/test_h2_frame_production_deep.cp），4/4 回归通过。

## [2026-11-21] - 编译期类型特征自省与指针元数据（5/5 PASS，自动动态挂载 `cap.type_traits`）
新增 cap:type_traits 标准库（lib/type_traits/、doc/lib/type_traits.md、cap/test/test_type_traits_production_deep.cp），3/3 回归通过。

## [2026-11-21] - 栈内联缓冲优先 + 堆回退分配器（5/5 PASS，自动动态挂载 `cap.fallback_alloc`）
新增 cap:fallback_alloc 标准库（lib/fallback_alloc/、doc/lib/fallback_alloc.md、cap/test/test_fallback_alloc_production_deep.cp），4/4 回归通过（零泄漏）。

## [2026-11-21] - 无分支 UTF-8 DFA 解码器与 ASCII 谓词（5/5 PASS，自动动态挂载 `cap.utf8_dfa`）
新增 cap:utf8_dfa 标准库（lib/utf8_dfa/、doc/lib/utf8_dfa.md、cap/test/test_utf8_dfa_production_deep.cp），4/4 回归通过。

## [2026-11-21] - CSS 遮罩与裁剪属性模型（5/5 PASS，自动动态挂载 `cap.css_masking`）
新增 cap:css_masking 标准库（lib/css_masking/、doc/lib/css_masking.md、cap/test/test_css_masking_production_deep.cp），5/5 回归通过。

## [2026-11-21] - Windows PE64 可执行文件解析与构建器（5/5 PASS，自动动态挂载 `cap.pe_format`）
新增 cap:pe_format 标准库（lib/pe_format/、doc/lib/pe_format.md、cap/test/test_pe_format_production_deep.cp），6/6 回归通过（零泄漏）。

## [2026-11-21] - 紧凑二进制 Schema 流式编解码器（5/5 PASS，自动动态挂载 `cap.binary_schema`）
新增 cap:binary_schema 标准库（lib/binary_schema/、doc/lib/binary_schema.md、cap/test/test_binary_schema_production_deep.cp），5/5 回归通过（零泄漏）。

## [2026-11-21] - JPEG/TIFF EXIF 方向元数据读取器（5/5 PASS，自动动态挂载 `cap.exif`）
新增 cap:exif 标准库（lib/exif/、doc/lib/exif.md、cap/test/test_exif_production_deep.cp），4/4 回归通过。

## [2026-11-21] - QUIC 报文头、VarInt 与 QPACK/SocketKind 协议集（5/5 PASS，自动动态挂载 `cap.quic_header`）
新增 cap:quic_header 标准库（lib/quic_header/、doc/lib/quic_header.md、cap/test/test_quic_header_production_deep.cp），5/5 回归通过。

## [2026-11-21] - CSS 文本、列表与滤镜混合属性（5/5 PASS，自动动态挂载 `cap.css_text_list`）
新增 cap:css_text_list 标准库（lib/css_text_list/、doc/lib/css_text_list.md、cap/test/test_css_text_list_production_deep.cp），6/6 回归通过。

## [2026-11-21] - PostgreSQL v3 线协议字段描述与启动报文（5/5 PASS，自动动态挂载 `cap.pg_wire_types`）
新增 `cap:pg_wire_types` 标准库（`lib/pg_wire_types/`、、），5/5 回归通过（零泄漏）。

## [2026-11-21] - CSS 几何与溢出布局值类型（5/5 PASS，自动动态挂载 `cap.css_geometry`）
新增 `cap:css_geometry` 标准库（`lib/css_geometry/`、、），6/6 回归通过。

## [2026-11-21] - JS/TS AST 运算符与优先级表（5/5 PASS，自动动态挂载 `cap.js_op`）
新增 `cap:js_op` 标准库（`lib/js_op/`、、），5/5 回归通过。

## [2026-11-21] - 结构体零填充校验器（5/5 PASS，自动动态挂载 `cap.padding_checker`）
新增 `cap:padding_checker` 标准库（`lib/padding_checker/`、、），5/5 回归通过。

## [2026-11-21] - 新建 `cap:mysql_proto` 纯 CP MySQL Wire Protocol 能力位、服务端状态标志与 226 项 Collation/字符集查找表（5/5 PASS，自动动态挂载 `cap.mysql_proto`）
新增 lib/mysql_proto/{protocol,index}.cp、doc/lib/mysql_proto.md 与 6 组深度测试（100% PASS）。

## [2026-11-21] - 新建 `cap:http_status` 纯 CP 零分配 HTTP 状态码 100-599 全量短语表与 RFC 9110 语义分类器（5/5 PASS，自动动态挂载 `cap.http_status`）
新增 lib/http_status/{status,index}.cp、doc/lib/http_status.md 与 5 组深度测试（100% PASS）。

## [2026-11-21] - 新建 `cap:intrusive_heap` 纯 CP 零堆分配侵入式配对最小堆（均摊 O(1) 插入/合并，O(log N) 弹出/任意删除）（5/5 PASS，自动动态挂载 `cap.intrusive_heap`）
新增 lib/intrusive_heap/{heap,index}.cp、doc/lib/intrusive_heap.md 与 5 组深度测试（100% PASS）。

## [2026-11-21] - 新建 `cap:exact_matcher` 纯 CP 定长整型打包（1/2/4/8/12/16 字节）O(1) 无分支字符串快速匹配器（5/5 PASS，自动动态挂载 `cap.exact_matcher`）
新增 lib/exact_matcher/{matcher,index}.cp、doc/lib/exact_matcher.md 与 5 组深度测试（100% PASS，修复等长 matchLower 缺陷）。

## [2026-11-21] - 新建 `cap:myers_diff` 纯 CP Myers O(ND) 最短编辑脚本行级/字节级 Diff 引擎（含尾随逗号容错与 ~init() 自动释放）（5/5 PASS，自动动态挂载 `cap.myers_diff`）
新增 lib/myers_diff/{myers,index}.cp、doc/lib/myers_diff.md 与 5 组深度测试（100% PASS，std.testing.allocator 零泄漏）。

## [2026-11-21] - 新建 `cap:feature_flags` 纯 CP 特性开关与环境变量强类型容错解析引擎（吸收自 Bun `src/bun_core/{feature_flags,env_var}.cp`，支持 `cap.feature_flags` 自动动态挂载，详见 `doc/lib/feature_flags.md`）（5/5 PASS，自动动态挂载 `cap.feature_flags`）
- **新增标准库**：`lib/feature_flags/{feature_flags,index}.cp`，从 Bun `bun_core` 吸收 `FeatureFlags` 默认开关集与布尔/整数环境变量容错解析。\n- **新增文档与测试**：`doc/lib/feature_flags.md` 扁平文档与 `cap/test/test_feature_flags_production_deep.cp`（4/4 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:coverage_options` 纯 CP 代码覆盖率配置、阈值判定与 LCOV 格式化引擎（吸收自 Bun `src/options_types/CodeCoverageOptions.cp`，支持 `cap.coverage_options` 自动动态挂载，详见 `doc/lib/coverage_options.md`）（5/5 PASS，自动动态挂载 `cap.coverage_options`）
- **新增标准库**：`lib/coverage_options/{coverage_options,index}.cp`，从 Bun `CodeCoverageOptions.cp` 吸收覆盖率阈值判定与 LCOV 格式化。\n- **新增文档与测试**：`doc/lib/coverage_options.md` 扁平文档与 `cap/test/test_coverage_options_production_deep.cp`（4/4 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:small_string` 纯 CP 8 字节短串内联 (SSO) 与 Wyhash 去重驻留池引擎（吸收自 Bun `src/install_types/{SemverString,ExternalString,SlicedString}.cp`，支持 `cap.small_string` 自动动态挂载，详见 `doc/lib/small_string.md`）（5/5 PASS，自动动态挂载 `cap.small_string`）
- **新增标准库**：`lib/small_string/{small_string,index}.cp`，从 Bun `install_types` 吸收 8 字节 SSO `SmallString`、`ExternalString` 与去重驻留池 `StringInternPool`。\n- **新增文档与测试**：`doc/lib/small_string.md` 扁平文档与 `cap/test/test_small_string_production_deep.cp`（4/4 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:mime_type` 纯 CP MIME 类型语义分类与零堆扩展名/路径反查引擎（吸收自 Bun `src/http_types/MimeType.cp`，支持 `cap.mime_type` 自动动态挂载，详见 `doc/lib/mime_type.md`）（5/5 PASS，自动动态挂载 `cap.mime_type`）
- **新增标准库**：`lib/mime_type/{mime_type,index}.cp`，从 Bun `MimeType.cp` 吸收 `Category` 分类与 40+ 扩展名零堆查表。\n- **新增文档与测试**：`doc/lib/mime_type.md` 扁平文档与 `cap/test/test_mime_type_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:command_tag` 纯 CP CLI 32 种子命令判别、遥测码、NPM 判定与配置加载策略引擎（吸收自 Bun `src/options_types/CommandTag.cp`，支持 `cap.command_tag` 自动动态挂载，详见 `doc/lib/command_tag.md`）（5/5 PASS，自动动态挂载 `cap.command_tag`）
- **新增标准库**：`lib/command_tag/{command_tag,index}.cp`，从 Bun `CommandTag.cp` 吸收 32 个子命令判别枚举与策略谓词。\n- **新增文档与测试**：`doc/lib/command_tag.md` 扁平文档与 `cap/test/test_command_tag_production_deep.cp`（4/4 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bundle_options` 纯 CP 打包输出格式、全局缓存策略、离线模式与包链接模式枚举引擎（吸收自 Bun `src/options_types/` 与 `src/install_types/`，支持 `cap.bundle_options` 自动动态挂载，详见 `doc/lib/bundle_options.md`）（5/5 PASS，自动动态挂载 `cap.bundle_options`）
- **新增标准库**：`lib/bundle_options/{bundle_options,index}.cp`，从 Bun `options_types`/`install_types` 吸收 `Format`、`GlobalCache`、`OfflineMode`、`NodeLinker` 与 `WindowsOptions`。\n- **新增文档与测试**：`doc/lib/bundle_options.md` 扁平文档与 `cap/test/test_bundle_options_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:node_builtins` 纯 CP Node.js & Bun 内建模块映射与规范化注册表（吸收自 Bun `src/resolve_builtins/HardcodedModule.cp`，支持 `cap.node_builtins` 自动动态挂载，详见 `doc/lib/node_builtins.md`）（5/5 PASS，自动动态挂载 `cap.node_builtins`）
- **新增标准库**：`lib/node_builtins/{builtins,index}.cp`，从 Bun `src/resolve_builtins/HardcodedModule.cp` 吸收完整 `node:*`、裸模块、历史别名与 Jest/Vitest 重写规则。\n- **新增文档与测试**：`doc/lib/node_builtins.md` 扁平文档与 `cap/test/test_node_builtins_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:http_types` 纯 CP HTTP 核心类型、ETag 弱匹配、Fetch 枚举与零堆 URLPath 解析引擎（吸收自 Bun `src/http_types/*.cp`，支持 `cap.http_types` 自动动态挂载，详见 `doc/lib/http_types.md`）（5/5 PASS，自动动态挂载 `cap.http_types`）
- **新增标准库**：`lib/http_types/{http_types,index}.cp`，从 Bun `src/http_types/` 吸收 `Method`（36 种方法）、`Encoding`、Fetch 枚举、`ETag` 与 `URLPath`。\n- **新增文档与测试**：`doc/lib/http_types.md` 扁平文档与 `cap/test/test_http_types_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:union_interval` 纯 CP 不相交区间合并集合引擎（自动合并重叠/相邻区间、删除自动分裂与 $O(\log N)$ 二分点查，支持 `cap.union_interval` 自动动态挂载，详见 `doc/lib/union_interval.md`）（5/5 PASS，自动动态挂载 `cap.union_interval`）
- **新增标准库**：`lib/union_interval/{interval_set,index}.cp`，实现泛型半开区间合并集合 `IntervalSet(T)`。\n- **新增文档与测试**：`doc/lib/union_interval.md` 扁平文档与 `cap/test/test_union_interval_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:varint_leb128` 纯 CP 零堆 ULEB128 / SLEB128 / ZigZag 32/64 位变长整数编解码引擎（支持 10 字节溢出与截断防护，支持 `cap.varint_leb128` 自动动态挂载，详见 `doc/lib/varint_leb128.md`）（5/5 PASS，自动动态挂载 `cap.varint_leb128`）
- **新增标准库**：`lib/varint_leb128/{varint,index}.cp`，实现 ULEB128、SLEB128 与 ZigZag 32/64 编解码。\n- **新增文档与测试**：`doc/lib/varint_leb128.md` 扁平文档与 `cap/test/test_varint_leb128_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:decimal_fp` 纯 CP 金融定点数 Decimal 精确算术引擎（`i128` 尾数 + `0..18` 标度，零浮点漂移、4 种舍入模式与字符串往返，支持 `cap.decimal_fp` 自动动态挂载，详见 `doc/lib/decimal_fp.md`）（5/5 PASS，自动动态挂载 `cap.decimal_fp`）
- **新增标准库**：`lib/decimal_fp/{decimal,index}.cp`，实现精确金融定点数 `Decimal` 四则运算与舍入。\n- **新增文档与测试**：`doc/lib/decimal_fp.md` 扁平文档与 `cap/test/test_decimal_fp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:base64url` 纯 CP RFC 4648 Base64 与 JWT/JWK 专用无填充/自适应 Base64URL 编解码引擎（支持 `cap.base64url` 自动动态挂载，详见 `doc/lib/base64url.md`）（5/5 PASS，自动动态挂载 `cap.base64url`）
- **新增标准库**：`lib/base64url/{base64url,index}.cp`，实现 Standard Base64 与 Base64URL 编解码。\n- **新增文档与测试**：`doc/lib/base64url.md` 扁平文档与 `cap/test/test_base64url_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bitset_simd` 纯 CP 按字向量化加速动态位集合引擎（基于 `u64` 字块 `@popCount`/`@ctz` 批量位运算、`setRange`/`nextSetBit` 及集合并交差，支持 `cap.bitset_simd` 自动动态挂载，详见 `doc/lib/bitset_simd.md`）（5/5 PASS，自动动态挂载 `cap.bitset_simd`）
- **新增标准库**：`lib/bitset_simd/{bitset,index}.cp`，实现字块加速 `BitSet` 位集与稀疏迭代器。\n- **新增文档与测试**：`doc/lib/bitset_simd.md` 扁平文档与 `cap/test/test_bitset_simd_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。


## [2026-11-21] - CP 所有权/借用/五态解构/唯一双重幂等析构 `~init()` 全面重构 + 饱和/环绕复合赋值热修
- **修改 `cpc/src/ast.rs`、`cpc/src/lexer.rs`、`cpc/src/parser/{container,expr,pattern}.rs`**：
  - 新增 `~init() { ... }` 唯一析构函数语法与 `.~init()` 调用解析，非法参数/返回值/重名输出三段式 AI-First 诊断；
  - 新增 `DestructureMode`（`ReadOnly` / `MutBorrow` / `Move`）与对象解构字段级 `*` / `move` / `own` 前缀解析；
  - 修复 `ref struct` 被 `struct` 分支覆盖 `is_ref = false` 的缺陷；
  - 新增饱和/环绕复合赋值运算符（`+|=`, `-|=`, `*|=` , `<<|=`, `+%=`, `-%=`, `*%=`）及 `*|` / `<<|` 二元运算（结单 `task-bun-20261121-sat-assign-op`）；修复 `?? break :label [val]` 与 `?? continue :label`（含 `,`/`]` 终结符，结单 `task-bun-20261121-coalesce-break-label`）。
- **修改 `cpc/src/sema/{mod,coerce}.rs`**：
  - 内建注册 `drop(x)`；支持 `&v` 取址直接初始化 `*T!` / `*const T!` 裸指针。
- **修改 `cpc/src/codegen/{types/mod,decl_class,actor,stmt,expr/call,expr/ops,runtime_emit}.rs`**：
  - `type_has_release` 严格禁止解包裸指针与安全借用，仅识别 `~init()` (`__cp_deinit`) 与零额外参数 `release`；删除 `stmt.rs` 中误伤全文件的兜底 `defer` 注入；
  - 实现双重幂等析构（调用侧 `__x_moved` 守卫 + 非 C-ABI 实例 `__cp_dropped` 守卫，覆盖用户 `~init()` 与内置 `Map`/`Set`/`VecDeque`/`BinaryHeap`/`BTreeMap`/`BTreeSet`/`RegExp`，并跨 Zig 0.15/0.17 兼容 `pointer.is_const`/`attrs.@"const"` 与 `fn.params`/`param_types`）；
  - 统一直接赋值与五态对象解构代码生成及成员级分离借用检查（同步函数与状态机 `ctx` 双路径完整支持）；
  - 扩展 `Expr::Move` 支持成员字段转移（`move _.field`）并安全守卫 `__{id}_moved` 生成。
- **新增/更新测试与规范**：新增 `cap/test/test_raii_deinit_ownership_deep.cp`（8/8 PASS），更新 `cap/test/test_ownership_fail.cp` 与 `cp_agent_ref.md`（带 `[SYNC_PENDING: 2026-11-21]`）。

## [2026-11-21] - 彻底移除 `release` 析构语义（`~init()` 唯一析构）+ 全面对齐更新 `cp_agent_ref.md` 与下游 `CP_PITFALLS.md`
- **修改 `cpc/src/codegen/{types/mod,decl_class,actor,expr/call,runtime_emit}.rs`**：
  - `method_is_zero_arg_destructor`、`build_drop_call_snippet`、`__box_drop` 仅识别 `__cp_deinit`（`~init()`），彻底废除用户 `release` 方法作为析构函数；
  - `expr/call.rs` 移除对普通对象 `.release(...)` 的无参截断拦截，使无参与多参 `release` 100% 回归普通业务方法；
  - 内置运行时容器/守卫（`Map`/`Set`/`VecDeque`/`BinaryHeap`/`BTreeMap`/`BTreeSet`/`RegExp`/`LockGuard`/`ReadGuard`/`WriteGuard`/`ZigSharedTensor`）统一提供 `__cp_deinit`。
- **更新测试与文档**：迁移 `cap/test/test_{ownership_pass,ownership_fail,lifetime_pass,lifetime_fail_nll,lifetime_fail_dangling,ref_struct_fail_escape,trait_class_pass,own_keyword,error_box_switch_pass}.cp` 至 `~init()`；扩展 `test_raii_deinit_ownership_deep.cp` 验证 0 参与多参 `release` 为普通方法；全面同步 `cp_agent_ref.md`、`/data/dev/bun/CP_PITFALLS.md` 与 `/data/dev/capsurf/CP_PITFALLS.md` 并广播 `syntax-raii-20261121-v2`。

## [2026-11-21] - 升级 `cpc fix` 与编译诊断提示 + 一键迁移脚本 `scripts/migrate_cp_syntax.py` + 全仓库 `lib/` 与 `cap/test/` 新语法迁移
- **新增脚本**：`scripts/migrate_cp_syntax.py`（支持 `--dry-run`/`--apply`/`--no-release`/`--rewrite-release-calls`/`--show-audits`，严格跳过 `zig {}` 逃逸块与注释）。
- **修改 `cpc/src/{diagnostic.rs,parser/decl.rs,sema/mod.rs,codegen/{decl_enum,pattern,stmt,borrow,expr/ops,expr/call}.rs}`**：
  - `cpc fix` 修复 `E0121`（`*this`/`*self` 不再误生成 `**_`）、`E0201`（改为 `param: *T`）、`E0125`（补齐 `param: move/own T -> move/own param: T`）；
  - 全量更新编译报错/建议文案，彻底消除旧 `mut _` / `&mut` / `mut x` / `: void` 过时提示；
  - `Box<dyn Trait>` 补齐 `__cp_deinit` 自动析构支持；发布 Release 版 `cpc`。
- **批量迁移与验证**：对 `lib/` 与 `cap/test/` 共 78 个 `.cp` 文件完成新语法迁移（含全部 Bun 吸收资产 `~init()` 析构与调用点），11 组核心测试与 3 组 `verify_module.sh` 全量 100% PASS。
