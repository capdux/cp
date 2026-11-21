# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

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
