# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:agent` 纯 CP 零分配 AI Agent 统一运行时聚合主包：四维 Token/微美元成本/步数/超时预算控制、LoopGuard 死循环检测、ToolCircuitBreaker 工具熔断与 ContextWindowAdvisor 上下文压缩建议（5/5 PASS，自动动态挂载 `cap.agent`）
新建统一 AI Agent 聚合主包 cap:agent 并交付首个核心子模块 budget.cp（四维预算、死循环护栏、工具熔断、上下文顾问），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:keyframes` 吸收自 Bun css/rules/keyframes.cp 的纯 CP 零分配 CSS @keyframes 解析、压缩与时间轴采样库：保留字拦截、from/to 最优压缩与关键帧区间插值采样（5/5 PASS，自动动态挂载 `cap.keyframes`）
吸收 Bun 上游 css/rules/keyframes.cp 为纯 CP 零分配库 cap:keyframes（CSS @keyframes 解析、压缩与采样），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:byterange` 吸收自 Bun runtime/server/RangeRequest.cp 的纯 CP 零分配 HTTP Range（RFC 9110）单/多段解析与 Content-Range 206/416 编解码库（5/5 PASS，自动动态挂载 `cap.byterange`）
吸收 Bun 上游 runtime/server/RangeRequest.cp 为纯 CP 零分配库 cap:byterange（HTTP Range & Content-Range），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:catalog` 吸收自 Bun install/lockfile/CatalogMap.cp 的纯 CP 零分配 Monorepo 依赖目录协议库：catalog:/catalog:<group> 解析、默认与命名分组管理及确定性 Lockfile 输出（5/5 PASS，自动动态挂载 `cap.catalog`）
吸收 Bun 上游 install/lockfile/CatalogMap.cp 为纯 CP 零分配库 cap:catalog（Monorepo 依赖目录协议与解析），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:bloburl` 吸收自 Bun runtime/webcore/ObjectURLRegistry.cp 的纯 CP 零分配 W3C blob: Object URL 解析、RFC 4122 UUID v4 生成与固定池注册表库（5/5 PASS，自动动态挂载 `cap.bloburl`）
吸收 Bun 上游 ObjectURLRegistry.cp 为纯 CP 零分配库 cap:bloburl（blob: URL 解析与固定池注册表），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:textdecode` 吸收自 Bun runtime/webcore/TextDecoder.cp 的纯 CP 零分配 WHATWG 流式文本解码器：UTF-8/UTF-16LE/UTF-16BE/Latin-1 跨块重组、BOM 探测剥离与 fatal/U+FFFD 替换模式（5/5 PASS，自动动态挂载 `cap.textdecode`）
吸收 Bun 上游 TextDecoder.cp 为纯 CP 零分配库 cap:textdecode（WHATWG 流式文本解码器），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:cssmodules` 吸收自 Bun css/css_modules.cp 的纯 CP 零分配 CSS Modules 命名模板、Wyhash+Base64 确定性哈希、作用域类名编译与 composes 跨文件依赖解析库（5/5 PASS，自动动态挂载 `cap.cssmodules`）
吸收 Bun 上游 css/css_modules.cp 为纯 CP 零分配库 cap:cssmodules（CSS Modules 命名模板、哈希与 composes 解析），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:mediaquery` 吸收自 Bun css/media_query.cp 的纯 CP 零分配 W3C CSS Media Queries Level 4 库：长度/分辨率/宽高比单位换算、单/双边区间范围、媒体特性求值与规范化（5/5 PASS，自动动态挂载 `cap.mediaquery`）
吸收 Bun 上游 css/media_query.cp 为纯 CP 零分配库 cap:mediaquery（CSS Media Queries Level 4 解析/求值/规范化），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:textstream` 吸收自 Bun runtime/webcore/TextEncoderStreamEncoder.cp 的纯 CP 零分配 WHATWG 流式 UTF-8 编码器：跨分块 UTF-16 代理对拼接、孤立代理 U+FFFD 替换、Latin-1 转码与小缓冲原子保护（5/5 PASS，自动动态挂载 `cap.textstream`）
吸收 Bun 上游 TextEncoderStreamEncoder.cp 为纯 CP 零分配库 cap:textstream（WHATWG 流式 UTF-8 编码器），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:h2stream` 吸收自 Bun http/h2_client/Stream.cp 的纯 CP 零分配 HTTP/2（RFC 9113）流状态机与双向流控窗口库：半关闭生命周期、14 种错误码、RST_STREAM/WINDOW_UPDATE 帧编码与自动补窗（5/5 PASS，自动动态挂载 `cap.h2stream`）
吸收 Bun 上游 http/h2_client/Stream.cp 为纯 CP 零分配库 cap:h2stream（RFC 9113 HTTP/2 流状态机与流控窗口），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:qpack` 吸收自 Bun uws_sys/quic/Header.cp 与 http/h3_client/encode.cp 的纯 CP 零分配 HTTP/3 QPACK（RFC 9204）编解码库：静态表分类、多前缀整数编解码、请求伪头部编排与 Field Section 零拷贝往返（5/5 PASS，自动动态挂载 `cap.qpack`）
吸收 Bun 上游 uws_sys/quic/Header.cp & h3_client/encode.cp 为纯 CP 零分配库 cap:qpack（HTTP/3 QPACK RFC 9204），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:h2_encode` 吸收自 Bun http/h2_client/encode.cp 的纯 CP 零分配 HTTP/2 出站编码库：RFC 9113 请求头过滤、HPACK 动态表更新、客户端序言、HEADERS/CONTINUATION 分片与双窗口 DATA 帧编码（5/5 PASS，自动动态挂载 `cap.h2_encode`）
吸收 Bun 上游 src/http/h2_client/encode.cp 为纯 CP 零分配库 cap:h2_encode，含文档与 6 项深度生产测试


## [2026-11-26] - 完成 CP & CPC 生态 12 项核心优化（全部提交并发布 release 版 cpc）
- **#1 (`a1220ad0`)** `cpc/src/codegen/mod.rs`, `cpc/src/codegen/actor.rs`, `cap/test/testExternDedup.cp`: 跨模块相同顶层 `extern fn` 与相同顶层 `const` 自动去重（保留冲突 `const` 报错）。
- **#2 (`540f8c66`)** `cpc/src/codegen/expr/error_flow.rs`, `cap/test/testNullCoalesceShortCircuit.cp`: `??` 空值合并改为原生 Zig `orelse` + `comptime @typeInfo` 严格惰性短路求值。
- **#3 / #6 (`9731de3d`)** `cpc/src/parser/mod.rs`, `cpc/src/diagnostic.rs`, `cpc/src/codegen/stmt.rs`: 新增 AI-First 三段式诊断与 `--fix` 自动修复（`E0131` 保留字 `dec`、`E0132` 可选切片 `[]T?`→`([]T)?`、`E0204` 最近 `const`→`let`）。
- **#4 (`96fa41f4`)** `cpc/tests/golden_and_fuzz.rs`, `cpc/src/parser/{mod,expr,stmt,primary,pattern,types}.rs`: 新增 5 项 Golden Codegen/Diagnostic 回归测试 + 300 轮确定性 Parser/Sema 零 Panic Fuzz 测试与 64 层递归深度护栏（6/6 PASS）。
- **#5 (`520ed0c6`)** `cpc/src/mcp.rs`, `cpc/src/lib.rs`, `cpc/src/main.rs`: 内置 `cpc mcp` stdio JSON-RPC 2.0 工具链服务端（`cpc_check` / `cpc_brief` / `cpc_emit_types` / `cpc_search_stdlib`，5/5 单测 PASS）。
- **#7 / #8 (`822d0594`)** `packages/@web/*/src/index.d.cp`, `cpc/src/main.rs`, `cpc/src/migrate/{deps,mod}.rs`: 生成 33 个官方 `@web/*` 包 `.d.cp` 极简接口存根（体积 -87.5%，节省约 64.3K AI Token）并支持 `cap from -ts/-rs --verify` 自动校验修复闭环（11/11 单测 PASS）。
- **#9 (`c43ebf0a`)** `cap/src/main.zig`: `cap` 多级自动发现 `cpc`/`zig` 二进制，新增 Linux `inotify` 50ms 防抖热重载监听。
- **#10 / #11 (`ce7d60c4`)** `cap.json`, `cap/src/main.zig`: 根 `cap.json` 挂载 `packages/@web/*` workspace，新增 `cap init web <dir>` 生产级脚手架、`cap test --filter/-k <pat>` 过滤与 `cap demangle` 提示。
- **#12 (`d2043c04`)** `lib/web/tls.cp`, `lib/web/server.cp`, `lib/web/index.cp`, `cap/test/testWebTlsConfig.cp`: 纯 CP TLS 1.2/1.3 配置、PEM 校验、ClientHello 探测、零分配 SNI 提取与 ALPN 协商（6/6 PASS）。
