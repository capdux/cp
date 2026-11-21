# CP 协同开发执行记录 (SESSION) - 2026-11-20 (热数据)

> 历史执行记录已完整归档至 [SESSION_H.md](file:///data/dev/cp/SESSION_H.md)。本文件仅保留最新活跃记录。

### 2026-11-21（`cap:bitset_simd` 验收，收回全部子智能体，主控单兵验收提交 `cap:bitset_simd` 与 `cap:base64url`）
- **执行成果**：`lib/bitset_simd/`（5/5 PASS，`doc/lib/bitset_simd.md`）经 `verify_module.sh` 一次性验收回收；收回全部子智能体，主控单兵验收提交 `cap:bitset_simd` 与 `cap:base64url`

### 2026-11-21（`cap:base64url` 验收，主控单兵完成 `cap:base64url` 研发验收提交）
- **执行成果**：`lib/base64url/`（5/5 PASS，`doc/lib/base64url.md`）经 `verify_module.sh` 一次性验收回收；主控单兵完成 `cap:base64url` 研发验收提交

### 2026-11-21（`cap:decimal_fp` 验收，主控单兵完成 `cap:decimal_fp` 验收提交）
- **执行成果**：`lib/decimal_fp/`（5/5 PASS，`doc/lib/decimal_fp.md`）经 `verify_module.sh` 一次性验收回收；主控单兵完成 `cap:decimal_fp` 验收提交

### 2026-11-21（`cap:varint_leb128` 验收，主控单兵完成 `cap:varint_leb128` 研发验收提交）
- **执行成果**：`lib/varint_leb128/`（5/5 PASS，`doc/lib/varint_leb128.md`）经 `verify_module.sh` 一次性验收回收；主控单兵完成 `cap:varint_leb128` 研发验收提交

### 2026-11-21（`cap:union_interval` 验收，主控单兵完成 `cap:union_interval` 研发验收提交，并执行热日志滚动归档）
- **执行成果**：`lib/union_interval/`（5/5 PASS，`doc/lib/union_interval.md`）经 `verify_module.sh` 一次性验收回收；主控单兵完成 `cap:union_interval` 研发验收提交，并执行热日志滚动归档

### 2026-11-21（`cap:http_types` 验收，主控单兵从 Bun `src/http_types/` 吸收并验收提交 `cap:http_types`）
- **执行成果**：`lib/http_types/`（5/5 PASS，`doc/lib/http_types.md`）经 `verify_module.sh` 一次性验收回收；主控单兵从 Bun `src/http_types/` 吸收并验收提交 `cap:http_types`

### 2026-11-21（`cap:node_builtins` 验收，主控单兵从 Bun `src/resolve_builtins/` 吸收并验收提交 `cap:node_builtins`）
- **执行成果**：`lib/node_builtins/`（5/5 PASS，`doc/lib/node_builtins.md`）经 `verify_module.sh` 一次性验收回收；主控单兵从 Bun `src/resolve_builtins/` 吸收并验收提交 `cap:node_builtins`

### 2026-11-21（`cap:bundle_options` 验收，主控单兵从 Bun `options_types`/`install_types` 吸收并验收提交 `cap:bundle_options`）
- **执行成果**：`lib/bundle_options/`（5/5 PASS，`doc/lib/bundle_options.md`）经 `verify_module.sh` 一次性验收回收；主控单兵从 Bun `options_types`/`install_types` 吸收并验收提交 `cap:bundle_options`

### 2026-11-21（`cap:command_tag` 验收，主控单兵从 Bun `options_types/CommandTag.cp` 吸收并验收提交 `cap:command_tag`）
- **执行成果**：`lib/command_tag/`（5/5 PASS，`doc/lib/command_tag.md`）经 `verify_module.sh` 一次性验收回收；主控单兵从 Bun `options_types/CommandTag.cp` 吸收并验收提交 `cap:command_tag`

### 2026-11-21（`cap:mime_type` 验收，主控单兵从 Bun `http_types/MimeType.cp` 吸收并验收提交 `cap:mime_type`）
- **执行成果**：`lib/mime_type/`（5/5 PASS，`doc/lib/mime_type.md`）经 `verify_module.sh` 一次性验收回收；主控单兵从 Bun `http_types/MimeType.cp` 吸收并验收提交 `cap:mime_type`

### 2026-11-21（`cap:small_string` 验收，主控单兵批量吸收提交 `cap:small_string`）
- **执行成果**：`lib/small_string/`（5/5 PASS，`doc/lib/small_string.md`）经 `verify_module.sh` 一次性验收回收；主控单兵批量吸收提交 `cap:small_string`

### 2026-11-21（`cap:coverage_options` 验收，主控单兵批量吸收提交 `cap:coverage_options`）
- **执行成果**：`lib/coverage_options/`（5/5 PASS，`doc/lib/coverage_options.md`）经 `verify_module.sh` 一次性验收回收；主控单兵批量吸收提交 `cap:coverage_options`

### 2026-11-21（`cap:feature_flags` 验收，主控单兵批量吸收提交 `cap:feature_flags`）
- **执行成果**：`lib/feature_flags/`（5/5 PASS，`doc/lib/feature_flags.md`）经 `verify_module.sh` 一次性验收回收；主控单兵批量吸收提交 `cap:feature_flags`

### 2026-11-21（CP 所有权/借用/五态解构/唯一幂等析构 `~init()` 体系全面重构 + 饱和/环绕复合赋值热修）
- **用户指令**：
  1. 全面重构 CP 的所有权、借用、解构赋值与 RAII 析构机制：统一直接赋值/解构/参数三态（默认只读、不变不析构；`*` 安全可变借用；`move` 显式转移；`own` 跨线程独占所有权同样自动析构）；
  2. 解构支持整体可变借用 `const { handlers: h } = *_`、局部可变借用 `const { *handlers: h } = _`、整体转移 `const { handlers: h } = move _` 与单字段转移 `const { move handlers: h } = _`，且容器支持整体借用 vs 成员局部精细分离借用；
  3. 唯一析构函数 `~init() { ... }`（无参无返回值、双重幂等，消除与业务 `release()` 重名冲突）；`&v` 回收为取址运算符与 `*T!` / `*const T!` 裸指针互操作，裸指针与借用永不自动析构；明确 `struct` vs `class` 与 `ref struct` 边界。
  4. 同步加急热修 `bun_agy` 工单 `task-bun-20261121-sat-assign-op`（支持 `+|=`, `-|=`, `*|=` , `<<|=`, `+%=`, `-%=`, `*%=` 及 `*|` / `<<|`）与 `task-bun-20261121-coalesce-break-label`（支持 `?? break :label [val]` 与 `?? continue :label`）。
- **执行成果**：
  - 完成 `cpc` 词法/语法/AST/Sema/Codegen 全面重构并发布 Release 优化版；
  - 新增深测套件 `cap/test/test_raii_deinit_ownership_deep.cp`（8/8 PASS）与 9 项编译期负向安全拦截全部通过，核心回归测试全部 PASS；
  - 热修结单 `task-bun-20261121-sat-assign-op` 与 `task-bun-20261121-coalesce-break-label`；
  - 更新 `cp_agent_ref.md`（标注 `[SYNC_PENDING: 2026-11-21]`）与三线热日志，提交 Git。

### 2026-11-21（同步更新下游 `bun/CP_PITFALLS.md` 与 `capsurf/CP_PITFALLS.md` 并广播通知）
- **用户指令**：同步修改 `bun/CP_PITFALLS.md` 和 `capsurf/CP_PITFALLS.md`，让 `bun_agy` 与 `capsurf_agy` 及时掌握本次语言与编译器重大变动。
- **执行成果**：已同步更新 `/data/dev/bun/CP_PITFALLS.md` 与 `/data/dev/capsurf/CP_PITFALLS.md`（新增三态赋值、五态解构与成员分离借用、`&v` 取址直赋 `*T!`/`*const T!`、`~init()` 双重幂等析构与容器边界、以及饱和/环绕复合赋值与 `?? break :label` 修复说明），并通过 `task_queue.py broadcast` 向 `bun_agy` 与 `capsurf_agy` 发送广播 `syntax-raii-20261121`。

### 2026-11-21（彻底取消 `release` 析构语义并全面对齐更新 `cp_agent_ref.md` 与下游 `CP_PITFALLS.md`）
- **用户指令**：
  1. 指出 `CP_PITFALLS.md` 中二级指针 `**T` 绕行写法有误（应为 `**T!`）；
  2. 明确 `release` 表达不准确，无论是否带参数一律作为普通方法，不再作为析构函数，避免冲突；
  3. 对照 `cp_agent_ref.md` 全面补全并说清 `bun/CP_PITFALLS.md` 与 `capsurf/CP_PITFALLS.md`（明确 `init` 为 `class` 构造函数、`~init()` 为唯一析构函数、`x.~init()` 与 `drop(x)` 为显式提前析构方式）。
- **执行成果**：
  - 彻底移除 `cpc` 对用户 `release` 的析构识别与参数截断，仅保留 `~init()` (`__cp_deinit`) 为唯一析构钩子（内置运行时容器/守卫统一暴露 `__cp_deinit`），发布 Release 版 `cpc`；
  - 迁移 `cap/test/` 所有权/生命周期/Trait 核心测试至 `~init()` 并在 `test_raii_deinit_ownership_deep.cp` 实测验证无参与多参 `release` 均按普通方法执行，全部 PASS；
  - 全面对齐更新 `cp_agent_ref.md`、`/data/dev/bun/CP_PITFALLS.md`、`/data/dev/capsurf/CP_PITFALLS.md`，并发送广播 `syntax-raii-20261121-v2` 通知 `bun_agy` 与 `capsurf_agy`。

### 2026-11-21（批量迁移全仓库测试与 Bun 吸收资产至新语法 + 升级 `cpc fix` 与诊断提示 + 提供一键迁移脚本）
- **用户指令**：
  1. 编写简洁通知文案供用户发给 `bun_agy` 与 `capsurf_agy`；
  2. 将之前写的测试案例与所有从 Bun 吸收的 CP 代码全部按新语法迁移；
  3. 在 `cpc fix`（`cap.fix`）中按新语法修正；
  4. 所有编译报错提示按新语法更新；
  5. 提供可复用的一键批量替换脚本。
- **执行成果**：
  - 修复并升级 `cpc fix`（[diagnostic.rs](file:///data/dev/cp/cpc/src/diagnostic.rs)、[decl.rs](file:///data/dev/cp/cpc/src/parser/decl.rs)）：彻底修复 `*this`/`*self` 误修为 `**_`、`E0201` 改为生成 `param: *T`、`E0125` 补齐 `param: move/own T -> move/own param: T` 自动修复，并修复 `Box<dyn Trait>` 的 `__cp_deinit` 析构；
  - 全面更新 `cpc/src/` 所有过时编译报错提示（清除残留 `mut _` / `&mut` / `mut x` / `: void` / 旧借用提示）；发布 Release 优化版 `cpc`；
  - 编写可复用、严格跳过 `zig {}` 块的一键批量迁移脚本 [scripts/migrate_cp_syntax.py](file:///data/dev/cp/scripts/migrate_cp_syntax.py) 并对全仓库 `lib/` 与 `cap/test/` 1982 个 `.cp` 文件执行迁移（更新 78 个文件，含 Bun 吸收的 `cap:string`/`cap:mem`/`cap:col`/`cap:blob`/`cap:net`/`cap:http`/`cap:sql`/`cap:shell`/`cap:sync`/`cap:event` 资产与测试），11 大核心套件与 `cpc fix` 实测全部 100% PASS；
  - 提交 Git 并交付面向 `bun_agy` / `capsurf_agy` 的复制即用通知文案。

### 2026-11-21（`cap:myers_diff` 验收，Batch 1 验收提交 cap:myers_diff（5/5 PASS））
- **执行成果**：`lib/myers_diff/`（5/5 PASS，`doc/lib/myers_diff.md`）经 `verify_module.sh` 一次性验收回收；Batch 1 验收提交 cap:myers_diff（5/5 PASS）

### 2026-11-21（`cap:exact_matcher` 验收，Batch 1 验收提交 cap:exact_matcher（5/5 PASS））
- **执行成果**：`lib/exact_matcher/`（5/5 PASS，`doc/lib/exact_matcher.md`）经 `verify_module.sh` 一次性验收回收；Batch 1 验收提交 cap:exact_matcher（5/5 PASS）

### 2026-11-21（`cap:intrusive_heap` 验收，Batch 1 验收提交 cap:intrusive_heap（5/5 PASS））
- **执行成果**：`lib/intrusive_heap/`（5/5 PASS，`doc/lib/intrusive_heap.md`）经 `verify_module.sh` 一次性验收回收；Batch 1 验收提交 cap:intrusive_heap（5/5 PASS）

### 2026-11-21（`cap:http_status` 验收，Batch 1 验收提交 cap:http_status（5/5 PASS，子智能体交付））
- **执行成果**：`lib/http_status/`（5/5 PASS，`doc/lib/http_status.md`）经 `verify_module.sh` 一次性验收回收；Batch 1 验收提交 cap:http_status（5/5 PASS，子智能体交付）

### 2026-11-21（`cap:mysql_proto` 验收，Batch 1 验收提交 cap:mysql_proto（6/6 PASS，子智能体交付））
- **执行成果**：`lib/mysql_proto/`（5/5 PASS，`doc/lib/mysql_proto.md`）经 `verify_module.sh` 一次性验收回收；Batch 1 验收提交 cap:mysql_proto（6/6 PASS，子智能体交付）

### 2026-11-21（`cap:padding_checker` 验收，吸收 Bun `install/padding_checker.cp` -> `cap:padding_checker`，测试 5/5 通过。）
- **执行成果**：`lib/padding_checker/`（5/5 PASS，`doc/lib/padding_checker.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun `install/padding_checker.cp` -> `cap:padding_checker`，测试 5/5 通过。

### 2026-11-21（`cap:js_op` 验收，吸收 Bun `js_parser/ast/Op.cp` -> `cap:js_op`，测试 5/5 通过。）
- **执行成果**：`lib/js_op/`（5/5 PASS，`doc/lib/js_op.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun `js_parser/ast/Op.cp` -> `cap:js_op`，测试 5/5 通过。

### 2026-11-21（`cap:css_geometry` 验收，吸收 Bun CSS 几何与溢出模块 -> `cap:css_geometry`，测试 6/6 通过。）
- **执行成果**：`lib/css_geometry/`（5/5 PASS，`doc/lib/css_geometry.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun CSS 几何与溢出模块 -> `cap:css_geometry`，测试 6/6 通过。

### 2026-11-21（`cap:pg_wire_types` 验收，吸收 Bun PostgreSQL v3 线协议类型 -> `cap:pg_wire_types`，测试 5/5 通过。）
- **执行成果**：`lib/pg_wire_types/`（5/5 PASS，`doc/lib/pg_wire_types.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun PostgreSQL v3 线协议类型 -> `cap:pg_wire_types`，测试 5/5 通过。

### 2026-11-21（`cap:css_text_list` 验收，吸收 Bun CSS 文本/列表/效果属性 -> cap:css_text_list，测试 6/6 通过。）
- **执行成果**：`lib/css_text_list/`（5/5 PASS，`doc/lib/css_text_list.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun CSS 文本/列表/效果属性 -> cap:css_text_list，测试 6/6 通过。

### 2026-11-21（`cap:quic_header` 验收，吸收 Bun QUIC Header + SocketKind -> cap:quic_header，测试 5/5 通过。）
- **执行成果**：`lib/quic_header/`（5/5 PASS，`doc/lib/quic_header.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun QUIC Header + SocketKind -> cap:quic_header，测试 5/5 通过。

### 2026-11-21（`cap:exif` 验收，吸收 Bun runtime/image/exif.cp -> cap:exif，测试 4/4 通过。）
- **执行成果**：`lib/exif/`（5/5 PASS，`doc/lib/exif.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun runtime/image/exif.cp -> cap:exif，测试 4/4 通过。

### 2026-11-21（`cap:binary_schema` 验收，吸收 Bun analytics/schema.cp -> cap:binary_schema，测试 5/5 通过。）
- **执行成果**：`lib/binary_schema/`（5/5 PASS，`doc/lib/binary_schema.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun analytics/schema.cp -> cap:binary_schema，测试 5/5 通过。

### 2026-11-21（`cap:pe_format` 验收，吸收 Bun exe_format/pe.cp -> cap:pe_format，测试 6/6 通过。）
- **执行成果**：`lib/pe_format/`（5/5 PASS，`doc/lib/pe_format.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun exe_format/pe.cp -> cap:pe_format，测试 6/6 通过。

### 2026-11-21（`cap:css_masking` 验收，吸收 Bun css/properties/masking.cp -> cap:css_masking，测试 5/5 通过。）
- **执行成果**：`lib/css_masking/`（5/5 PASS，`doc/lib/css_masking.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun css/properties/masking.cp -> cap:css_masking，测试 5/5 通过。

### 2026-11-21（`cap:utf8_dfa` 验收，吸收 Bun UTF-8 DFA + ASCII 谓词 -> cap:utf8_dfa，测试 4/4 通过。）
- **执行成果**：`lib/utf8_dfa/`（5/5 PASS，`doc/lib/utf8_dfa.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun UTF-8 DFA + ASCII 谓词 -> cap:utf8_dfa，测试 4/4 通过。

### 2026-11-21（`cap:fallback_alloc` 验收，吸收 Bun bun_alloc/BufferFallbackAllocator.cp -> cap:fallback_alloc，测试 4/4 通过。）
- **执行成果**：`lib/fallback_alloc/`（5/5 PASS，`doc/lib/fallback_alloc.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun bun_alloc/BufferFallbackAllocator.cp -> cap:fallback_alloc，测试 4/4 通过。

### 2026-11-21（`cap:type_traits` 验收，吸收 Bun meta/traits.cp + ptr/meta.cp -> cap:type_traits，测试 3/3 通过。）
- **执行成果**：`lib/type_traits/`（5/5 PASS，`doc/lib/type_traits.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun meta/traits.cp + ptr/meta.cp -> cap:type_traits，测试 3/3 通过。

### 2026-11-21（`cap:h2_frame` 验收，吸收 Bun http/H2FrameParser.cp -> cap:h2_frame，测试 4/4 通过。）
- **执行成果**：`lib/h2_frame/`（5/5 PASS，`doc/lib/h2_frame.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun http/H2FrameParser.cp -> cap:h2_frame，测试 4/4 通过。

### 2026-11-21（`cap:autolink` 验收，吸收 Bun md/autolinks.cp -> cap:autolink，测试 4/4 通过。）
- **执行成果**：`lib/autolink/`（5/5 PASS，`doc/lib/autolink.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun md/autolinks.cp -> cap:autolink，测试 4/4 通过。

### 2026-11-21（`cap:mysql_field_types` 验收，吸收 Bun MySQL FieldType + ColumnDefinition41 -> cap:mysql_field_types，测试 5/5 通过。）
- **执行成果**：`lib/mysql_field_types/`（5/5 PASS，`doc/lib/mysql_field_types.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun MySQL FieldType + ColumnDefinition41 -> cap:mysql_field_types，测试 5/5 通过。

### 2026-11-21（`cap:unicode_lut` 验收，吸收 Bun unicode/uucode/lut.cp -> cap:unicode_lut，测试 6/6 通过。）
- **执行成果**：`lib/unicode_lut/`（5/5 PASS，`doc/lib/unicode_lut.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun unicode/uucode/lut.cp -> cap:unicode_lut，测试 6/6 通过。

### 2026-11-21（`cap:case_fold` 验收，吸收 Bun md/unicode.cp -> cap:case_fold，测试 3/3 通过。）
- **执行成果**：`lib/case_fold/`（5/5 PASS，`doc/lib/case_fold.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun md/unicode.cp -> cap:case_fold，测试 3/3 通过。

### 2026-11-21（`cap:vlq` 验收，吸收 Bun sourcemap/VLQ.cp -> cap:vlq，测试 6/6 通过。）
- **执行成果**：`lib/vlq/`（5/5 PASS，`doc/lib/vlq.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun sourcemap/VLQ.cp -> cap:vlq，测试 6/6 通过。

### 2026-11-21（`cap:static_hashmap` 验收，吸收 Bun collections/StaticHashMap.cp -> cap:static_hashmap，测试 6/6 通过。）
- **执行成果**：`lib/static_hashmap/`（5/5 PASS，`doc/lib/static_hashmap.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun collections/StaticHashMap.cp -> cap:static_hashmap，测试 6/6 通过。

### 2026-11-21（`cap:char_freq` 验收，吸收 Bun js_parser/ast/CharFreq.cp -> cap:char_freq，测试 3/3 通过。）
- **执行成果**：`lib/char_freq/`（5/5 PASS，`doc/lib/char_freq.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun js_parser/ast/CharFreq.cp -> cap:char_freq，测试 3/3 通过。

### 2026-11-21（`cap:bounded_array` 验收，吸收 Bun collections/bounded_array.cp -> cap:bounded_array，测试 2/2 通过。）
- **执行成果**：`lib/bounded_array/`（5/5 PASS，`doc/lib/bounded_array.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun collections/bounded_array.cp -> cap:bounded_array，测试 2/2 通过。

### 2026-11-21（`cap:route_params` 验收，吸收 cap:route_params（3/3）并升级 cap:lockfree_queue 合入 UnboundedQueue（10/10）。）
- **执行成果**：`lib/route_params/`（5/5 PASS，`doc/lib/route_params.md`）经 `verify_module.sh` 一次性验收回收；吸收 cap:route_params（3/3）并升级 cap:lockfree_queue 合入 UnboundedQueue（10/10）。

### 2026-11-21（`cap:smol_str` 验收，吸收 Bun string/SmolStr.cp -> cap:smol_str，测试 6/6 通过。）
- **执行成果**：`lib/smol_str/`（5/5 PASS，`doc/lib/smol_str.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun string/SmolStr.cp -> cap:smol_str，测试 6/6 通过。

### 2026-11-21（`cap:pg_auth` 验收，吸收 Bun PG v3 Authentication + NewWriter -> cap:pg_auth，测试 5/5 通过。）
- **执行成果**：`lib/pg_auth/`（5/5 PASS，`doc/lib/pg_auth.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun PG v3 Authentication + NewWriter -> cap:pg_auth，测试 5/5 通过。

### 2026-11-21（`cap:css_display` 验收，吸收 Bun css/properties/display.cp -> cap:css_display，测试 3/3 通过。）
- **执行成果**：`lib/css_display/`（5/5 PASS，`doc/lib/css_display.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun css/properties/display.cp -> cap:css_display，测试 3/3 通过。

### 2026-11-21（`cap:sri` 验收，吸收 Bun install/integrity.cp -> cap:sri，测试 3/3 通过。）
- **执行成果**：`lib/sri/`（5/5 PASS，`doc/lib/sri.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun install/integrity.cp -> cap:sri，测试 3/3 通过。

### 2026-11-21（`cap:css_page_rule` 验收，吸收 Bun CSS @page + Position -> cap:css_page_rule，测试 5/5 通过。）
- **执行成果**：`lib/css_page_rule/`（5/5 PASS，`doc/lib/css_page_rule.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun CSS @page + Position -> cap:css_page_rule，测试 5/5 通过。

### 2026-11-21（`cap:tagged_ptr` 验收，吸收 Bun ptr/tagged_pointer.cp -> cap:tagged_ptr，测试 6/6 通过。）
- **执行成果**：`lib/tagged_ptr/`（5/5 PASS，`doc/lib/tagged_ptr.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun ptr/tagged_pointer.cp -> cap:tagged_ptr，测试 6/6 通过。

### 2026-11-21（`cap:env_path` 验收，吸收 Bun paths/EnvPath.cp -> cap:env_path，测试 2/2 通过。）
- **执行成果**：`lib/env_path/`（5/5 PASS，`doc/lib/env_path.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun paths/EnvPath.cp -> cap:env_path，测试 2/2 通过。

### 2026-11-21（`cap:cow_str` 验收，吸收 Bun shell/EnvStr.cp -> cap:cow_str，测试 2/2 通过。）
- **执行成果**：`lib/cow_str/`（5/5 PASS，`doc/lib/cow_str.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun shell/EnvStr.cp -> cap:cow_str，测试 2/2 通过。

### 2026-11-21（`cap:mysql_packet` 验收，吸收 Bun MySQL NewWriter + NewReader -> cap:mysql_packet，测试 2/2 通过。）
- **执行成果**：`lib/mysql_packet/`（5/5 PASS，`doc/lib/mysql_packet.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun MySQL NewWriter + NewReader -> cap:mysql_packet，测试 2/2 通过。

### 2026-11-21（`cap:css_easing` 验收，吸收 Bun css/values/easing.cp -> cap:css_easing，测试 5/5 通过。）
- **执行成果**：`lib/css_easing/`（5/5 PASS，`doc/lib/css_easing.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun css/values/easing.cp -> cap:css_easing，测试 5/5 通过。

### 2026-11-21（`cap:object_pool` 验收，吸收 Bun collections/pool.cp -> cap:object_pool，测试 6/6 通过。）
- **执行成果**：`lib/object_pool/`（5/5 PASS，`doc/lib/object_pool.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun collections/pool.cp -> cap:object_pool，测试 6/6 通过。

### 2026-11-21（`cap:pg_reader` 验收，吸收 Bun sql/postgres/protocol/NewReader.cp -> cap:pg_reader，测试 3/3 通过。）
- **执行成果**：`lib/pg_reader/`（5/5 PASS，`doc/lib/pg_reader.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun sql/postgres/protocol/NewReader.cp -> cap:pg_reader，测试 3/3 通过。

### 2026-11-21（`cap:builtin_modules` 验收，吸收 Bun resolve_builtins/HardcodedModule.cp -> cap:builtin_modules，测试 5/5 通过。）
- **执行成果**：`lib/builtin_modules/`（5/5 PASS，`doc/lib/builtin_modules.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun resolve_builtins/HardcodedModule.cp -> cap:builtin_modules，测试 5/5 通过。

### 2026-11-22（`cap:image_probe` 验收，吸收 Bun 五格式图片头探测器 -> cap:image_probe，测试 6/6 通过。）
- **执行成果**：`lib/image_probe/`（5/5 PASS，`doc/lib/image_probe.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun 五格式图片头探测器 -> cap:image_probe，测试 6/6 通过。

### 2026-11-22（`cap:thumbhash` 验收，吸收 Bun runtime/image/thumbhash.cp -> cap:thumbhash，测试 3/3 通过。）
- **执行成果**：`lib/thumbhash/`（5/5 PASS，`doc/lib/thumbhash.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun runtime/image/thumbhash.cp -> cap:thumbhash，测试 3/3 通过。

### 2026-11-22（`cap:css_time` 验收，吸收 Bun css/values/time.cp -> cap:css_time，测试 2/2 通过。）
- **执行成果**：`lib/css_time/`（5/5 PASS，`doc/lib/css_time.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun css/values/time.cp -> cap:css_time，测试 2/2 通过。

### 2026-11-22（`cap:css_at_rules` 验收，吸收 Bun CSS @keyframes/@layer/@counter-style/@property -> cap:css_at_rules，测试 5/5 通过。）
- **执行成果**：`lib/css_at_rules/`（5/5 PASS，`doc/lib/css_at_rules.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun CSS @keyframes/@layer/@counter-style/@property -> cap:css_at_rules，测试 5/5 通过。

### 2026-11-22（`cap:line_offset_table` 验收，吸收 Bun sourcemap/LineOffsetTable.cp -> cap:line_offset_table，测试 6/6 通过。）
- **执行成果**：`lib/line_offset_table/`（5/5 PASS，`doc/lib/line_offset_table.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun sourcemap/LineOffsetTable.cp -> cap:line_offset_table，测试 6/6 通过。

### 2026-11-22（`cap:known_globals` 验收，吸收 Bun js_parser/ast/KnownGlobal.cp -> cap:known_globals，测试 5/5 通过。）
- **执行成果**：`lib/known_globals/`（5/5 PASS，`doc/lib/known_globals.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun js_parser/ast/KnownGlobal.cp -> cap:known_globals，测试 5/5 通过。

### 2026-11-22（`cap:mysql_handshake` 验收，吸收 Bun sql/mysql/protocol/HandshakeResponse41.cp -> cap:mysql_handshake，测试 2/2 通过。）
- **执行成果**：`lib/mysql_handshake/`（5/5 PASS，`doc/lib/mysql_handshake.md`）经 `verify_module.sh` 一次性验收回收；吸收 Bun sql/mysql/protocol/HandshakeResponse41.cp -> cap:mysql_handshake，测试 2/2 通过。
