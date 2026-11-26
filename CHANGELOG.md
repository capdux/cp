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

## [2026-11-26] - 审查收口 `cap:css` CSSOM 值校验器与 Color 4 / Stylesheet / `@supports` 规范增强（79/79 CSS 测试全绿）
- `lib/css/*.cp`（26 个文件）: 审查并收口 CapSurf 下沉的 59 项 `isValidCss*` CSSOM 属性值校验器、`CssColor` Color 4（`calc()` / `none` / `lab()` / `lch()` / `color()` / 注释剥离 / CSSOM 序列化）、`CssProperty.parse`（注释剥离 / 拒绝重复 `!important` / `--custom: ` 空白值）与 `@supports`（自定义属性与 `\` 转义字符），零破坏性变更、26/26 单文件 `cpc check` 通过。
- `cap/test/testCssValidationAndColor4.cp`: 新增 12 项专项单元测试，覆盖 40 个 `isValidCss*` 函数合法/非法边界、Color 4、Stylesheet 与 `@supports` 增强（PASSED 947ms）。
- `cap/test/test_css_full.cp`: 修正 L96 历史遗留断言 `"rgba(59, 130, 246, 128)"` → W3C 标准 `"rgba(59, 130, 246, 0.501961)"`，全量 CSS 回归达到 79/79 PASS。
- `doc/lib/css.md`, `cp_lib_ref.md`: 同步登记全部 59 项新增 CSSOM 校验 API 与解析增强契约。

## [2026-11-26] - Wave 1 生态丰富与标准库深度优化（新增 `@cap/blake3`，深度优化 `cap:slugify` / `cap:sri` / `cap:etag` / `cap:logfmt`）
- `packages/@cap/blake3/` (`cap.json`, `src/index.cp`, `src/index.d.cp`, `README.md`) & `cap/test/testCapBlake3.cp`: 新增纯 CP 零外部依赖 `@cap/blake3` 包（对标 Rust 官方 `blake3` crate，含完整 54 层 Merkle 树、`hash`/`keyedHash`/`deriveKey`/`Blake3Hasher`/`finalizeXof`，官方向量逐字节交叉验证 PASS）。
- `lib/slugify/` & `doc/lib/slugify.md` & `cap/test/testSlugifyDeep.cp`: 对标 Rust `slug`+`deunicode` crate 深度优化，新增 Latin-1/Extended-A 音译表、`deunicodeChar`/`deunicode`、`SlugOptions`（保留大小写、符号展开、词边界截断）、`isValidSlugWithSep` 与 3 组可运行文档示例（5/5 PASS）。
- `lib/sri/` & `doc/lib/sri.md` & `cap/test/testSriDeep.cp`: 对标 Rust `ssri` crate 深度优化，新增 W3C SRI §3.3.3 多候选最强算法集合防降级校验 `IntegrityList`、`Integrity.toHex` 与 3 组文档示例（3/3 PASS）。
- `lib/etag/` & `doc/lib/etag.md` & `cap/test/testEtagDeep.cp`: 对标 Rust `etag` crate + RFC 9110 §8.8.3/§13.1/§13.2.2 深度优化，新增严格 `EntityTag`/`etagc` 校验、128-bit+len/SHA-256/`fromStat` 生成器、引号逗号安全 `ETagListIter`、`If-Range` 与 304/412 `evaluatePreconditions` 决策机（5/5 PASS）。
- `lib/logfmt/` & `doc/lib/logfmt.md` & `cap/test/testLogfmtDeep.cp`: 对标 Rust `logfmt`/Go `go-logfmt` 深度优化，新增原子防注入构造器 `LogfmtEncoder`、类型化解析 `asInt`/`asUint`/`asFloat`/`asBool`、`getLogfmtInt`/`getLogfmtBool`/`countLogfmtFields` 与文档示例（3/3 PASS）。
- `cp_lib_ref.md`: 同步更新 `cap:slugify`、`cap:sri`、`cap:etag`、`cap:logfmt` 索引契约。

## [2026-11-26] - Wave 2A 生态丰富与标准库深度优化（新增 `@cap/msgpack`，深度优化 `cap:ulid` / `cap:jsonc`）
- `packages/@cap/msgpack/` (`cap.json`, `src/index.cp`, `src/index.d.cp`, `README.md`) & `cap/test/testCapMsgpack.cp`: 新增纯 CP 零外部依赖 `@cap/msgpack` 包（对标 Rust `rmp` crate，完整 MessagePack 编解码器、`skipValue` 深度护栏，PASSED 1042ms）。
- `lib/ulid/` & `doc/lib/ulid.md` & `cap/test/testUlidDeep.cp`: 对标 Rust `ulid` crate 新增 `fromU128`/`toU128`/`increment()`（80-bit 防溢出），修复 `generator.cp` 在 Zig 0.17 下的 CSPRNG 兼容性，补齐 3 组可运行文档示例（3/3 PASS）。
- `lib/jsonc/` & `doc/lib/jsonc.md` & `cap/test/testJsoncDeep.cp`: 对标 Rust `json_comments` crate 新增 `JsoncOptions`/`stripJsoncWithOptions`/`stripJsoncWithSpaces`/`stripJsoncInPlace`（零额外缓冲原地剥离，1:1 保持行列偏移），补齐文档示例（3/3 PASS）。
- `cp_lib_ref.md`: 同步更新 `cap:ulid`、`cap:jsonc` 索引契约。

## [2026-11-26] - Wave 2B 生态丰富与标准库深度优化（新增 `@cap/cbor`，深度优化 `cap:rate`）
- `packages/@cap/cbor/` (`cap.json`, `src/index.cp`, `src/index.d.cp`, `README.md`) & `cap/test/testCapCbor.cp`: 新增纯 CP 零外部依赖 `@cap/cbor` 包（严格对标 RFC 8949 与 Rust `minicbor`/`ciborium` crate，含 `CborEncoder`/`CborDecoder`、`MAX_CBOR_DEPTH=128` 护栏与 Appendix A 测试向量，PASSED 885ms）。
- `lib/rate/` (`gcra.cp`, `bucket.cp`, `sliding.cp`, `limiter.cp`, `index.cp`) & `doc/lib/rate.md` & `cap/test/testRateDeep.cp`: 对标 Rust `governor` crate 新增全 `u128` 防溢出 GCRA 限流器（`Quota` / `RateDecision` / `GcraLimiter` / `checkNAt` / IETF `RateLimit-*` & `Retry-After` 响应头格式化器），修复旧 `self` 语法，补齐 3 组可运行文档示例（6/6 PASS）。
- `cp_lib_ref.md`: 同步更新 `cap:rate` 索引契约。

## [2026-11-26] - Wave 3A 吸收 npm/Rust 亿级高频基础生态包（5 个 MIT Clean-Room 纯 CP `@cap/*` 包）
- `packages/@cap/nanoid/` & `cap/test/testCapNanoid.cp`: 加密安全 URL 友好短 ID 生成器（对标 npm `nanoid` [~90M/wk] & Rust `nanoid`，零取模偏差位掩码拒绝采样、OS CSPRNG、预设字母表、确定性测试接口，4/4 PASS）。
- `packages/@cap/ms/` & `cap/test/testCapMs.cp`: 人类可读时长双向解析与格式化（对标 npm `ms` [~250M/wk] & Rust `humantime`，纯 128-bit 定点精确计算、多段复合时长、短/长/humantime 格式，PASS）。
- `packages/@cap/bytes/` & `cap/test/testCapBytes.cp`: 字节大小 SI/IEC 双向解析与格式化（对标 npm `bytes` [~120M/wk] & Rust `bytesize`/`humansize`，纯整数 `u64`/`u128` 精确运算，PASS）。
- `packages/@cap/crc32c/` & `cap/test/testCapCrc32c.cp`: CRC32C Castagnoli 校验和、流式哈希器、O(log N) GF(2) 并行分片合并与 LevelDB/RocksDB/TFRecord 屏蔽算法（对标 Rust `crc32c` & npm `fast-crc32c`，RFC 3720 官方向量 3/3 PASS）。
- `packages/@cap/base58/` & `cap/test/testCapBase58.cp`: Base58 & Base58Check 编解码器（对标 Rust `bs58` & npm `bs58`，支持 Bitcoin/Solana/IPFS/Flickr/Ripple 字母表与双 SHA-256 恒定时间校验和，3/3 PASS）。

## [2026-11-26] - Wave 3B 吸收 npm/Rust 高频基础生态包（`@cap/base32` & `@cap/cookie`）
- `packages/@cap/base32/` & `cap/test/testCapBase32.cp`: 零分配 RFC 4648 Base32 / Base32hex / Crockford / Z-Base-32 编解码器（对标 Rust `base32`/`data-encoding` & npm `hi-base32`，含 RFC 4648 §10 全部官方向量与非零尾比特校验，5/5 PASS）。
- `packages/@cap/cookie/` & `cap/test/testCapCookie.cp`: 零分配 RFC 6265 + CHIPS `Partitioned` Cookie 解析器与防注入 `Set-Cookie` 序列化器（对标 npm `cookie` [~110M/wk] & Rust `cookie`，3/3 PASS）。

## [2026-11-26] - Wave 3C 吸收 npm/Rust 高频基础生态包（`@cap/mime`）
- `packages/@cap/mime/` & `cap/test/testCapMime.cp`: 零分配 IANA Media Types 双向映射库（对标 npm `mime-types` [~130M/wk] & Rust `mime_guess`，覆盖 109 个高频扩展名与 12 个历史别名、`lookupByExt`/`extensionForMime`/`defaultCharset`/`isCompressible`/`contentTypeHeader`，PASS 852ms）。

## [2026-11-26] - 架构分层收敛：基础核心能力归位 `lib/` 内置标准库，`packages/@cap/` 零冗余
- 归位 3 项真正填补内置库缺口的核心模块至 `lib/`：
  - `lib/hash/crc32c.cp`（`cap:hash/crc32c`，同步导出至 `lib/hash/index.cp`，`testCapCrc32c` 3/3 PASS）
  - `lib/time/human.cp`（`cap:time/human`，同步导出至 `lib/time/index.cp`，`testCapMs` PASS）
  - `lib/bytes/`（新建 `cap:bytes` + `doc/lib/bytes.md`，`testCapBytes` PASS）
- 清理与既有 `lib/{crypto,cbor,uuid,codec,mime,http}` 重叠的 11 个轻量 `@cap/*` 重复包，确立清晰双层边界：`lib/*` 承载零依赖核心基础库，`packages/@cap/*` 承载原生 C/C++ 绑定库与大型独立领域引擎包。
- `cp_lib_ref.md`: 登记 `cap:bytes`、`cap:hash/crc32c`、`cap:time/human`。

## [2026-11-26] - Wave 4 重量级独立 `@cap/*` 生态引擎（`@cap/lru`, `@cap/qs`, `@cap/retry`，全部 MIT Clean-Room + CP 性能特性优化）
- `packages/@cap/lru/` & `cap/test/testCapLru.cp`: O(1) 侵入式双向链表 + 512 桶开放寻址 FNV-1a 哈希表 + `slots = undefined` 零初始化栈池 + TTL 过期 + 字节权重淘汰 + 命中率统计（对标 npm `lru-cache` [~150M/wk] & Rust `lru`，7/7 PASS）。
- `packages/@cap/qs/` & `cap/test/testCapQs.cp`: 零堆分配深层嵌套 QueryString 解析与构造引擎（4 KiB `undefined` 栈解码池、方括号/点号嵌套、索引/空括号/逗号数组、严格 null、RFC 3986 percent 编解码、`max_depth`/`max_params` 资源耗尽防护，对标 npm `qs` [~120M/wk] & Rust `serde_qs`，7/7 PASS）。
- `packages/@cap/retry/` & `cap/test/testCapRetry.cp`: 工业级重试与退避引擎（Fixed/Linear/Exponential/Fibonacci + AWS Full/Equal/Decorrelated Jitter + SplitMix64 + 失败分类/`Retry-After` + 总耗时预算 + Envoy 风格 `RetryBudget`，对标 npm `p-retry` & Rust `backoff`/`backon`，4/4 PASS）。

## [2026-11-26] - 落地 `cap from -npm/-crate` 生态自繁衍引擎、`packages/@cap/registry.json` 全局去重索引与 Wave 5（`@cap/diff` & `@cap/template`）
- `GEMINI.md`: 新增第六章《CP 内置库与 `@cap` 生态吸收指南（病毒式生态自我繁衍规范）》，固化双层分层、防重复融合、MIT/Apache-2.0 版权白名单、运行依赖独立化与全局索引规范。
- `packages/@cap/registry.json`: 新建全局去重与映射索引（支持 GitHub Raw URL 访问），预登记 22 条高频 npm/crates.io 映射与别名。
- `cpc/src/migrate/registry.rs`, `cpc/src/migrate/mod.rs`, `cpc/src/main.rs`, `cap/src/main.zig`: 新增 `cap from -npm <pkg[@ver]>` 与 `cap from -crate <name[@ver]>`（别名 `-cargo`），支持查索引去重/提醒、MIT/Apache-2.0 协议守门、运行依赖递归独立转换（跳过 devDependencies）与索引自动回写（4/4 单测 PASS）。
- `packages/@cap/diff/` & `cap/test/testCapDiff.cp`: 新增纯 CP 零堆分配 Myers O(ND) Diff 引擎（FNV-1a 行哈希预过滤、紧凑三角形 `undefined` 栈 Trace 缓冲、GNU Unified Diff 与无损 `applyLineDiff`，对标 npm `diff` [~120M/wk] & Rust `similar`，6/6 PASS）。
- `packages/@cap/template/` & `cap/test/testCapTemplate.cp`: 新增纯 CP 零堆分配模板引擎（默认 HTML XSS 转义、`{{{ raw }}}`、链式过滤器、`#if`/`else`/`#unless`/`#each` + `@index`/`@first`/`@last`/`this` + `MAX_BLOCK_DEPTH=16` 护栏，对标 npm `handlebars`/`mustache` & Rust `minijinja`/`tera`，7/7 PASS）。

## [2026-11-26] - Wave 6 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/validator`, `@cap/table`, `@cap/faker`）并发布至 `capdux.org`
- `packages/@cap/validator/` & `cap/test/testCapValidator.cp`: 零堆分配数据与格式校验引擎（Email/URL/UUID/ULID/IPv4/IPv6/CIDR/SemVer 2.0/ISO-8601 闰年感知日期时间/Luhn 信用卡/E.164 手机号/Base64/Slug + 栈式多错误收集器 `Validator`，对标 npm `zod`/`validator`/`ajv` & Rust `validator`，5/5 PASS）。
- `packages/@cap/table/` & `cap/test/testCapTable.cp`: 纯 CP 终端/Markdown 表格排版引擎（5 套边框预设 Unicode/Rounded/Ascii/Markdown/Compact、左中右对齐、ANSI CSI/OSC 转义跳过与 CJK/Emoji 宽字符宽度感知、自动截断省略号与动态多列构建器，对标 npm `cli-table3`/`table` & Rust `comfy-table`/`tabled`，7/7 PASS）。
- `packages/@cap/faker/` & `cap/test/testCapFaker.cp`: 零堆分配确定性种子（SplitMix64 + xoshiro256**）测试数据与 Mock 生成器（姓名/邮箱/用户名/E.164 手机/IPv4/IPv6/URL/RFC 4122 UUIDv4/颜色/公司/地址/ISO 日期/Luhn 校验信用卡/Fisher-Yates 洗牌与 Floyd 抽样，对标 npm `@faker-js/faker` & Rust `fake`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 32 条高频 npm/crates.io 映射（新增 Wave 6 三大包 + `qrcode`/`ipaddr.js`/`fuse.js`/`change-case`/`semver`/`dotenv`/`cron-parser` 标准库去重映射）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 7 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/decimal`, `@cap/useragent`, `@cap/pluralize`）并发布至 `capdux.org`
- `packages/@cap/decimal/` & `cap/test/testCapDecimal.cp`: 零堆分配 128-bit 精确十进制金融算术引擎（精确加减乘除、除零安全返回、6 种舍入含银行家 `HalfEven`、科学记数法解析与 `toFixed` 栈缓冲格式化，对标 npm `decimal.js`/`bignumber.js` & Rust `rust_decimal`，5/5 PASS）。
- `packages/@cap/useragent/` & `cap/test/testCapUserAgent.cp`: 零堆分配 HTTP `User-Agent` 解析引擎（桌面/移动/平板/CLI/搜索引擎爬虫/AI 爬虫如 `GPTBot`/`ClaudeBot`/`DeepSeekBot`/`PerplexityBot` 全识别，对标 npm `ua-parser-js`/`bowser` & Rust `woothee`，6/6 PASS）。
- `packages/@cap/pluralize/` & `cap/test/testCapPluralize.cp`: 英文单复数与序数词变形引擎（56 组不规则词 + 36 个不可数词 + 大小写还原 + `pluralizeCount` + `ordinal`，对标 npm `pluralize`/`inflection` & Rust `pluralizer`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 38 条高频映射（新增 Wave 7 三大包 + `fast-json-patch`/`robots-parser`/`rss-parser` 标准库去重映射）并同步发布至 `https://capdux.org`（线上官方包总数达 88）。

## [2026-11-26] - Wave 8 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/redact`, `@cap/phone`, `@cap/expr`）并发布至 `capdux.org`
- `packages/@cap/redact/` & `cap/test/testCapRedact.cp`: 零堆分配 PII 与密钥脱敏引擎（单值掩码 + 自由文本自动识别邮箱/Luhn 信用卡/JWT/API Token/手机号/IPv4 + JSON 敏感字段脱敏 + `containsSensitive`，对标 npm `fast-redact`/`maskdata` & Rust `secrecy`，5/5 PASS）。
- `packages/@cap/phone/` & `cap/test/testCapPhone.cp`: 零堆分配国际电话号码解析与格式化引擎（内置 45 国拨号区号与长度规则、支持 `+`/`00`/`tel:`/前导 0 剥离与 E.164/国际/国内/RFC 3966 四种格式输出，对标 npm `libphonenumber-js` & Rust `phonenumber`，6/6 PASS）。
- `packages/@cap/expr/` & `cap/test/testCapExpr.cp`: 零堆分配安全数学与规则表达式求值引擎（完整优先级与短路逻辑、三元运算、右结合幂、64 变量环境、12 安全函数与 32 层深度护栏，对标 npm `expr-eval`/`filtrex` & Rust `evalexpr`/`meval`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 48 条高频映射（新增 Wave 8 三大包 + `otplib`/`ical`/`rrule`/`snowflake-id`/`uri-templates`/`opossum`/`dinero.js` 标准库去重映射）并同步发布至 `https://capdux.org`（线上官方包总数达 91）。

## [2026-11-26] - Wave 9 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/barcode`, `@cap/sqids`, `@cap/cron-desc`）并发布至 `capdux.org`
- `packages/@cap/barcode/` & `cap/test/testCapBarcode.cp`: 零堆分配一维条码编码与 SVG 渲染引擎（EAN-13/UPC-A 含校验位、Code 128B 全 ASCII、Code 39 + SVG/ASCII 输出，对标 npm `jsbarcode` & Rust `barcoders`，5/5 PASS）。
- `packages/@cap/sqids/` & `cap/test/testCapSqids.cp`: 零堆分配 YouTube 风格可逆无脏词短 ID 编码器（62 字符洗牌、52 词屏蔽重试、`min_length` 填充、多数字无损往返，对标 npm `sqids`/`hashids` & Rust `sqids`/`harsh`，6/6 PASS）。
- `packages/@cap/cron-desc/` & `cap/test/testCapCronDesc.cp`: 零堆分配 Cron 表达式自然语言翻译引擎（5 字段 + 7 大宏、英文/中文双语、12h/24h 切换，对标 npm `cronstrue` & Rust `cron-descriptor`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 57 条高频映射（新增 Wave 9 三大包 + `ignore`/`negotiator`/`aho-corasick`/`ksuid`/`@paralleldrive/cuid2`/`roaring` 标准库去重映射）并同步发布至 `https://capdux.org`（线上官方包总数达 94）。

## [2026-11-26] - Wave 10 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/shlex`, `@cap/textwrap`, `@cap/natsort`）并发布至 `capdux.org`
- `packages/@cap/shlex/` & `cap/test/testCapShlex.cp`: 零堆分配 POSIX Shell 安全转义与词法拆分引擎（单/双引号、转义、注释、防注入元字符检测，对标 npm `shell-quote` [45M/wk] & Rust `shlex` [25M/mo]，5/5 PASS）。
- `packages/@cap/textwrap/` & `cap/test/testCapTextwrap.cp`: 零堆分配 ANSI/CJK 感知智能断词换行、缩进、`dedent` 与截断引擎（对标 npm `wrap-ansi` [90M/wk] & Rust `textwrap` [35M/mo]，6/6 PASS）。
- `packages/@cap/natsort/` & `cap/test/testCapNatsort.cp`: 零堆分配任意长度数字自然排序比较与原地排序引擎（对标 npm `natural-compare`/`natsort` [25M/wk] & Rust `natord`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 62 条高频映射（新增 Wave 10 三大包 + `iban`/`isbn3` 标准库映射）并同步发布至 `https://capdux.org`（线上官方包总数达 97）。

## [2026-11-26] - Wave 11 突破 100 官方包里程碑！（`@cap/sqlformat`, `@cap/petname`, `@cap/jsonflat`）并发布至 `capdux.org`
- `packages/@cap/sqlformat/` & `cap/test/testCapSqlformat.cp`: 零堆词法安全 SQL 美化格式化与压缩引擎（保护字符串/转义引号/行块注释、关键词大写、子句缩进与 `minifySql`，对标 npm `sql-formatter` & Rust `sqlformat`，6/6 PASS）。
- `packages/@cap/petname/` & `cap/test/testCapPetname.cp`: 零堆确定性 SplitMix64 可读名称与 DNS 安全子域名生成器（赋能 `caprun` 沙箱命名，对标 npm `unique-names-generator`/`human-id` & Rust `petname`，4/4 PASS）。
- `packages/@cap/jsonflat/` & `cap/test/testCapJsonflat.cp`: 零堆递归 JSON 扁平化与 `dot-prop` 路径直取引擎（对标 npm `flat`/`dot-prop`，4/4 PASS）。
- `packages/@cap/registry.json`: 扩充至 65 条高频映射并同步发布至 `https://capdux.org`（线上官方 `.cap` 包总数正式突破 **100 个**里程碑！）。

## [2026-11-26] - Wave 12 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/http-status`, `@cap/wcag`, `@cap/didyoumean`）并发布至 `capdux.org`
- `packages/@cap/http-status/` & `cap/test/testCapHttpStatus.cp`: 69 个 IANA/RFC 9110/Cloudflare HTTP 状态码双向查询与语义判断（`isRetryable`/`isEmptyBody`/`isCacheableByDefault`，对标 npm `statuses` [80M/wk] & `http-status-codes` [30M/wk]，6/6 PASS）。
- `packages/@cap/wcag/` & `cap/test/testCapWcag.cp`: W3C WCAG 2.1/2.2 色彩无障碍对比度计算、AA/AAA 评级与自动高对比文字色推荐（对标 npm `wcag-contrast` [8M/wk] & Rust `contrast`，6/6 PASS）。
- `packages/@cap/didyoumean/` & `cap/test/testCapDidyoumean.cp`: 零堆分配 Levenshtein / Damerau-Levenshtein 换位编辑距离与 CLI/API 拼写建议引擎（对标 npm `didyoumean` [15M/wk] & `fastest-levenshtein` [30M/wk]，4/4 PASS）。
- `packages/@cap/registry.json`: 扩充至 68 条高频映射并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 13 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/human-number`, `@cap/envsubst`, `@cap/link-header`）并发布至 `capdux.org`
- `packages/@cap/human-number/` & `cap/test/testCapHumanNumber.cp`: 零堆分配人类可读数字格式化引擎（千分位、中英文紧凑缩写、百分比、规范罗马数字双向转换与英文拼写，对标 npm `numeral`/`millify`/`number-to-words` & Rust `num-format`，6/6 PASS）。
- `packages/@cap/envsubst/` & `cap/test/testCapEnvsubst.cp`: 零堆 POSIX 环境变量模板展开引擎（`$VAR`/`${VAR:-def}`/`:+alt`/`:?err`/`$$` 转义，对标 npm `dotenv-expand` [35M/wk] & Rust `shellexpand` [15M/mo]，6/6 PASS）。
- `packages/@cap/link-header/` & `cap/test/testCapLinkHeader.cp`: 零堆 RFC 8288 HTTP `Link` 分页头解析与构建器（对标 npm `parse-link-header` [10M/wk] & Rust `parse_link_header`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 73 条高频映射（新增 Wave 13 三大包 + `cache-control-parser`/`content-disposition` 标准库映射）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 14 突破 110 官方包！（`@cap/forwarded`, `@cap/path-to-regexp`, `@cap/color-names`）并发布至 `capdux.org`
- `packages/@cap/forwarded/` & `cap/test/testCapForwarded.cp`: 零堆 RFC 7239 `Forwarded` 与 `X-Forwarded-For` 代理链解析与防伪造 `trusted_hops` 真实 IP 提取（对标 npm `forwarded` [45M/wk] & `proxy-addr` [50M/wk]，6/6 PASS）。
- `packages/@cap/path-to-regexp/` & `cap/test/testCapPathToRegexp.cp`: 零堆 URL 路由模式匹配、命名/可选/通配参数提取与反向 `buildRoute` URL 构建器（对标 npm `path-to-regexp` [65M/wk]，4/4 PASS）。
- `packages/@cap/color-names/` & `cap/test/testCapColorNames.cp`: 全部 148 个 W3C CSS Color 4 命名颜色二分查表 + RGB/HSL/HSV/HEX 互转与调色（对标 npm `color-name`/`color-convert` [95M/wk]，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 76 条高频映射并同步发布至 `https://capdux.org`（线上官方 `.cap` 包突破 **110 个**！）。

## [2026-11-26] - Wave 15 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/json-repair`, `@cap/html-entities`, `@cap/data-uri`）并发布至 `capdux.org`
- `packages/@cap/json-repair/` & `cap/test/testCapJsonRepair.cp`: 零堆 LLM/坏 JSON 自动修复引擎（对标 npm `jsonrepair` & Rust `llm_json`，5/5 PASS）。
- `packages/@cap/html-entities/` & `cap/test/testCapHtmlEntities.cp`: 零堆 HTML5 实体编解码与 XSS 转义引擎（对标 npm `he` [55M/wk] & `escape-html` [70M/wk]，6/6 PASS）。
- `packages/@cap/data-uri/` & `cap/test/testCapDataUri.cp`: 零堆 RFC 2397 `data:` URI 解析、Base64/百分号解码与编码引擎（对标 npm `data-uri-to-buffer` [15M/wk] & Rust `data-url`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 80 条高频映射（新增 Wave 15 三大包 + `range-parser` 标准库映射）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 16 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/content-type`, `@cap/markdown-toc`, `@cap/short-uuid`）并发布至 `capdux.org`
- `packages/@cap/content-type/` & `cap/test/testCapContentType.cp`: RFC 9110/6838 HTTP `Content-Type` 解析与规范化格式化（对标 npm `content-type` [70M/wk] & `media-typer` [55M/wk]，6/6 PASS）。
- `packages/@cap/markdown-toc/` & `cap/test/testCapMarkdownToc.cp`: Markdown 标题大纲提取、GitHub 锚点去重与嵌套 TOC 渲染（对标 npm `github-slugger` [35M/wk] & `markdown-toc`，4/4 PASS）。
- `packages/@cap/short-uuid/` & `cap/test/testCapShortUuid.cp`: 128-bit UUID 无损双向短 ID 编解码（Flickr Base58 / Base62 / Cookie Base90，对标 npm `short-uuid` [5M/wk]，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 85 条高频映射（新增 Wave 16 三大包 + `httpdate`/`escape-string-regexp` 标准库映射）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 17 三路并行吸收重量级独立 `@cap/*` 生态引擎（`@cap/ascii-tree`, `@cap/url-join`, `@cap/text-stats`）并发布至 `capdux.org`
- `packages/@cap/ascii-tree/` & `cap/test/testCapAsciiTree.cp`: 零堆 Unicode/Rounded/ASCII 树形结构渲染器（对标 npm `archy` [35M/wk] & Rust `ptree`，4/4 PASS）。
- `packages/@cap/url-join/` & `cap/test/testCapUrlJoin.cp`: 零堆 RFC 3986 URL 路径/查询/锚点智能拼接引擎（对标 npm `url-join` [15M/wk]，6/6 PASS）。
- `packages/@cap/text-stats/` & `cap/test/testCapTextStats.cp`: 零堆单遍 UTF-8 中英文本统计、阅读时长与 LLM Token 估算引擎（对标 npm `reading-time` & `word-count`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 92 条高频映射（新增 Wave 17 三大包 + `@langchain/textsplitters`/`helmet`/`soundex-code`/`macaddress` 标准库去重映射）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 18 按下载量吸收 10 项高频资产（`@cap/timeago`, `@cap/markdown-strip`, `@cap/sparkline` + 7 项头部 npm/crates 映射）并突破 120 官方包与 100 映射里程碑
- `packages/@cap/timeago/` & `cap/test/testCapTimeago.cp`: 零堆确定性中英相对时间格式化引擎（对标 npm `timeago.js` / Rust `chrono-humanize`，6/6 PASS）。
- `packages/@cap/markdown-strip/` & `cap/test/testCapMarkdownStrip.cp`: 零堆 Markdown 转纯文本与 UTF-8 安全摘要引擎（对标 npm `remove-markdown` [6M/wk]，6/6 PASS）。
- `packages/@cap/sparkline/` & `cap/test/testCapSparkline.cp`: 零堆 Unicode 迷你折线图 `▁▂▃▄▅▆▇█`、1/8 精度水平柱状图与分桶降采样引擎（对标 npm `sparkly`/`asciichart`，6/6 PASS）。
- `packages/@cap/registry.json`: 突破 **102 条**高频映射（新增 `ms` [190M/wk]、`strip-ansi` [180M/wk]、`json5` [90M/wk]、`bytes` [85M/wk]、`etag` [45M/wk]、`gray-matter` [15M/wk]、`slugify` [10M/wk] 直连内部 `cap:*` 标准库映射 + Wave 18 三大独立包）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 19 四路并行吸收 10 项高下载资产（`@cap/spdx`, `@cap/boxen`, `@cap/conventional-commits`, `@cap/svg-badge` + 6 项头部去重映射）并发布至 `capdux.org`
- `packages/@cap/spdx/` & `cap/test/testCapSpdx.cp`: 43 项 SPDX 元数据 + 递归下降表达式解析与白名单/宽松协议求值（对标 npm `spdx-expression-parse` [55M/wk] & Rust `spdx`，7/7 PASS）。
- `packages/@cap/boxen/` & `cap/test/testCapBoxen.cp`: 6 种终端边框、标题嵌入、CJK 宽字符与 ANSI 穿透零堆渲染器（对标 npm `boxen` [25M/wk]，6/6 PASS）。
- `packages/@cap/conventional-commits/` & `cap/test/testCapConventionalCommits.cp`: Conventional Commits 1.0.0 解析与 Semver 推导引擎（对标 npm `conventional-commits-parser` [18M/wk]，6/6 PASS）。
- `packages/@cap/svg-badge/` & `cap/test/testCapSvgBadge.cp`: Shields.io 风格 Flat/FlatSquare/Plastic SVG 状态徽章零堆生成器（对标 npm `badge-maker` [3M/wk]，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **112 条**高频映射（新增 `toml`/`ini`/`cookie`/`punycode`/`human-signals`/`editorconfig` 内部库直连映射 + Wave 19 四大独立包）并同步发布至 `https://capdux.org`。

## [2026-11-26] - Wave 20 四路并行吸收 10 项高下载资产（`@cap/git-url`, `@cap/stacktrace`, `@cap/oci-ref`, `@cap/card-bin` + 6 项超高下载去重映射）并突破 129 官方包
- `packages/@cap/git-url/` & `cap/test/testCapGitUrl.cp`: SCP/HTTPS/SSH/托管简写 Git 仓库地址零拷贝解析与规范化（对标 npm `hosted-git-info` [65M/wk] & `git-url-parse` [15M/wk]，6/6 PASS）。
- `packages/@cap/stacktrace/` & `cap/test/testCapStacktrace.cp`: V8/SpiderMonkey/Rust/Zig/CP 多格式调用栈解析与内部帧清洗（对标 npm `error-stack-parser` [40M/wk] & `stacktrace-parser` [35M/wk]，6/6 PASS）。
- `packages/@cap/oci-ref/` & `cap/test/testCapOciRef.cp`: OCI/Docker 容器镜像引用解析、校验与规范化（对标 Rust `oci-distribution` / `docker-reference`，6/6 PASS）。
- `packages/@cap/card-bin/` & `cap/test/testCapCardBin.cp`: 8 大支付卡组织 BIN 识别、Luhn 校验、分组格式化与 PCI-DSS 脱敏（对标 npm `credit-card-type` [4M/wk] & `card-validator`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **122 条**高频映射（新增 `chalk`/`picocolors` [260M/wk]、`@jridgewell/sourcemap-codec`/`vlq` [120M/wk]、`nanoid` [65M/wk]、`shell-quote` [45M/wk]、`ulid` [15M/wk]、`auth-header` [15M/wk] 去重映射 + Wave 20 四大独立包）。

## [2026-11-26] - Wave 21 四路并行吸收 10 项高下载资产（`@cap/jcs`, `@cap/cli-spinners`, `@cap/strftime`, `@cap/calver` + 6 项超高下载去重映射）并突破 133 官方包
- `packages/@cap/jcs/` & `cap/test/testCapJcs.cp`: RFC 8785 确定性规范 JSON 序列化与等价比较引擎（对标 npm `fast-json-stable-stringify` [55M/wk]，6/6 PASS）。
- `packages/@cap/cli-spinners/` & `cap/test/testCapCliSpinners.cp`: 12 套终端 Spinner 预设 + `log-symbols` Unicode/ASCII 状态符号引擎（对标 npm `cli-spinners`/`ora` [35M/wk] & `log-symbols` [45M/wk]，6/6 PASS）。
- `packages/@cap/strftime/` & `cap/test/testCapStrftime.cp`: 零堆 POSIX `strftime` 与 ISO-8601 日期格式化引擎（对标 npm `strftime`/`dateformat` [20M/wk]，6/6 PASS）。
- `packages/@cap/calver/` & `cap/test/testCapCalver.cp`: Calendar Versioning 解析、比较与周期递增引擎（对标 npm/Rust `calver`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **132 条**高频映射（新增 `mime-types`/`ci-info`/`protobufjs`/`@msgpack/msgpack`/`base64url`/`cbor-x` 内部库直连映射 + Wave 21 四大独立包）。

## [2026-11-26] - Wave 22 四路并行吸收 10 项高下载资产（`@cap/svg-path`, `@cap/mail-addr`, `@cap/keep-changelog`, `@cap/subtitle` + 6 项去重映射）并突破 138 官方包
- `packages/@cap/svg-path/` & `cap/test/testCapSvgPath.cp`: SVG 全 20 命令零堆解析、2D 包围盒计算与规范回写（对标 npm `parse-svg-path` [15M/wk] & `svg-path-parser` [12M/wk]，6/6 PASS）。
- `packages/@cap/mail-addr/` & `cap/test/testCapMailAddr.cp`: RFC 5322 邮件地址与多收件人列表零拷贝解析与格式化（对标 npm `addressparser` [10M/wk]，6/6 PASS）。
- `packages/@cap/keep-changelog/` & `cap/test/testCapKeepChangelog.cp`: Keep a Changelog 1.1.0 解析、版本检索与分类计数（对标 npm `keep-a-changelog` [3M/wk]，6/6 PASS）。
- `packages/@cap/subtitle/` & `cap/test/testCapSubtitle.cp`: SRT & WebVTT 字幕解析、时间轴偏移与双向转换（对标 npm `subtitle` [2M/wk]，v1.0.1 6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **142 条**高频映射（新增 `ndjson`/`netrc-parser`/`iso8601-duration`/`codeowners`/`dockerfile-ast`/`logfmt` 内部库直连映射 + Wave 22 四大独立包）。

## [2026-11-26] - Wave 23 交付 `@cap/ssri`, `@cap/pem`, `@cap/bcp47`, `@cap/hosts-file` 并建立 npm/crates.io 真实下载总榜排序引擎
- `packages/@cap/ssri/` & `cap/test/testCapSsri.cp`: W3C SRI 多算法完整性校验串解析与恒定时间匹配（6/6 PASS）。
- `packages/@cap/pem/` & `cap/test/testCapPem.cp`: RFC 7468 PEM 多块解析与 64 列编码器（6/6 PASS）。
- `packages/@cap/bcp47/` & `cap/test/testCapBcp47.cp`: RFC 5646 / BCP-47 语言标签解析、规范化与回退匹配（6/6 PASS）。
- `packages/@cap/hosts-file/` & `cap/test/testCapHostsFile.cp`: `/etc/hosts` 多别名解析、IPv4/IPv6 正反向查表与格式化（6/6 PASS）。
- 建立 `temp/global_top_rank.json` 实时抓取 `crates.io` Top 200 总下载榜与 `api.npmjs.org` 真实周下载榜，后续批次严格按真实总下载量单调递减推进。

## [2026-11-26] - Batch 24 严格按 npm 真实周下载量 Top 10（12.17 亿 → 5.52 亿/周）吸收 `@cap/debug`, `@cap/wrap-ansi` + 7 项亿级去重映射
- `packages/@cap/debug/` & `cap/test/testCapDebug.cp`: 零堆 `DEBUG` 命名空间通配符过滤（排除优先）、确定性配色与 `+ms` 耗时差引擎（对标 npm `debug` **968.8M/wk**，6/6 PASS）。
- `packages/@cap/wrap-ansi/` & `cap/test/testCapWrapAnsi.cp`: 零堆 ANSI 颜色栈跨行续接折行与按列切片引擎（对标 npm `wrap-ansi` **614.8M/wk** & `slice-ansi`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **155 条**高频映射（新增 `minimatch` [10.07亿/wk]、`ansi-styles` [9.42亿/wk]、`lru-cache` [8.21亿/wk]、`commander` [7.82亿/wk]、`string-width` [7.34亿/wk]、`picomatch` [7.33亿/wk]、`glob` [5.52亿/wk] + `@cap/debug` + `@cap/wrap-ansi`）。

## [2026-11-26] - Batch 25 严格单调递减吸收 npm 5.42 亿 → 3.99 亿/周 Top 10（`@cap/json-schema`, `@cap/css-units` + 8 项亿级去重映射）
- `packages/@cap/json-schema/` & `cap/test/testCapJsonSchema.cp`: 零堆 JSON Schema Draft-07/2020-12 校验引擎（对标 npm `ajv` **542.0M/wk** & `zod` **467.6M/wk**，6/6 PASS）。
- `packages/@cap/css-units/` & `cap/test/testCapCssUnits.cp`: 26 种 CSS 维度单位解析与换算引擎（对标 npm `postcss-value-parser`，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **165 条**高频映射（新增 `ajv`/`source-map`/`which`/`p-limit`/`postcss`/`postcss-value-parser`/`find-up`/`ws`/`js-yaml`/`uuid`）。

## [2026-11-26] - Batch 26 严格单调递减吸收 npm 3.61 亿 → 2.42 亿/周 Top 10（`@cap/http-errors` + 9 项亿级去重映射）
- `packages/@cap/http-errors/` & `cap/test/testCapHttpErrors.cp`: 16 个标准 4xx/5xx 快捷构造器、可重试判定与 RFC 9457 `problem+json` 序列化（对标 npm `http-errors` **253.4M/wk**，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **175 条**高频映射（新增 `cross-spawn`/`undici`/`acorn`/`resolve`/`fs-extra`/`node-fetch`/`chokidar`/`http-errors`/`parse5`/`encodeurl`）。

## [2026-11-26] - Batch 27 严格单调递减吸收 Rust crates.io Top 10（32.61 亿 → 17.87 亿总下载量）并建立 Rust 优先安全吸收线
- `packages/@cap/registry.json`: 扩充至 **185 条**高频映射（按 crates.io 真实总下载量与被引用包数严格递减纳入 `hashbrown`/`getrandom`/`bitflags`/`rand`/`base64`/`indexmap`/`itertools`/`memchr`/`unicode-ident`/`serde`，全部精准去重直连 CP 内部标准库）。

## [2026-11-26] - Batch 28 严格单调递减吸收 Rust crates.io 17.28 亿 → 11.92 亿 Top 10（`@cap/strsim`, `@cap/slab` + 8 项十亿级去重映射）
- `packages/@cap/strsim/` & `cap/test/testCapStrsim.cp`: 7 合 1 零堆字符串距离与相似度算法（对标 Rust `strsim` **13.25 亿总下载**，6/6 PASS）。
- `packages/@cap/slab/` & `cap/test/testCapSlab.cp`: O(1) 预分配对象池与防 ABA 代数句柄引擎（对标 Rust `slab` **11.92 亿总下载**，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **195 条**高频映射（新增 `itoa`/`heck`/`log`/`regex`/`clap`/`bytes`/`strsim`/`http`/`sha2`/`slab`）。

## [2026-11-26] - Batch 29 严格单调递减吸收 Rust crates.io 11.79 亿 → 9.93 亿 Top 10（`@cap/fastrand`, `@cap/percent-encoding`, `@cap/byteorder`）并突破 205 条生态映射里程碑
- `packages/@cap/fastrand/` & `cap/test/testCapFastrand.cp`: WyRand 64-bit 零堆伪随机数与无偏采样引擎（对标 Rust `fastrand` **11.79 亿总下载**，6/6 PASS）。
- `packages/@cap/percent-encoding/` & `cap/test/testCapPercentEncoding.cp`: WHATWG/RFC 3986 128-bit `AsciiSet` 百分号编解码引擎（对标 Rust `percent-encoding` **11.58 亿总下载**，6/6 PASS）。
- `packages/@cap/byteorder/` & `cap/test/testCapByteorder.cp`: 大小端二进制编解码与零越界单向 `ByteReader`/`ByteWriter`（对标 Rust `untrusted` **10.18 亿** + `byteorder` **9.93 亿**，6/6 PASS）。
- `packages/@cap/registry.json`: 正式突破 **205 条**生态映射里程碑（新增 `fastrand`/`miniz_oxide`/`percent-encoding`/`time`/`url`/`errno`/`tempfile`/`chrono`/`untrusted`/`byteorder`）。

## [2026-11-26] - Batch 30 严格单调递减吸收 Rust crates.io 10.77 亿 → 8.12 亿 Top 10（一口气交付 6 大 Rust 独立 `@cap` 包，总映射突破 215 条）
- `packages/@cap/fnv/` & `cap/test/testCapFnv.cp`: FNV-1a 32/64 + FxHash 64（对标 `rustc-hash` 10.77 亿 + `fnv` 10.07 亿，6/6 PASS）。
- `packages/@cap/subtle/` & `cap/test/testCapSubtle.cp`: 恒定时间防侧信道原语 + 安全内存清零（对标 `zeroize` 9.35 亿 + `subtle` 9.21 亿，6/6 PASS）。
- `packages/@cap/crc32/` & `cap/test/testCapCrc32.cp`: IEEE CRC-32 / CRC-32C / Adler-32 + 增量合并（对标 `crc32fast` 9.18 亿，6/6 PASS）。
- `packages/@cap/num-integer/` & `cap/test/testCapNumInteger.cp`: 精确整数除余/数论/GCD/Bézout/模逆元/整数根（对标 `num-integer` 8.88 亿，6/6 PASS）。
- `packages/@cap/hex/` & `cap/test/testCapHex.cp`: 零堆十六进制编解码与 HexDump（对标 `hex` 8.43 亿，6/6 PASS）。
- `packages/@cap/tinystr/` & `cap/test/testCapTinystr.cp`: `TinyStr4`/`TinyStr8` 寄存器打包短字符串（对标 `tinystr` 8.35 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **215 条**高频映射（含 `iana-time-zone`→`cap:tz`、`walkdir`→`cap:fs`）。

## [2026-11-26] - Batch 31 严格单调递减吸收 Rust crates.io 8.24 亿 → 6.90 亿 Top 10（单批交付 5 大 Rust 核心包，总映射突破 225 条）
- `packages/@cap/litemap/` & `cap/test/testCapLitemap.cp`: 零堆紧凑有序二分查找映射表（对标 `litemap` 8.24 亿，6/6 PASS）。
- `packages/@cap/siphash/` & `cap/test/testCapSiphash.cp`: 防 HashDoS 带密钥 SipHash-2-4/1-3 流式哈希（对标 `siphasher` 7.36 亿，6/6 PASS）。
- `packages/@cap/arrayvec/` & `cap/test/testCapArrayvec.cp`: 零堆栈上定容字符串与定容向量（对标 `arrayvec` 7.30 亿，6/6 PASS）。
- `packages/@cap/unicode-seg/` & `cap/test/testCapUnicodeSeg.cp`: UAX #29 字素簇与词边界切分（对标 `unicode-segmentation` 7.18 亿，6/6 PASS）。
- `packages/@cap/oid/` & `cap/test/testCapOid.cp`: ASN.1 OID X.690 DER 编解码与 PKI 查表（对标 `const-oid` 6.90 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **225 条**高频映射（含 `hmac`/`mime`/`num-bigint`/`ipnet`/`glob` 内部库直连）。

## [2026-11-26] - Batch 32 严格单调递减吸收 Rust crates.io 6.41 亿 → 5.32 亿 Top 10（单批交付 5 大 Rust 核心包，总映射突破 235 条）
- `packages/@cap/bitset/` & `cap/test/testCapBitset.cp`: `BitSet256`/`BitSet1024` 位集合代数与高速扫描（对标 `fixedbitset`/`bit-vec`/`bit-set`，6/6 PASS）。
- `packages/@cap/half/` & `cap/test/testCapHalf.cp`: IEEE 754 `f16` 与 `bf16` 比特精确编解码（对标 `half` 5.99 亿，6/6 PASS）。
- `packages/@cap/matchit/` & `cap/test/testCapMatchit.cp`: `axum` 风格零拷贝高优先级 URL 路由引擎（对标 `matchit` 5.95 亿，6/6 PASS）。
- `packages/@cap/ordered-float/` & `cap/test/testCapOrderedFloat.cp`: IEEE 754 全序浮点、`NotNanF64` 与安全排序（对标 `ordered-float` 5.71 亿，6/6 PASS）。
- `packages/@cap/data-encoding/` & `cap/test/testCapDataEncoding.cp`: RFC 4648 Base32/Base32hex/Crockford/Base64url 编解码（对标 `data-encoding` 5.58 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **235 条**高频映射（含 `petgraph`/`rustls-pemfile`/`quick-xml`/`simd-adler32`/`bstr` 去重直连）。

## [2026-11-26] - Batch 33 严格单调递减吸收 Rust crates.io 5.16 亿 → 4.67 亿 Top 10（交付 4 大 Rust 核心包，总映射突破 245 条）
- `packages/@cap/unicase/` & `cap/test/testCapUnicase.cp`: 零堆大小写不敏感包装、查找与 FNV-1a 折叠哈希（对标 `unicase` 5.16 亿，6/6 PASS）。
- `packages/@cap/num-complex/` & `cap/test/testCapNumComplex.cp`: `Complex64` 复数代数与欧拉复变函数（对标 `num-complex` 4.96 亿，6/6 PASS）。
- `packages/@cap/num-rational/` & `cap/test/testCapNumRational.cp`: `Ratio64` 精确自动约分有理数引擎（对标 `num-rational` 4.90 亿，6/6 PASS）。
- `packages/@cap/target-triple/` & `cap/test/testCapTargetTriple.cp`: LLVM/Rust/Zig 目标三元组解析器（对标 `target-lexicon` 4.79 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **245 条**高频映射（含 `humantime`/`zstd`/`which`/`md-5`/`textwrap`/`lru` 去重直连）。

## [2026-11-26] - Batch 34 吸收 10 项 Rust 生态核心资产（交付 6 大独立 `@cap` 包，总映射突破 **255 条**里程碑！）
- `packages/@cap/der/` & `cap/test/testCapDer.cp`: ITU-T X.690 ASN.1 DER 严格 TLV 编解码（对标 `der` 6.13 亿，6/6 PASS）。
- `packages/@cap/bincode/` & `cap/test/testCapBincode.cp`: bincode v2 紧凑二进制 Varint/ZigZag 序列化（对标 `bincode` 3.91 亿，6/6 PASS）。
- `packages/@cap/indoc/` & `cap/test/testCapIndoc.cp`: 多行文本去缩进与 `stripMargin` 引擎（对标 `indoc` 3.86 亿，6/6 PASS）。
- `packages/@cap/ringbuf/` & `cap/test/testCapRingbuf.cp`: 2 的幂位掩码双端环形队列与流式 `ByteRingBuf`（6/6 PASS）。
- `packages/@cap/bloom/` & `cap/test/testCapBloom.cp`: Kirsch-Mitzenmacher 双哈希零堆布隆过滤器（6/6 PASS）。
- `packages/@cap/toml-writer/` & `cap/test/testCapTomlWriter.cp`: 零堆 TOML 1.0 序列化格式化器（对标 `toml_writer` 3.21 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 正式突破 **255 条**高频生态映射（含 `pem-rfc7468`/`tungstenite`/`chacha20`/`constant_time_eq` 去重直连）。

## [2026-11-26] - Batch 35 严格单调递减吸收 Rust crates.io 3.81 亿 → 2.95 亿 Top 10（交付 4 大 Rust 核心包，总映射突破 265 条）
- `packages/@cap/multimap/` & `cap/test/testCapMultimap.cp`: 零堆一键多值保序映射表（对标 `multimap` 3.81 亿，6/6 PASS）。
- `packages/@cap/utf8-path/` & `cap/test/testCapUtf8Path.cp`: UTF-8 路径规范化与防目录穿越 `safeJoin`（对标 `camino` 3.41 亿，6/6 PASS）。
- `packages/@cap/utf8-valid/` & `cap/test/testCapUtf8Valid.cp`: RFC 3629 严格 UTF-8 验证与 `U+FFFD` 有损修复（对标 `simdutf8` 3.30 亿，6/6 PASS）。
- `packages/@cap/pcg/` & `cap/test/testCapPcg.cp`: PCG32 独立流 PRNG 与 O(log N) 跳跃（对标 `rand_pcg` 2.95 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **265 条**高频映射（含 `mime_guess`/`zip`/`brotli`/`twox-hash`/`tar`/`csv` 去重直连）。
