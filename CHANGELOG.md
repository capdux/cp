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

## [2026-11-26] - Batch 36 单批交付 6 大 Rust 独立 `@cap` 包（`@cap/bs58`, `@cap/varint`, `@cap/online-stats`, `@cap/base85`, `@cap/governor`, `@cap/backoff`），总映射突破 275 条
- `packages/@cap/bs58/` & `cap/test/testCapBs58.cp`: Bitcoin/Ripple/Flickr Base58 零堆编解码（6/6 PASS）。
- `packages/@cap/varint/` & `cap/test/testCapVarint.cp`: ULEB128/SLEB128/ZigZag/QUIC RFC 9000 变长整数（6/6 PASS）。
- `packages/@cap/online-stats/` & `cap/test/testCapOnlineStats.cp`: Welford 流式方差 + Chan 并行合并 + 线性回归 + 分位数（6/6 PASS）。
- `packages/@cap/base85/` & `cap/test/testCapBase85.cp`: ZeroMQ Z85 RFC 32 与 Adobe Ascii85 编解码（6/6 PASS）。
- `packages/@cap/governor/` & `cap/test/testCapGovernor.cp`: GCRA 与令牌桶确定性限流器（6/6 PASS）。
- `packages/@cap/backoff/` & `cap/test/testCapBackoff.cp`: 指数/线性退避与 AWS 抖动算法（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **275 条**高频映射（含 `yansi`/`tinytemplate`/`pbkdf2`/`jiff` 去重直连）。

## [2026-11-26] - Batch 37 历史里程碑：突破 **200 官方 `.cap` 包 + 285 条映射**！（交付 `@cap/murmur3`, `@cap/consistent-hash`, `@cap/hyperloglog`, `@cap/count-min`）
- `packages/@cap/murmur3/` & `cap/test/testCapMurmur3.cp`: MurmurHash3 32-bit x86 & 128-bit x64 比特精确实现（6/6 PASS）。
- `packages/@cap/consistent-hash/` & `cap/test/testCapConsistentHash.cp`: 虚拟节点一致性哈希环与去重多副本 `getReplicas`（6/6 PASS）。
- `packages/@cap/hyperloglog/` & `cap/test/testCapHyperloglog.cp`: 256 字节零堆 HyperLogLog 基数估算与分布式合并（6/6 PASS）。
- `packages/@cap/count-min/` & `cap/test/testCapCountMin.cp`: Count-Min Sketch 流式频次估算与热点检测（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **285 条**高频映射（含 `ciborium`/`wasmparser`/`signal-hook`/`cookie`/`tiny-keccak`/`png` 去重直连）。

## [2026-11-26] - Batch 38 交付 4 大 Rust 高速哈希与无损编码包（`@cap/foldhash`, `@cap/bech32`, `@cap/cobs`, `@cap/rle`），总映射突破 **295 条**！
- `packages/@cap/foldhash/` & `cap/test/testCapFoldhash.cp`: 对标 Rust `foldhash`（8.95 亿下载），纯 64 位无溢出 128-bit 折叠乘加哈希（6/6 PASS）。
- `packages/@cap/bech32/` & `cap/test/testCapBech32.cp`: BIP-173 Bech32 & BIP-350 Bech32m 6 字符 BCH 校验编解码（6/6 PASS）。
- `packages/@cap/cobs/` & `cap/test/testCapCobs.cp`: Consistent Overhead Byte Stuffing 帧同步编码与流式 `CobsDecoder`（6/6 PASS）。
- `packages/@cap/rle/` & `cap/test/testCapRle.cp`: Apple/TIFF PackBits RLE + Delta / Delta-of-Delta 时序压缩（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **295 条**高频映射（含 `once_cell`/`socket2`/`parking_lot`/`tokio`/`lazy_static`/`scopeguard` 6 大十亿级 Rust crate 去重直连）。

## [2026-11-26] - Batch 39 里程碑：总生态映射突破 **305 条** + 线上官方包突破 **210 个**！（交付 `@cap/ahash`, `@cap/merkle`, `@cap/toposort`, `@cap/disjoint-set`, `@cap/fenwick`, `@cap/range-set`）
- `packages/@cap/ahash/` & `cap/test/testCapAhash.cp`: 对标 Rust `ahash`（9.46 亿下载），4-key `RandomState` 高速非加密哈希（6/6 PASS）。
- `packages/@cap/merkle/` & `cap/test/testCapMerkle.cp`: RFC 6962 域分离 SHA-256 默克尔树与审计证明（6/6 PASS）。
- `packages/@cap/toposort/` & `cap/test/testCapToposort.cp`: Kahn 最小字典序拓扑排序、环路检测与并行层级（6/6 PASS）。
- `packages/@cap/disjoint-set/` & `cap/test/testCapDisjointSet.cp`: 路径减半 + 按大小合并零堆并查集（6/6 PASS）。
- `packages/@cap/fenwick/` & `cap/test/testCapFenwick.cp`: O(log N) 树状数组前缀/区间和与 `lowerBound`（6/6 PASS）。
- `packages/@cap/range-set/` & `cap/test/testCapRangeSet.cp`: 零堆有序半开区间自动合并与劈裂集合（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **305 条**高频映射（含 `zerocopy`/`crossbeam-utils`/`http-body`/`futures` 去重直连）。

## [2026-11-26] - Batch 40 单批交付 6 大压缩、索引与缓存算法包（`@cap/lz4-block`, `@cap/roaring`, `@cap/tdigest`, `@cap/trie`, `@cap/huffman`, `@cap/sieve-cache`），总映射突破 **315 条**！
- `packages/@cap/lz4-block/` & `cap/test/testCapLz4Block.cp`: 标准 LZ4 Raw Block 极速块压缩与解压（6/6 PASS）。
- `packages/@cap/roaring/` & `cap/test/testCapRoaring.cp`: 高 16 位分桶 Roaring Bitmap 压缩位图与集合代数（6/6 PASS）。
- `packages/@cap/tdigest/` & `cap/test/testCapTdigest.cp`: Merging T-Digest 流式尾部延迟分位数估算与分布式合并（6/6 PASS）。
- `packages/@cap/trie/` & `cap/test/testCapTrie.cp`: 零堆 LCRS 静态池字节前缀树与 LPM（6/6 PASS）。
- `packages/@cap/huffman/` & `cap/test/testCapHuffman.cp`: RFC 1951 Canonical Huffman 规范霍夫曼编解码（6/6 PASS）。
- `packages/@cap/sieve-cache/` & `cap/test/testCapSieveCache.cp`: NSDI'24 SIEVE 缓存淘汰引擎（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **315 条**高频映射（含 `either`/`toml_edit`/`thread_local`/`colorchoice` 去重直连）。

## [2026-11-26] - Batch 41 单批交付 6 大地理空间、压缩与有序结构包（`@cap/snappy-block`, `@cap/bimap`, `@cap/geohash`, `@cap/geo-dist`, `@cap/morton`, `@cap/skiplist`），总映射突破 **325 条**！
- `packages/@cap/snappy-block/` & `cap/test/testCapSnappyBlock.cp`: Google Snappy Raw Block 极速无损压缩与解压（6/6 PASS）。
- `packages/@cap/bimap/` & `cap/test/testCapBimap.cp`: 零堆严格双向 1-to-1 键值映射表（6/6 PASS）。
- `packages/@cap/geohash/` & `cap/test/testCapGeohash.cp`: Base32 Geohash 编解码、包围盒与 8 邻域（6/6 PASS）。
- `packages/@cap/geo-dist/` & `cap/test/testCapGeoDist.cp`: WGS-84 Haversine/航向角/目标点/中点/多边形包含（6/6 PASS）。
- `packages/@cap/morton/` & `cap/test/testCapMorton.cp`: 2D/3D Morton 码与 2D Hilbert 空间曲线（6/6 PASS）。
- `packages/@cap/skiplist/` & `cap/test/testCapSkiplist.cp`: 零堆静态节点池确定性跳表（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **325 条**高频映射（含 `typenum`/`generic-array`/`equivalent`/`memoffset` 去重直连）。

## [2026-11-26] - Batch 42 交付 5 大校验和、定点数与概率/堆结构包（`@cap/adler32`, `@cap/crc64`, `@cap/fixed-point`, `@cap/priority-queue`, `@cap/cuckoo-filter`），总映射突破 **335 条**！
- `packages/@cap/adler32/` & `cap/test/testCapAdler32.cp`: 对标 Rust `adler2`（8.3 亿+），RFC 1950 流式/滚动窗口/合并 Adler-32（6/6 PASS）。
- `packages/@cap/crc64/` & `cap/test/testCapCrc64.cp`: ECMA-182 / XZ / GO-ISO 三标准 CRC-64（6/6 PASS）。
- `packages/@cap/fixed-point/` & `cap/test/testCapFixedPoint.cp`: Q32.32 确定性定点数算术与 `sqrt`/`lerp`（6/6 PASS）。
- `packages/@cap/priority-queue/` & `cap/test/testCapPriorityQueue.cp`: 零堆可动态调权索引最小堆（6/6 PASS）。
- `packages/@cap/cuckoo-filter/` & `cap/test/testCapCuckooFilter.cp`: 支持 O(1) 安全删除的布谷鸟过滤器（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **335 条**高频映射（含 `spin`/`try-lock`/`utf8parse`/`icu_normalizer`/`bumpalo` 8 亿级去重直连）。

## [2026-11-26] - Batch 43 单批交付 6 大纠删码、位流、图算法与过滤器包（`@cap/dijkstra`, `@cap/reed-solomon`, `@cap/bitstream`, `@cap/segment-tree`, `@cap/crc16`, `@cap/xor-filter`），总映射突破 **345 条**！
- `packages/@cap/dijkstra/` & `cap/test/testCapDijkstra.cp`: Dijkstra + Bellman-Ford 负环检测与路径回溯（6/6 PASS）。
- `packages/@cap/reed-solomon/` & `cap/test/testCapReedSolomon.cp`: GF(2^8) 柯西矩阵纠删码与任意丢片恢复（6/6 PASS）。
- `packages/@cap/bitstream/` & `cap/test/testCapBitstream.cp`: MSB/LSB 双端位流读写与 Exp-Golomb 编解码（6/6 PASS）。
- `packages/@cap/segment-tree/` & `cap/test/testCapSegmentTree.cp`: O(log N) 迭代式线段树（`sum`/`min`/`max`）（6/6 PASS）。
- `packages/@cap/crc16/` & `cap/test/testCapCrc16.cp`: CCITT-FALSE/XMODEM/MODBUS/KERMIT/ARC/USB CRC-16（6/6 PASS）。
- `packages/@cap/xor-filter/` & `cap/test/testCapXorFilter.cp`: ACM JEA'2020 Xor8 静态指纹过滤器（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **345 条**高频映射（含 `crossbeam-deque`/`same-file`/`idna_adapter`/`zmij` 去重直连）。

## [2026-11-26] - Batch 44 单批交付 6 大超十亿级搜索、压缩与校验包（`@cap/memmem`, `@cap/aho-corasick`, `@cap/lzw`, `@cap/radix-sort`, `@cap/bwt`, `@cap/luhn`），总映射突破 **355 条**！
- `packages/@cap/memmem/` & `cap/test/testCapMemmem.cp`: 对标 Rust `memchr::memmem`（18.26 亿），BMH+KMP 混合子串搜索（6/6 PASS）。
- `packages/@cap/aho-corasick/` & `cap/test/testCapAhoCorasick.cp`: 对标 Rust `aho-corasick`（14.50 亿），AC 自动机多模式匹配（6/6 PASS）。
- `packages/@cap/lzw/` & `cap/test/testCapLzw.cp`: GIF/TIFF 标准 LZW 变长码无损压缩与解压（6/6 PASS）。
- `packages/@cap/radix-sort/` & `cap/test/testCapRadixSort.cp`: O(N) 线性 LSD 基数排序（含 IEEE-754 `f64` 保序）（6/6 PASS）。
- `packages/@cap/bwt/` & `cap/test/testCapBwt.cp`: Burrows-Wheeler 块排序变换 + Move-To-Front（6/6 PASS）。
- `packages/@cap/luhn/` & `cap/test/testCapLuhn.cp`: ISO 7812 Luhn + Damm + Verhoeff 三合一校验码（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **355 条**高频映射（含 `tower`/`yoke`/`zerovec`/`phf` 去重直连）。

## [2026-11-26] - Batch 45 单批交付 6 大频谱、索引、协议与编码包（`@cap/suffix-array`, `@cap/hamming`, `@cap/bencode`, `@cap/fft`, `@cap/crc8`, `@cap/base62`），总映射突破 **365 条**！
- `packages/@cap/suffix-array/` & `cap/test/testCapSuffixArray.cp`: 后缀数组 + Kasai O(N) LCP 与最长重复子串（6/6 PASS）。
- `packages/@cap/hamming/` & `cap/test/testCapHamming.cp`: SWAR Popcount、SimHash64 与 Hamming(7,4) 单位纠错（6/6 PASS）。
- `packages/@cap/bencode/` & `cap/test/testCapBencode.cp`: BitTorrent BEP-3 确定性编解码与规范校验（6/6 PASS）。
- `packages/@cap/fft/` & `cap/test/testCapFft.cp`: 原地基-2 Cooley-Tukey FFT/IFFT 与 Hann/Hamming 窗（6/6 PASS）。
- `packages/@cap/crc8/` & `cap/test/testCapCrc8.cp`: SMBus/Maxim/SAE-J1850/AUTOSAR/CDMA2000/Bluetooth CRC-8（6/6 PASS）。
- `packages/@cap/base62/` & `cap/test/testCapBase62.cp`: URL 安全 Base62 `u64`/定宽保序/字节切片编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **365 条**高频映射（含 `serde_urlencoded`/`utf8_iter`/`rayon`/`num_cpus` 去重直连）。

## [2026-11-26] - Batch 46 里程碑：突破 **250 官方 `.cap` 包大关 + 375 条生态映射**！（交付 `@cap/glam`, `@cap/kd-tree`, `@cap/easing`, `@cap/spline`, `@cap/pid`, `@cap/kalman`）
- `packages/@cap/glam/` & `cap/test/testCapGlam.cp`: 对标 Rust `glam`（4 亿+），`Vec3`/`Quat`/`Mat4` 3D 数学（6/6 PASS）。
- `packages/@cap/kd-tree/` & `cap/test/testCapKdTree.cp`: 零堆平衡 2D KD-Tree 最近邻与半径检索（6/6 PASS）。
- `packages/@cap/easing/` & `cap/test/testCapEasing.cp`: 25 种 Penner 缓动 + CSS `cubicBezier`（6/6 PASS）。
- `packages/@cap/spline/` & `cap/test/testCapSpline.cp`: 自然三次样条 + Catmull-Rom + Bezier（6/6 PASS）。
- `packages/@cap/pid/` & `cap/test/testCapPid.cp`: 工业级抗积分饱和 PID 控制器（6/6 PASS）。
- `packages/@cap/kalman/` & `cap/test/testCapKalman.cp`: 1D/2D Joseph 稳定卡尔曼滤波器（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **375 条**高频映射（含 `schemars`/`event-listener`/`atomic-waker`/`env_logger` 去重直连）。

## [2026-11-26] - Batch 47 纯 npm 超高下载 Top 10（2.39 亿 → 8359 万/周，交付 `@cap/fill-range` + 9 大 Web/TS 去重直连，总映射突破 **385 条**！）
- `packages/@cap/fill-range/` & `cap/test/testCapFillRange.cp`: 对标 npm `fill-range`（2.26 亿/周），零堆数字/字母/步长/零填充区间展开（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **385 条**高频映射（含 `form-data`/`body-parser`/`sharp`/`deepmerge`/`marked`/`cors`/`pg`/`decamelize`/`jsonwebtoken` 9 大亿级 npm 包去重直连）。

## [2026-11-26] - Batch 48 纯 npm 超高下载 Top 10（8119 万 → 882 万/周，交付 `@cap/debounce`, `@cap/syntax-highlight`, `@cap/dns-packet`, `@cap/sitemap`，总映射突破 **395 条**！）
- `packages/@cap/debounce/` & `cap/test/testCapDebounce.cp`: 对标 npm `lodash.debounce`（7445 万/周）+ `lodash.throttle`，确定性防抖/节流（6/6 PASS）。
- `packages/@cap/syntax-highlight/` & `cap/test/testCapSyntaxHighlight.cp`: 对标 npm `highlight.js`（5802 万/周）+ `prismjs`，多语言词法高亮（6/6 PASS）。
- `packages/@cap/dns-packet/` & `cap/test/testCapDnsPacket.cp`: 对标 npm `dns-packet`（2139 万/周），RFC 1035 DNS 编解码与防环指针解压（6/6 PASS）。
- `packages/@cap/sitemap/` & `cap/test/testCapSitemap.cp`: 对标 npm `sitemap`（882 万/周），Sitemaps.org 0.9 XML 生成（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **395 条**高频映射（含 `pino`/`compression`/`ejs`/`moment`/`fast-diff`/`ioredis` 6 大千万级 npm 包去重直连）。

## [2026-11-26] - Batch 49 纯 npm 里程碑：突破 **405 条映射 + 260+ 官方 `.cap` 包**！（交付 `@cap/cookie-sign`, `@cap/content-disposition`, `@cap/path-to-regexp`, `@cap/minisearch`, `@cap/cidr-tools`, `@cap/color-space`）
- `packages/@cap/cookie-sign/` & `cap/test/testCapCookieSign.cp`: 对标 npm `cookie-signature`（2.12 亿/周），HMAC-SHA256 Cookie 签名与 RFC 6265 解析（6/6 PASS）。
- `packages/@cap/content-disposition/` & `cap/test/testCapContentDisposition.cp`: 对标 npm `content-disposition`，RFC 6266/5987 编解码（6/6 PASS）。
- `packages/@cap/path-to-regexp/` & `cap/test/testCapPathToRegexp.cp`: 对标 npm `path-to-regexp`（6500 万/周），路由匹配与反向 URL 生成（6/6 PASS）。
- `packages/@cap/minisearch/` & `cap/test/testCapMinisearch.cp`: 对标 npm `lunr`/`minisearch`，零堆倒排索引与 BM25 全文检索（6/6 PASS）。
- `packages/@cap/cidr-tools/` & `cap/test/testCapCidrTools.cp`: 对标 npm `cidr-tools`，IPv4 CIDR 合并与差集排除（6/6 PASS）。
- `packages/@cap/color-space/` & `cap/test/testCapColorSpace.cp`: 对标 npm `color-convert`/Rust `palette`，OKLab/OKLCH/HSL/HSV 色彩转换（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **405 条**高频映射（npm 达 **188 条**，含 `crypto-js`/`semver-compare`/`bcryptjs`/`filesize` 去重直连）。

## [2026-11-26] - Batch 50 纯 npm 2 亿级 HTTP 核心大满贯（`@cap/type-is`, `@cap/range-parser`, `@cap/fresh`, `@cap/vary`, `@cap/accepts`, `@cap/shell-quote`），总映射突破 **415 条**！
- `packages/@cap/type-is/` & `cap/test/testCapTypeIs.cp`: 对标 npm `type-is`（2.27 亿/周），MIME 短名/通配/`+json` 匹配（6/6 PASS）。
- `packages/@cap/range-parser/` & `cap/test/testCapRangeParser.cp`: 对标 npm `range-parser`，RFC 9110 `Range`/`Content-Range`（6/6 PASS）。
- `packages/@cap/fresh/` & `cap/test/testCapFresh.cp`: 对标 npm `fresh`，ETag + `If-Modified-Since` 304 新鲜度校验（6/6 PASS）。
- `packages/@cap/vary/` & `cap/test/testCapVary.cp`: 对标 npm `vary`（1.77 亿/周），RFC 9110 `Vary` 头去重与 `*` 合并（6/6 PASS）。
- `packages/@cap/accepts/` & `cap/test/testCapAccepts.cp`: 对标 npm `accepts`+`negotiator`，HTTP 内容协商引擎（6/6 PASS）。
- `packages/@cap/shell-quote/` & `cap/test/testCapShellQuote.cp`: 对标 npm `shell-quote`，POSIX 防注入引号转义与分词（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **415 条**高频映射（npm 达 **198 条**，含 `is-wsl`/`is-docker`/`prompts`/`get-port` 去重直连）。

## [2026-11-26] - Batch 51 双生态 1:1 均衡单批交付 **8 大全新独立包**（`@cap/encodeurl`, `@cap/clsx`, `@cap/strip-json-comments`, `@cap/ini`, `@cap/dotenv-expand`, `@cap/bvh`, `@cap/noise`, `@cap/astar`），总映射突破 **425 条**！
- `packages/@cap/encodeurl/` & `cap/test/testCapEncodeurl.cp`: 对标 npm `encodeurl`（2.3 亿+/周）+ `escape-string-regexp`（6/6 PASS）。
- `packages/@cap/clsx/` & `cap/test/testCapClsx.cp`: 对标 npm `clsx`（1.74 亿/周）+ `classnames`（6/6 PASS）。
- `packages/@cap/strip-json-comments/` & `cap/test/testCapStripJsonComments.cp`: 对标 npm `strip-json-comments`（1.5 亿+/周）（6/6 PASS）。
- `packages/@cap/ini/` & `cap/test/testCapIni.cp`: 对标 npm `ini`（9500 万+/周）+ Rust `rust-ini`（6/6 PASS）。
- `packages/@cap/dotenv-expand/` & `cap/test/testCapDotenvExpand.cp`: 对标 npm `dotenv-expand`（5500 万+/周）（6/6 PASS）。
- `packages/@cap/bvh/` & `cap/test/testCapBvh.cp`: 3D AABB BVH 碰撞查询与 Slab 射线求交（6/6 PASS）。
- `packages/@cap/noise/` & `cap/test/testCapNoise.cp`: 2D/3D 改进 Perlin 噪声 + fBm + Ridged（6/6 PASS）。
- `packages/@cap/astar/` & `cap/test/testCapAstar.cp`: 2D 网格 4/8 邻域 A* 最优寻路（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **425 条**高频映射（**npm=203 / crates.io=222** 1:1 均衡，含 `object`/`libloading` 去重直连）。

## [2026-11-26] - Batch 52 双生态单批交付 **7 大全新独立包**（`@cap/glob-to-regex`, `@cap/cron-next`, `@cap/csv-line`, `@cap/duration-iso`, `@cap/properties`, `@cap/cuid2`, `@cap/retry-after`），总映射突破 **435 条**！
- `packages/@cap/glob-to-regex/` & `cap/test/testCapGlobToRegex.cp`: 对标 npm `glob-to-regexp`（3500 万+/周）（6/6 PASS）。
- `packages/@cap/cron-next/` & `cap/test/testCapCronNext.cp`: 对标 npm `cron-parser`（2500 万+/周）+ Rust `cron`（6/6 PASS）。
- `packages/@cap/csv-line/` & `cap/test/testCapCsvLine.cp`: 对标 npm `csv-parse`/`papaparse`，RFC 4180 CSV/TSV（6/6 PASS）。
- `packages/@cap/duration-iso/` & `cap/test/testCapDurationIso.cp`: ISO 8601 时长解析与格式化（6/6 PASS）。
- `packages/@cap/properties/` & `cap/test/testCapProperties.cp`: Java `.properties` 与 `\uXXXX` 解析（6/6 PASS）。
- `packages/@cap/cuid2/` & `cap/test/testCapCuid2.cp`: 零堆 SHA-256 Base36 分布式唯一 ID（6/6 PASS）。
- `packages/@cap/retry-after/` & `cap/test/testCapRetryAfter.cp`: RFC 9110 `Retry-After` 双格式解析（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **435 条**高频映射（**npm=209 / crates.io=226** 1:1 均衡，含 `encoding_rs`/`minimal-lexical`/`rustc-demangle` 去重直连）。

## [2026-11-26] - Batch 53 云原生可观测性与 Web 安全专场（6 大全新独立包：`@cap/ndjson`, `@cap/cache-control`, `@cap/csp`, `@cap/logfmt`, `@cap/syslog`, `@cap/traceparent`），总映射突破 **445 条**！
- `packages/@cap/ndjson/` & `cap/test/testCapNdjson.cp`: NDJSON/JSONL 流式增量行切分与写入（6/6 PASS）。
- `packages/@cap/cache-control/` & `cap/test/testCapCacheControl.cp`: RFC 9111 `Cache-Control` 解析与 TTL 计算（6/6 PASS）。
- `packages/@cap/csp/` & `cap/test/testCapCsp.cp`: W3C CSP3 安全策略构建与解析（6/6 PASS）。
- `packages/@cap/logfmt/` & `cap/test/testCapLogfmt.cp`: 云原生 `logfmt` 键值日志解析与写入（6/6 PASS）。
- `packages/@cap/syslog/` & `cap/test/testCapSyslog.cp`: RFC 5424 Syslog 报文编解码（6/6 PASS）。
- `packages/@cap/traceparent/` & `cap/test/testCapTraceparent.cp`: W3C Trace Context `traceparent`/`tracestate`（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **445 条**高频映射（**npm=216 / crates.io=229** 均衡，含 `tslib`/`open`/`is-plain-object`/`allocator-api2` 去重直连）。

## [2026-11-26] - Batch 54 双生态单批交付 **7 大全新独立包**（`@cap/prom-text`, `@cap/robots`, `@cap/statsd`, `@cap/clf-log`, `@cap/ical`, `@cap/vcard`, `@cap/netstring`），总映射突破 **455 条**！
- `packages/@cap/prom-text/` & `cap/test/testCapPromText.cp`: Prometheus `/metrics` 文本序列化器（6/6 PASS）。
- `packages/@cap/robots/` & `cap/test/testCapRobots.cp`: RFC 9309 `robots.txt` 规则评估引擎（6/6 PASS）。
- `packages/@cap/statsd/` & `cap/test/testCapStatsd.cp`: StatsD / DogStatsD UDP 指标编解码（6/6 PASS）。
- `packages/@cap/clf-log/` & `cap/test/testCapClfLog.cp`: Apache/Nginx Common & Combined 访问日志解析（6/6 PASS）。
- `packages/@cap/ical/` & `cap/test/testCapIcal.cp`: RFC 5545 `.ics` 日历事件生成与解析（6/6 PASS）。
- `packages/@cap/vcard/` & `cap/test/testCapVcard.cp`: RFC 6350 vCard 4.0 电子名片编解码（6/6 PASS）。
- `packages/@cap/netstring/` & `cap/test/testCapNetstring.cp`: Bernstein Netstring 二进制帧编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **455 条**高频映射（**npm=223 / crates.io=232** 均衡，含 `pdf-lib`/`clipboardy`/`sharded-slab` 去重直连）。

## [2026-11-26] - Batch 55 突破 **300+ 官方 `.cap` 包里程碑（302 包 LIVE）** — 单批交付 **7 大协议/路径/地理新包**，总映射达 **465 条**！
- `packages/@cap/uri-template/` & `cap/test/testCapUriTemplate.cp`: RFC 6570 URI Template Levels 1–4 零堆展开器（6/6 PASS）。
- `packages/@cap/json-pointer/` & `cap/test/testCapJsonPointer.cp`: RFC 6901 JSON Pointer 解析与流式迭代（6/6 PASS）。
- `packages/@cap/sse-parse/` & `cap/test/testCapSseParse.cp`: W3C `text/event-stream` 流式 SSE 解析器（6/6 PASS）。
- `packages/@cap/resp/` & `cap/test/testCapResp.cp`: Redis RESP2 线协议帧编解码（6/6 PASS）。
- `packages/@cap/mqtt-packet/` & `cap/test/testCapMqttPacket.cp`: OASIS MQTT v3.1.1 控制报文编解码（6/6 PASS）。
- `packages/@cap/rss/` & `cap/test/testCapRss.cp`: RSS 2.0 XML 生成与标签提取（6/6 PASS）。
- `packages/@cap/polyline/` & `cap/test/testCapPolyline.cp`: Google Encoded Polyline (precision 5/6) GPS 轨迹编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **465 条**高频映射（**npm=229 / crates.io=236** 均衡，含亿级 npm `braces`/`pako`/`ansi-escapes` 去重直连）。

## [2026-11-26] - Batch 56 双生态均衡交付 **6 大高频新包**（`@cap/etag`, `@cap/bytes`, `@cap/media-type`, `@cap/stomp`, `@cap/iban`, `@cap/isbn`），总映射突破 **475 条**！
- `packages/@cap/etag/` & `cap/test/testCapEtag.cp`: RFC 9110 HTTP `ETag` 生成解析与 `If-Match`/`If-None-Match` 强弱匹配（6/6 PASS）。
- `packages/@cap/bytes/` & `cap/test/testCapBytes.cp`: IEC/SI 字节大小零堆解析与人类可读格式化（6/6 PASS）。
- `packages/@cap/media-type/` & `cap/test/testCapMediaType.cp`: RFC 6838/9110 MIME 媒体类型与 `+suffix` 解析（6/6 PASS）。
- `packages/@cap/stomp/` & `cap/test/testCapStomp.cp`: STOMP 1.2 MQ 协议帧零拷贝编解码（6/6 PASS）。
- `packages/@cap/iban/` & `cap/test/testCapIban.cp`: ISO 13616 88 国 IBAN Mod-97 校验与格式化（6/6 PASS）。
- `packages/@cap/isbn/` & `cap/test/testCapIsbn.cp`: ISBN-10/13/ISSN/EAN-13 校验与互转（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **475 条**高频映射（**npm=235 / crates.io=240** 1:1 均衡，含亿级 `tsconfig-paths`/`tar`/`fast-xml-parser`/`forwarded-header-value` 去重直连）。

## [2026-11-26] - Batch 57 HTTP 认证/状态/查询/Cookie 六件套（`@cap/qs-lite`, `@cap/statuses`, `@cap/basic-auth`, `@cap/cookie-jar`, `@cap/bearer-auth`, `@cap/server-timing`）全 6/6 PASS！
- `packages/@cap/qs-lite/` & `cap/test/testCapQsLite.cp`: 零堆 URL 查询字符串解析与序列化（6/6 PASS）。
- `packages/@cap/statuses/` & `cap/test/testCapStatuses.cp`: 62 个 IANA HTTP 状态码二分查询与分类判定（6/6 PASS）。
- `packages/@cap/basic-auth/` & `cap/test/testCapBasicAuth.cp`: RFC 7617 Basic 认证编解码与恒定时间校验（6/6 PASS）。
- `packages/@cap/cookie-jar/` & `cap/test/testCapCookieJar.cp`: RFC 6265 Cookie 域/路径匹配与组装（6/6 PASS）。
- `packages/@cap/bearer-auth/` & `cap/test/testCapBearerAuth.cp`: RFC 6750 Bearer Token 提取与 JWT 预检（6/6 PASS）。
- `packages/@cap/server-timing/` & `cap/test/testCapServerTiming.cp`: W3C `Server-Timing` 响应性能头编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 同步扩充并升级 `qs`/`statuses`/`basic-auth`/`tough-cookie`/`express-bearer-token`/`server-timing`/`date-fns`/`http-auth-basic`/`cookie_store`/`serde_urlencoded` 映射。

## [2026-11-26] - Batch 58 双生态交付 **5 大 API 网关/RPC 核心新包**（`@cap/http-date`, `@cap/json-rpc`, `@cap/cors-match`, `@cap/rate-header`, `@cap/user-agent-hints`），总映射冲至 **495 条**（距 500 映射大关仅 5 条）！
- `packages/@cap/http-date/` & `cap/test/testCapHttpDate.cp`: RFC 9110 三格式 HTTP-date 解析与 Hinnant 公历互转（6/6 PASS）。
- `packages/@cap/json-rpc/` & `cap/test/testCapJsonRpc.cp`: JSON-RPC 2.0 协议帧零堆编码与信封校验（6/6 PASS）。
- `packages/@cap/cors-match/` & `cap/test/testCapCorsMatch.cp`: CORS Origin 通配子域匹配与预检校验（6/6 PASS）。
- `packages/@cap/rate-header/` & `cap/test/testCapRateHeader.cp`: IETF RateLimit Headers 解析与退避计算（6/6 PASS）。
- `packages/@cap/user-agent-hints/` & `cap/test/testCapUserAgentHints.cp`: W3C `Sec-CH-UA` Client Hints 解析与 GREASE 过滤（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **495 条**双生态高频映射（**npm=247 / crates.io=248** 1:1 绝对均衡）。

## [2026-11-26] - Batch 59 正式突破 **500+ 双生态映射大关（505 条）** — 交付 **6 大网络/安全协议新包**（`@cap/ws-frame`, `@cap/accept-lang`, `@cap/webhook-sig`, `@cap/idempotency`, `@cap/digest-auth`, `@cap/proxy-proto`）！
- `packages/@cap/ws-frame/` & `cap/test/testCapWsFrame.cp`: RFC 6455 WebSocket 二进制帧与掩码编解码（6/6 PASS）。
- `packages/@cap/accept-lang/` & `cap/test/testCapAcceptLang.cp`: RFC 9110 `Accept-Language` 权重排序与协商（6/6 PASS）。
- `packages/@cap/webhook-sig/` & `cap/test/testCapWebhookSig.cp`: Stripe/GitHub/Standard Webhooks 防重放签名解析（6/6 PASS）。
- `packages/@cap/idempotency/` & `cap/test/testCapIdempotency.cp`: IETF `Idempotency-Key` 请求指纹与四态判定（6/6 PASS）。
- `packages/@cap/digest-auth/` & `cap/test/testCapDigestAuth.cp`: RFC 7616 HTTP Digest 认证头零拷贝编解码（6/6 PASS）。
- `packages/@cap/proxy-proto/` & `cap/test/testCapProxyProto.cp`: HAProxy PROXY Protocol v1/v2 零堆解析编码（6/6 PASS）。
- `packages/@cap/registry.json`: 突破 **505 条**双生态高频映射（**npm=252 / crates.io=253** 1:1 绝对均衡）。

## [2026-11-26] - Batch 60 达成 **60 批大满贯** — 交付 **6 大安全/代理/可观测新包**（`@cap/pem-parse`, `@cap/grpc-frame`, `@cap/socks5-frame`, `@cap/totp-uri`, `@cap/ssh-key`, `@cap/otel-baggage`），总映射达 **514 条（npm=257 / crates.io=257）**！
- `packages/@cap/pem-parse/` & `cap/test/testCapPemParse.cp`: RFC 7468 PEM 证书链零堆解析与 64 列编码（6/6 PASS）。
- `packages/@cap/grpc-frame/` & `cap/test/testCapGrpcFrame.cp`: gRPC 5 字节帧、17 状态码与 timeout 解析（6/6 PASS）。
- `packages/@cap/socks5-frame/` & `cap/test/testCapSocks5Frame.cp`: RFC 1928/1929 SOCKS5 代理帧编解码（6/6 PASS）。
- `packages/@cap/totp-uri/` & `cap/test/testCapTotpUri.cp`: `otpauth://totp|hotp` 2FA URI 解析与序列化（6/6 PASS）。
- `packages/@cap/ssh-key/` & `cap/test/testCapSshKey.cp`: OpenSSH `authorized_keys` 公钥解析与格式化（6/6 PASS）。
- `packages/@cap/otel-baggage/` & `cap/test/testCapOtelBaggage.cp`: W3C OpenTelemetry Baggage 头部编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **514 条**双生态映射（**npm=257 / crates.io=257** 绝对 1:1 平衡）。

## [2026-11-26] - Batch 61 云原生与 HTTP/2 六件套（`@cap/jwt-parse`, `@cap/arn-parse`, `@cap/multipart-boundary`, `@cap/http2-hpack`, `@cap/docker-ref`, `@cap/s3-uri`），总映射突破 **523 条**！
- `packages/@cap/jwt-parse/` & `cap/test/testCapJwtParse.cp`: RFC 7519 JWT 零拷贝解码与 `alg:none` 安全预检（6/6 PASS）。
- `packages/@cap/arn-parse/` & `cap/test/testCapArnParse.cp`: AWS ARN 解析与 IAM 通配符匹配（6/6 PASS）。
- `packages/@cap/multipart-boundary/` & `cap/test/testCapMultipartBoundary.cp`: RFC 7578 `multipart/form-data` 零拷贝 Part 迭代（6/6 PASS）。
- `packages/@cap/http2-hpack/` & `cap/test/testCapHttp2Hpack.cp`: RFC 7541 HTTP/2 HPACK 静态表与前缀整数编解码（6/6 PASS）。
- `packages/@cap/docker-ref/` & `cap/test/testCapDockerRef.cp`: OCI/Docker 镜像引用解析与规范化（6/6 PASS）。
- `packages/@cap/s3-uri/` & `cap/test/testCapS3Uri.cp`: AWS S3 三格式 URI 解析与 Bucket 校验（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **523 条**双生态高频映射（**npm=262 / crates.io=261** 1:1 均衡）。

## [2026-11-26] - Batch 62 数据库/消息队列连接串与企业身份六件套（`@cap/pg-dsn`, `@cap/redis-url`, `@cap/amqp-url`, `@cap/purl`, `@cap/ldap-dn`, `@cap/magnet-uri`），总映射突破 **533 条**！
- `packages/@cap/pg-dsn/` & `cap/test/testCapPgDsn.cp`: PostgreSQL URI/libpq 连接串双格式解析与脱敏（6/6 PASS）。
- `packages/@cap/redis-url/` & `cap/test/testCapRedisUrl.cp`: Redis/Valkey TCP/TLS/Unix 连接串解析与脱敏（6/6 PASS）。
- `packages/@cap/amqp-url/` & `cap/test/testCapAmqpUrl.cp`: RabbitMQ/AMQP 连接串解析与 vhost 处理（6/6 PASS）。
- `packages/@cap/purl/` & `cap/test/testCapPurl.cp`: SBOM Package URL 规范解析与序列化（6/6 PASS）。
- `packages/@cap/ldap-dn/` & `cap/test/testCapLdapDn.cp`: RFC 4514 LDAP DN 解析与转义（6/6 PASS）。
- `packages/@cap/magnet-uri/` & `cap/test/testCapMagnetUri.cp`: BitTorrent v1/v2 Magnet URI 解析与构建（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **533 条**双生态映射（**npm=267 / crates.io=266** 绝对 1:1 均衡）。

## [2026-11-26] - Batch 63 突破 **350 官方 `.cap` 包大关** — 交付 **6 大 RFC/W3C/CNCF 标识协议新包**（`@cap/geo-uri`, `@cap/mailto-uri`, `@cap/tel-uri`, `@cap/urn-parse`, `@cap/spiffe-id`, `@cap/did-parse`），总映射达 **542 条（npm=271 / crates.io=271）**！
- `packages/@cap/geo-uri/` & `cap/test/testCapGeoUri.cp`: RFC 5870 `geo:` 2D/3D 地理坐标 URI 解析与格式化（6/6 PASS）。
- `packages/@cap/mailto-uri/` & `cap/test/testCapMailtoUri.cp`: RFC 6068 `mailto:` URI 解析与百分号编码（6/6 PASS）。
- `packages/@cap/tel-uri/` & `cap/test/testCapTelUri.cp`: RFC 3966 `tel:` URI 与 E.164 国际电话归一化（6/6 PASS）。
- `packages/@cap/urn-parse/` & `cap/test/testCapUrnParse.cp`: RFC 8141 URN 解析与等价比较（6/6 PASS）。
- `packages/@cap/spiffe-id/` & `cap/test/testCapSpiffeId.cp`: CNCF SPIFFE 零信任工作负载 ID 解析与校验（6/6 PASS）。
- `packages/@cap/did-parse/` & `cap/test/testCapDidParse.cp`: W3C DID Core 1.0 去中心化身份解析（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **542 条**双生态高频映射（**npm=271 / crates.io=271** 绝对 1:1 均衡）。

## [2026-11-26] - Batch 64 亿级包管/数据库/消息流六件套（`@cap/semver-range`, `@cap/npm-spec`, `@cap/mysql-dsn`, `@cap/mongo-uri`, `@cap/kafka-topic`, `@cap/at-uri`），总映射突破 **551 条**！
- `packages/@cap/semver-range/` & `cap/test/testCapSemverRange.cp`: npm/Cargo SemVer 范围解析与最高版本匹配（6/6 PASS）。
- `packages/@cap/npm-spec/` & `cap/test/testCapNpmSpec.cp`: npm 包名校验与 `@scope/pkg@ver` 解析（6/6 PASS）。
- `packages/@cap/mysql-dsn/` & `cap/test/testCapMysqlDsn.cp`: MySQL/MariaDB/TiDB 双格式连接串解析与脱敏（6/6 PASS）。
- `packages/@cap/mongo-uri/` & `cap/test/testCapMongoUri.cp`: MongoDB `mongodb://`/`mongodb+srv://` 多主机连接串解析（6/6 PASS）。
- `packages/@cap/kafka-topic/` & `cap/test/testCapKafkaTopic.cp`: Kafka Topic 校验与官方向量对齐 Murmur2 分区路由（6/6 PASS）。
- `packages/@cap/at-uri/` & `cap/test/testCapAtUri.cp`: AT Protocol `at://` URI 与 NSID 校验（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **551 条**双生态高频映射（**npm=275 / crates.io=276** 绝对 1:1 均衡）。

## [2026-11-26] - Batch 65 亿级通用基础库六件套（`@cap/glob-match`, `@cap/html-escape`, `@cap/regex-escape`, `@cap/magic-bytes`, `@cap/sql-escape`, `@cap/cron-expr`），总映射突破 **560 条**！
- `packages/@cap/glob-match/` & `cap/test/testCapGlobMatch.cp`: 防 ReDoS Glob 通配符与 `.gitignore` 否定规则引擎（6/6 PASS）。
- `packages/@cap/html-escape/` & `cap/test/testCapHtmlEscape.cp`: HTML XSS 实体转义与 UTF-8 数字/命名实体反转义（6/6 PASS）。
- `packages/@cap/regex-escape/` & `cap/test/testCapRegexEscape.cp`: 正则字面量与字符类转义引擎（6/6 PASS）。
- `packages/@cap/magic-bytes/` & `cap/test/testCapMagicBytes.cp`: 25 种主流二进制文件魔数签名嗅探（6/6 PASS）。
- `packages/@cap/sql-escape/` & `cap/test/testCapSqlEscape.cp`: ANSI/PG/MySQL 字符串/标识符/LIKE 转义（6/6 PASS）。
- `packages/@cap/cron-expr/` & `cap/test/testCapCronExpr.cp`: 5 段 Unix Cron 64-bit 位图编译与 O(1) 匹配（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **560+ 条**双生态高频映射（npm / crates.io 1:1 均衡）。

## [2026-11-26] - Batch 66 亿级 CLI/文本/编码六件套（`@cap/ms-parse`, `@cap/proc-args`, `@cap/indent-strip`, `@cap/shell-escape`, `@cap/utf16-codec`, `@cap/mime-sniff`），总映射突破 **571 条**！
- `packages/@cap/ms-parse/` & `cap/test/testCapMsParse.cp`: 人类可读时长解析与短/长格式输出（6/6 PASS）。
- `packages/@cap/proc-args/` & `cap/test/testCapProcArgs.cp`: POSIX/GNU CLI 参数解析与 `-abc` 短簇拆分（6/6 PASS）。
- `packages/@cap/indent-strip/` & `cap/test/testCapIndentStrip.cp`: 多行文本公共缩进剥离与统一缩进（6/6 PASS）。
- `packages/@cap/shell-escape/` & `cap/test/testCapShellEscape.cp`: POSIX/Windows 命令行安全转义（6/6 PASS）。
- `packages/@cap/utf16-codec/` & `cap/test/testCapUtf16Codec.cp`: UTF-8 <-> UTF-16/LE 代理对与 BOM 编解码（6/6 PASS）。
- `packages/@cap/mime-sniff/` & `cap/test/testCapMimeSniff.cp`: WHATWG 二进制/文本/HTML/JSON 内容嗅探（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **571 条**双生态高频映射（**npm=285 / crates.io=286** 绝对 1:1 均衡）。

## [2026-11-26] - Batch 67 亿级文本与终端处理六件套（`@cap/ansi-strip`, `@cap/camelcase`, `@cap/trunc-utf8`, `@cap/slugify`, `@cap/title-case`, `@cap/line-col`），总映射突破 **580 条（npm=290 / crates.io=290）**！
- `packages/@cap/ansi-strip/` & `cap/test/testCapAnsiStrip.cp`: 零堆 CSI/OSC ANSI 终端转义序列剥离（6/6 PASS）。
- `packages/@cap/camelcase/` & `cap/test/testCapCamelcase.cp`: 五种编程命名风格与连续缩写智能互转（6/6 PASS）。
- `packages/@cap/trunc-utf8/` & `cap/test/testCapTruncUtf8.cp`: UTF-8 码点安全截断与省略号拼接（6/6 PASS）。
- `packages/@cap/slugify/` & `cap/test/testCapSlugify.cp`: Latin 重音折叠 URL Slug 生成器（6/6 PASS）。
- `packages/@cap/title-case/` & `cap/test/testCapTitleCase.cp`: AP/Chicago 英文标题大小写与句首大写（6/6 PASS）。
- `packages/@cap/line-col/` & `cap/test/testCapLineCol.cp`: 字节偏移 <-> 1-based 行列号定位与行提取（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **580 条**双生态映射（**npm=290 / crates.io=290** 绝对 1:1 对称）。

## [2026-11-26] - Batch 68 亿级域名/网段/自然排序六件套（`@cap/punycode`, `@cap/ip-range`, `@cap/natural-cmp`, `@cap/word-count`, `@cap/roman-num`, `@cap/pad-str`），总映射突破 **589 条**！
- `packages/@cap/punycode/` & `cap/test/testCapPunycode.cp`: RFC 3492 IDNA `xn--` Punycode 编解码（6/6 PASS）。
- `packages/@cap/ip-range/` & `cap/test/testCapIpRange.cp`: IPv4 严格解析、CIDR 重叠检测与 RFC 保留网段分类（6/6 PASS）。
- `packages/@cap/natural-cmp/` & `cap/test/testCapNaturalCmp.cp`: 无溢出自然序比较与 Ciura 原地排序（6/6 PASS）。
- `packages/@cap/word-count/` & `cap/test/testCapWordCount.cp`: 中英日韩多语言字数统计与阅读时长估算（6/6 PASS）。
- `packages/@cap/roman-num/` & `cap/test/testCapRomanNum.cp`: 1..3999 严格规范罗马数字双向编解码（6/6 PASS）。
- `packages/@cap/pad-str/` & `cap/test/testCapPadStr.cp`: UTF-8 码点感知字符串对齐填充与重复（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **589 条**双生态高频映射（**npm=294 / crates.io=295** 绝对 1:1 均衡）。

## [2026-11-26] - Batch 69 正式突破 **600 双生态映射里程碑（npm=300 / crates.io=300 绝对 1:1 对称！）** — 交付分布式 ID 三巨头与 Bitflags 五件套（`@cap/ulid`, `@cap/uuid-v7`, `@cap/snowflake`, `@cap/bitflags`, `@cap/morse-code`）！
- `packages/@cap/ulid/` & `cap/test/testCapUlid.cp`: ULID 128-bit Crockford Base32 编解码与单调递增（6/6 PASS）。
- `packages/@cap/uuid-v7/` & `cap/test/testCapUuidV7.cp`: RFC 9562 UUIDv7 编解码与时间戳提取（6/6 PASS）。
- `packages/@cap/snowflake/` & `cap/test/testCapSnowflake.cp`: 64-bit Twitter/Discord Snowflake 分布式 ID 生成与反解（6/6 PASS）。
- `packages/@cap/bitflags/` & `cap/test/testCapBitflags.cp`: 64-bit 标志位集合代数与位运算（6/6 PASS）。
- `packages/@cap/morse-code/` & `cap/test/testCapMorseCode.cp`: ITU-R M.1677 国际摩尔斯电码编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 正式突破 **600 条**双生态高频映射（**npm=300 / crates.io=300** 完美对称）。

## [2026-11-26] - Batch 70 达成 **70 批大满贯** — 交付 **6 大亿级二进制/文本编码与 ID 新包**（`@cap/vlq`, `@cap/leb128`, `@cap/ksuid`, `@cap/base32-crockford`, `@cap/zbase32`, `@cap/ascii85`），总映射达 **610 条（npm=305 / crates.io=305）**！
- `packages/@cap/vlq/` & `cap/test/testCapVlq.cp`: Source Map v3 Base64 VLQ 编解码（6/6 PASS）。
- `packages/@cap/leb128/` & `cap/test/testCapLeb128.cp`: WebAssembly/DWARF ULEB128 & SLEB128 编解码（6/6 PASS）。
- `packages/@cap/ksuid/` & `cap/test/testCapKsuid.cp`: Segment KSUID 27 字符 Base62 编解码（6/6 PASS）。
- `packages/@cap/base32-crockford/` & `cap/test/testCapBase32Crockford.cp`: Crockford Base32 容错与模 37 校验编解码（6/6 PASS）。
- `packages/@cap/zbase32/` & `cap/test/testCapZbase32.cp`: Zooko z-base-32 人类友好编解码（6/6 PASS）。
- `packages/@cap/ascii85/` & `cap/test/testCapAscii85.cp`: Adobe Ascii85 含 `z` 压缩与 `<~...~>` 编解码（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **610 条**双生态高频映射（**npm=305 / crates.io=305** 绝对 1:1 对称）。

## [2026-11-26] - Batch 71 编解码/颜色/语音算法六件套（`@cap/quoted-printable`, `@cap/hex-color`, `@cap/base36`, `@cap/zigzag`, `@cap/uuencode`, `@cap/soundex`），官方包冲至 **397 个**（距 **400 包终极里程碑**仅差 3 步！），总映射 **620 条（310/310）**！
- `packages/@cap/quoted-printable/` & `cap/test/testCapQuotedPrintable.cp`: RFC 2045 Quoted-Printable 76 列软折行编解码（6/6 PASS）。
- `packages/@cap/hex-color/` & `cap/test/testCapHexColor.cp`: CSS 十六进制颜色解析、Alpha 合成与亮暗判定（6/6 PASS）。
- `packages/@cap/base36/` & `cap/test/testCapBase36.cp`: Base36 与 2..36 任意进制防溢出编解码（6/6 PASS）。
- `packages/@cap/zigzag/` & `cap/test/testCapZigzag.cp`: 32/64-bit ZigZag 与 ZigZag-Varint 流式编解码（6/6 PASS）。
- `packages/@cap/uuencode/` & `cap/test/testCapUuencode.cp`: POSIX `uuencode`/`uudecode` 零堆编解码（6/6 PASS）。
- `packages/@cap/soundex/` & `cap/test/testCapSoundex.cp`: NARA American Soundex 语音编码与匹配（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **620 条**双生态高频映射（**npm=310 / crates.io=310** 绝对 1:1 对称）。

## [2026-11-26] - Batch 72 正式跨越 **400+ 官方 `.cap` 包终极里程碑（403 包 LIVE + 630 条双生态映射）**！
- `packages/@cap/base32-rfc/` & `cap/test/testCapBase32Rfc.cp`: RFC 4648 标准 Base32 与 `base32hex` 编解码（6/6 PASS）。
- `packages/@cap/css-color/` & `cap/test/testCapCssColor.cp`: CSS Color Level 4 函数式颜色解析与 HSL/RGB 互转（6/6 PASS）。
- `packages/@cap/levenshtein/` & `cap/test/testCapLevenshtein.cp`: Levenshtein & Damerau-Levenshtein 编辑距离与最近词查找（6/6 PASS）。
- `packages/@cap/jaro-winkler/` & `cap/test/testCapJaroWinkler.cp`: Jaro & Jaro-Winkler 字符串相似度算法（6/6 PASS）。
- `packages/@cap/dice-coeff/` & `cap/test/testCapDiceCoeff.cp`: Sørensen-Dice Bigram 多重集相似度（6/6 PASS）。
- `packages/@cap/rot13/` & `cap/test/testCapRot13.cp`: ROT13 / ROT47 / Caesar / Atbash 字母表置换（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **630 条**双生态高频映射（**npm=316 / crates.io=314** 1:1 均衡）。

### 2026-11-27 (Batch 73 — 640 映射精确 1:1 对称 & 410 官方包)
- `packages/@cap/fnv-hash/` & `cap/test/testCapFnvHash.cp`: FNV-1 / FNV-1a 32/64 位流式哈希（6/6 PASS）。
- `packages/@cap/metaphone/` & `cap/test/testCapMetaphone.cp`: Metaphone 语音编码器（6/6 PASS）。
- `packages/@cap/bmh-search/` & `cap/test/testCapBmhSearch.cp`: Boyer-Moore-Horspool 快速字节子串搜索（6/6 PASS）。
- `packages/@cap/kmp-search/` & `cap/test/testCapKmpSearch.cp`: KMP 线性时间搜索与 LPS 构造（6/6 PASS）。
- `packages/@cap/porter-stem/` & `cap/test/testCapPorterStem.cp`: Porter 英文词干提取（6/6 PASS）。
- `packages/@cap/lcs-diff/` & `cap/test/testCapLcsDiff.cp`: LCS 长度、Diff 统计、相似度与最长公共子串（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **640 条**双生态高频映射（**npm=320 / crates.io=320 绝对 1:1 对称**）。

### 2026-11-27 (Batch 74 — 650 映射精确 1:1 对称 & 414 官方包)
- `packages/@cap/siphash-pure/` & `cap/test/testCapSiphashPure.cp`: SipHash-2-4 & SipHash-1-3 官方向量全通过（6/6 PASS）。
- `packages/@cap/xoshiro256/` & `cap/test/testCapXoshiro256.cp`: Xoshiro256** PRNG、SplitMix64 播种与 2^128 跳跃（6/6 PASS）。
- `packages/@cap/pcg32/` & `cap/test/testCapPcg32.cp`: PCG32 PRNG、无偏范围采样与 O(log n) advance（6/6 PASS）。
- `packages/@cap/wyhash/` & `cap/test/testCapWyhash.cp`: wyhash v4 64 位哈希与 WyRng（6/6 PASS）。
- `packages/@cap/chacha-rng/` & `cap/test/testCapChachaRng.cp`: RFC 8439 ChaCha8/12/20 块生成与 CSPRNG（6/6 PASS）。
- `packages/@cap/math-interp/` & `cap/test/testCapMathInterp.cp`: 插值、限幅、重映射、环绕与缓动曲线（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **650 条**双生态高频映射（**npm=325 / crates.io=325 绝对 1:1 对称**）。

### 2026-11-27 (Batch 75 — 660 映射精确 1:1 对称 & 420+ 官方包)
- `packages/@cap/stats-basic/` & `cap/test/testCapStatsBasic.cp`: Welford 在线统计、Chan 合并、Pearson 相关系数与百分位（6/6 PASS）。
- `packages/@cap/bloom-filter/` & `cap/test/testCapBloomFilter.cp`: 零堆外部位图双哈希布隆过滤器（6/6 PASS）。
- `packages/@cap/vec2/` & `cap/test/testCapVec2.cp`: 2D 向量与平面几何库（6/6 PASS）。
- `packages/@cap/vec3/` & `cap/test/testCapVec3.cp`: 3D 向量与空间几何库（6/6 PASS）。
- `packages/@cap/fletcher/` & `cap/test/testCapFletcher.cp`: Fletcher-16/32/64 校验和（6/6 PASS）。
- `packages/@cap/running-avg/` & `cap/test/testCapRunningAvg.cp`: EMA / SMA / WMA 流式移动平均（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **660 条**双生态高频映射（**npm=330 / crates.io=330 绝对 1:1 对称**）。

### 2026-11-27 (Batch 76 — 670 映射精确 1:1 对称 & 425+ 官方包)
- `packages/@cap/quat/` & `cap/test/testCapQuat.cp`: 3D 单位四元数、旋转与 SLERP/NLERP（6/6 PASS）。
- `packages/@cap/bezier-curve/` & `cap/test/testCapBezierCurve.cp`: 二/三次贝塞尔曲线、de Casteljau 分割与 CSS `cubic-bezier`（6/6 PASS）。
- `packages/@cap/mat3/` & `cap/test/testCapMat3.cp`: 3x3 矩阵与 2D 仿射变换（6/6 PASS）。
- `packages/@cap/mat2/` & `cap/test/testCapMat2.cp`: 2x2 矩阵与 2D 线性变换（6/6 PASS）。
- `packages/@cap/bspline/` & `cap/test/testCapBspline.cp`: Hermite / Catmull-Rom / 均匀三次 B-Spline（6/6 PASS）。
- `packages/@cap/color-hsl/` & `cap/test/testCapColorHsl.cp`: RGB / HSL / HSV / CMYK 色彩空间互转（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **670 条**双生态高频映射（**npm=335 / crates.io=335 绝对 1:1 对称**）。

### 2026-11-27 (Batch 77 — 680 映射精确 1:1 对称 & 432+ 官方包)
- `packages/@cap/complex-num/` & `cap/test/testCapComplexNum.cp`: 复数四则、极坐标、欧拉指数与复平方根（6/6 PASS）。
- `packages/@cap/pid-ctrl/` & `cap/test/testCapPidCtrl.cp`: 工业级 PID 控制器（抗积分饱和 & 测量微分，6/6 PASS）。
- `packages/@cap/kalman1d/` & `cap/test/testCapKalman1d.cp`: 1D 标量卡尔曼滤波器（6/6 PASS）。
- `packages/@cap/rational/` & `cap/test/testCapRational.cp`: 精确有理数/分数四则与比较（6/6 PASS）。
- `packages/@cap/aabb2/` & `cap/test/testCapAabb2.cp`: 2D AABB 包围盒与 2D 射线 Slab 测试（6/6 PASS）。
- `packages/@cap/aabb3/` & `cap/test/testCapAabb3.cp`: 3D AABB 包围盒与 3D 射线 Slab 测试（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **680 条**双生态高频映射（**npm=340 / crates.io=340 绝对 1:1 对称**）。

### 2026-11-27 (Batch 78 — 690 映射精确 1:1 对称 & 437+ 官方包，正式启动 Top-700 + 独立依赖拆分)
- `packages/@cap/perlin-noise/` & `cap/test/testCapPerlinNoise.cp`: 1D/2D/3D Perlin 梯度噪声、fBm 与山脊噪声（6/6 PASS）。
- `packages/@cap/fft-radix2/` & `cap/test/testCapFftRadix2.cp`: 原地 Radix-2 FFT / IFFT 与频谱分析（6/6 PASS）。
- `packages/@cap/fixed-q16/` & `cap/test/testCapFixedQ16.cp`: Q16.16 确定性定点数算术（6/6 PASS）。
- `packages/@cap/haversine-geo/` & `cap/test/testCapHaversineGeo.cp`: WGS-84 Haversine 球面距离与方位角（6/6 PASS）。
- `packages/@cap/polygon2d/` & `cap/test/testCapPolygon2d.cp`: 2D 多边形面积、质心、射线法与凹凸性（6/6 PASS）。
- `packages/@cap/mat4/` & `cap/test/testCapMat4.cp`: 4x4 3D 变换/投影/LookAt 矩阵（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **690 条**双生态高频映射（**npm=345 / crates.io=345 绝对 1:1 对称**，含 Top-59 Rust `semver` 12.5 亿下载）。

### 2026-11-27 (Batch 79 — 跨越 **700 条双生态映射里程碑**：npm=350 / crates.io=350 绝对 1:1 对称！)
- `packages/@cap/convex-hull/` & `cap/test/testCapConvexHull.cp`: Andrew 单调链 2D 凸包（6/6 PASS）。
- `packages/@cap/geohash-pure/` & `cap/test/testCapGeohashPure.cp`: Geohash Base32 编码解码与边界盒（6/6 PASS）。
- `packages/@cap/ramer-douglas-peucker/` & `cap/test/testCapRamerDouglasPeucker.cp`: 非递归 RDP 2D 折线抽稀（6/6 PASS）。
- `packages/@cap/bigint-u256/` & `cap/test/testCapBigintU256.cp`: 256 位无符号大整数 `U256`（6/6 PASS）。
- `packages/@cap/quadtree-2d/` & `cap/test/testCapQuadtree2d.cp`: 池化零堆 2D 四叉树与矩形/半径查询（6/6 PASS）。
- `packages/@cap/decimal-fixed/` & `cap/test/testCapDecimalFixed.cp`: 金融级高精度十进制 `Decimal`（6/6 PASS）。
- `packages/@cap/registry.json`: 正式突破 **700 条**双生态高频映射（**npm=350 / crates.io=350 绝对 1:1 对称**，含 Top-82 Rust `shlex` 11.6 亿下载）。

### 2026-11-27 (Batch 80 — 80 批次大里程碑！710 映射 355/355 绝对对称，全面吸收 Top-150 核心依赖链)
- `packages/@cap/block-buffer/` & `cap/test/testCapBlockBuffer.cp`: crates.io #37 `block-buffer`（14.3 亿下载，6/6 PASS）。
- `packages/@cap/unicode-width/` & `cap/test/testCapUnicodeWidth.cp`: crates.io #110 `unicode-width`（10.1 亿下载）+ npm `string-width`（6/6 PASS）。
- `packages/@cap/anstyle-parse/` & `cap/test/testCapAnstyleParse.cp`: crates.io #151 `anstyle-parse`（8.65 亿下载，6/6 PASS）。
- `packages/@cap/is-number/` & `cap/test/testCapIsNumber.cp`: npm `is-number`（4 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/to-regex-range/` & `cap/test/testCapToRegexRange.cp`: npm `to-regex-range`（3 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/normalize-path/` & `cap/test/testCapNormalizePath.cp`: npm `normalize-path`（3.5 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **710 条**双生态高频映射（**npm=355 / crates.io=355 绝对 1:1 对称**）。

### 2026-11-27 (Batch 81 — 720 映射 360/360 绝对对称！跨包依赖复用落地 + 40 亿级 crates.io 头部吸收)
- `packages/@cap/is-extglob/` & `cap/test/testCapIsExtglob.cp`: npm `is-extglob`（3 亿+/周独立前置依赖，6/6 PASS）。
- `packages/@cap/is-glob/` & `cap/test/testCapIsGlob.cp`: npm `is-glob`（3.5 亿+/周，直接 `import` 复用 `@cap/is-extglob`，6/6 PASS）。
- `packages/@cap/slash/` & `cap/test/testCapSlash.cp`: npm `slash`（3.5 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/shebang-regex/` & `cap/test/testCapShebangRegex.cp`: npm `shebang-regex`（2.5 亿+/周独立前置依赖，6/6 PASS）。
- `packages/@cap/path-key/` & `cap/test/testCapPathKey.cp`: npm `path-key`（2.5 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/anstyle/` & `cap/test/testCapAnstyle.cp`: crates.io #127 `anstyle`（9.57 亿下载，依赖 `@cap/anstyle-parse`，6/6 PASS）。
- `packages/@cap/nom-bytes/` & `cap/test/testCapNomBytes.cp`: crates.io #62 `winnow`（12.4 亿）+ #135 `nom`（9.38 亿，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **720 条**双生态高频映射（**npm=360 / crates.io=360 绝对 1:1 对称**）。

### 2026-11-27 (Batch 82 — 730 映射 365/365 绝对对称！一次性吸收 Rust `time` 核心依赖三件套 + `prost` + `libm`)
- `packages/@cap/shebang-command/` & `cap/test/testCapShebangCommand.cp`: npm `shebang-command`（2.5 亿+/周，复用 `@cap/shebang-regex`，6/6 PASS）。
- `packages/@cap/anymatch/` & `cap/test/testCapAnymatch.cp`: npm `anymatch`（2 亿+/周，复用 `@cap/normalize-path` + `@cap/is-glob`，6/6 PASS）。
- `packages/@cap/deranged/` & `cap/test/testCapDeranged.cp`: crates.io #187 `deranged`（8.01 亿下载，6/6 PASS）。
- `packages/@cap/num-conv/` & `cap/test/testCapNumConv.cp`: crates.io #206 `num-conv`（7.53 亿下载，6/6 PASS）。
- `packages/@cap/prost-wire/` & `cap/test/testCapProstWire.cp`: crates.io #208 `prost`（7.42 亿下载，复用 `@cap/varint`，6/6 PASS）。
- `packages/@cap/powerfmt/` & `cap/test/testCapPowerfmt.cp`: crates.io #210 `powerfmt`（7.36 亿下载，6/6 PASS）。
- `packages/@cap/libm/` & `cap/test/testCapLibm.cp`: crates.io #266 `libm`（6.26 亿下载全生态数学基石，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **730 条**双生态高频映射（**npm=365 / crates.io=365 绝对 1:1 对称**）。

### 2026-11-27 (Batch 83 — 740 映射 370/370 绝对对称！4 新独立包 + 4 历史旧包深度重构解耦)
- `packages/@cap/zerotrie/` & `cap/test/testCapZerotrie.cp`: crates.io #264 `zerotrie`（6.28 亿下载，6/6 PASS）。
- `packages/@cap/media-typer/` & `cap/test/testCapMediaTyper.cp`: npm `media-typer`（1.8 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/negotiator/` & `cap/test/testCapNegotiator.cp`: npm `negotiator`（1.5 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/ansi-regex/` & `cap/test/testCapAnsiRegex.cp`: npm `ansi-regex`（4 亿+/周，复用 `@cap/anstyle-parse`，6/6 PASS）。
- **历史旧包跨包依赖重构（零重复代码、100% 回归 PASS）**：
  - `packages/@cap/glob-parent/`：改为 `import` 复用 `@cap/is-extglob`（6/6 PASS）。
  - `packages/@cap/type-is/`：改为 `import` 复用 `@cap/media-typer`（6/6 PASS）。
  - `packages/@cap/accepts/`：改为 `import` 复用 `@cap/negotiator`（代码精简 70%，6/6 PASS）。
  - `packages/@cap/braces/`：改为 `import` 复用 `@cap/fill-range`（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **740 条**双生态高频映射（**npm=370 / crates.io=370 绝对 1:1 对称**）。

### 2026-11-27 (Batch 84 — **750 映射大里程碑：npm=375 / crates.io=375 绝对 1:1 对称！** + 3 个终端旧包统一解耦)
- `packages/@cap/half-f16/` & `cap/test/testCapHalfF16.cp`: IEEE 754 `f16` & Google `bf16` 半精度浮点（6/6 PASS）。
- `packages/@cap/unicode-bidi/` & `cap/test/testCapUnicodeBidi.cp`: crates.io #278 `unicode-bidi`（6.04 亿下载，6/6 PASS）。
- `packages/@cap/base64ct/` & `cap/test/testCapBase64ct.cp`: crates.io #299 `base64ct`（5.31 亿下载无查表恒定时间 Base64，6/6 PASS）。
- `packages/@cap/home-dir/` & `cap/test/testCapHomeDir.cp`: crates.io #305 `home`（5.18 亿下载）+ npm `untildify`/`expand-tilde`（6/6 PASS）。
- **历史终端旧包统一复用 `@cap/unicode-width`（18/18 零回归 PASS）**：
  - `packages/@cap/wrap-ansi/`、`packages/@cap/boxen/`、`packages/@cap/textwrap/` 全部删除内联重复宽度表，改为 `import { charWidth }` 复用 `@cap/unicode-width`！
- `packages/@cap/registry.json`: 正式突破 **750 条**双生态高频映射（**npm=375 / crates.io=375 绝对 1:1 对称**）。

### 2026-11-27 (Batch 85 — **760 映射 380/380 绝对对称！** 8 个重磅独立包全 6/6 PASS)
- `packages/@cap/proxy-addr/` & `cap/test/testCapProxyAddr.cp`: npm `proxy-addr`（1.5 亿+/周，6/6 PASS）。
- `packages/@cap/parseurl/` & `cap/test/testCapParseurl.cp`: npm `parseurl`（1.8 亿+/周，6/6 PASS）。
- `packages/@cap/escape-html/` & `cap/test/testCapEscapeHtml.cp`: npm `escape-html`（2 亿+/周，6/6 PASS）。
- `packages/@cap/cookie-signature/` & `cap/test/testCapCookieSignature.cp`: npm `cookie-signature`（1.2 亿+/周，复用 `@cap/base64ct`，6/6 PASS）。
- `packages/@cap/depd/` & `cap/test/testCapDepd.cp`: npm `depd`（1.5 亿+/周，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/quinn-varint/` & `cap/test/testCapQuinnVarint.cp`: crates.io #339 `quinn-proto`（4.59 亿下载，6/6 PASS）。
- `packages/@cap/subtle-ct/` & `cap/test/testCapSubtleCt.cp`: crates.io `subtle`（8 亿+）& `zeroize`（6 亿+，6/6 PASS）。
- `packages/@cap/dunce/` & `cap/test/testCapDunce.cp`: crates.io #351 `dunce`（4.33 亿下载，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **760 条**双生态高频映射（**npm=380 / crates.io=380 绝对 1:1 对称**）。

### 2026-11-27 (Batch 86 — **770 映射 385/385 绝对对称！** 8 个重磅独立包全 6/6 PASS，冲向 500 官方包)
- `packages/@cap/ucd-trie/` & `cap/test/testCapUcdTrie.cp`: crates.io #385 `ucd-trie`（3.82 亿下载，6/6 PASS）。
- `packages/@cap/cexpr/` & `cap/test/testCapCexpr.cp`: crates.io #403 `cexpr`（3.65 亿下载，6/6 PASS）。
- `packages/@cap/lru-slab/` & `cap/test/testCapLruSlab.cp`: crates.io #415 `lru-slab`（3.58 亿下载，6/6 PASS）。
- `packages/@cap/xorshift/` & `cap/test/testCapXorshift.cp`: crates.io #423 `rand_xorshift`（3.53 亿下载，6/6 PASS）。
- `packages/@cap/ring-queue/` & `cap/test/testCapRingQueue.cp`: crates.io #342 `crossbeam-queue`（4.55 亿下载，6/6 PASS）。
- `packages/@cap/form-urlencoded/` & `cap/test/testCapFormUrlencoded.cp`: crates.io `form_urlencoded`（11 亿+，6/6 PASS）。
- `packages/@cap/strip-bom/` & `cap/test/testCapStripBom.cp`: npm `strip-bom`（2.5 亿+/周，6/6 PASS）。
- `packages/@cap/parse-ms/` & `cap/test/testCapParseMs.cp`: npm `parse-ms`（1.2 亿+/周独立依赖，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **770 条**双生态高频映射（**npm=385 / crates.io=385 绝对 1:1 对称**）。

### 2026-11-27 (Batch 87 — **780 映射 390/390 绝对对称！** 9 新包 + 第 9 个历史旧包重构，正式跨过 **500 官方 `.cap` 包终极里程碑！**)
- `packages/@cap/kdtree-2d/` & `cap/test/testCapKdtree2d.cp`: 2D K-D 树最近邻与范围搜索（6/6 PASS）。
- `packages/@cap/ray-cast/` & `cap/test/testCapRayCast.cp`: 2D/3D 射线求交（复用 `@cap/libm`，6/6 PASS）。
- `packages/@cap/stringprep/` & `cap/test/testCapStringprep.cp`: crates.io `stringprep`（2.86 亿下载，6/6 PASS）。
- `packages/@cap/base16ct/` & `cap/test/testCapBase16ct.cp`: crates.io #452 `base16ct`（3.25 亿下载，6/6 PASS）。
- `packages/@cap/cargo-platform/` & `cap/test/testCapCargoPlatform.cp`: crates.io #500 `cargo-platform`（2.92 亿下载，6/6 PASS）。
- `packages/@cap/binary-extensions/` & `cap/test/testCapBinaryExtensions.cp`: npm `binary-extensions`（1.5 亿+/周，6/6 PASS）。
- `packages/@cap/pretty-bytes/` & `cap/test/testCapPrettyBytes.cp`: npm `pretty-bytes`（2 亿+/周，6/6 PASS）。
- `packages/@cap/strip-final-newline/` & `cap/test/testCapStripFinalNewline.cp`: npm `strip-final-newline`（2.5 亿+/周，6/6 PASS）。
- `packages/@cap/indent-string/` & `cap/test/testCapIndentString.cp`: npm `indent-string`（1.8 亿+/周，6/6 PASS）。
- `packages/@cap/pretty-ms/`: 第 9 个历史旧包跨包重构，真实 `import` 复用 `@cap/parse-ms`（6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **780 条**双生态高频映射（**npm=390 / crates.io=390 绝对 1:1 对称**）。

### 2026-11-27 (Batch 88 — **790 映射 395/395 绝对对称！** 6 个新包全部 6/6 PASS，官方包冲至 506+)
- `packages/@cap/is-binary-path/` & `cap/test/testCapIsBinaryPath.cp`: npm `is-binary-path`（复用 `@cap/binary-extensions`，6/6 PASS）。
- `packages/@cap/camelcase-keys/` & `cap/test/testCapCamelcaseKeys.cp`: npm `camelcase-keys`/`decamelize-keys`（复用 `@cap/camelcase`，6/6 PASS）。
- `packages/@cap/rfc6979/` & `cap/test/testCapRfc6979.cp`: crates.io #489 `rfc6979`（复用 `@cap/cookie-signature`，通过 RFC 6979 NIST P-256 官方向量，6/6 PASS）。
- `packages/@cap/ghash-gf128/` & `cap/test/testCapGhashGf128.cp`: crates.io `universal-hash`（通过 NIST SP 800-38D 官方向量，6/6 PASS）。
- `packages/@cap/hkdf-sha256/` & `cap/test/testCapHkdfSha256.cp`: crates.io `hkdf`（复用 `@cap/cookie-signature`，通过 RFC 5869 全 3 组官方向量，6/6 PASS）。
- `packages/@cap/atoi/` & `cap/test/testCapAtoi.cp`: crates.io `atoi`（2.62 亿下载，6/6 PASS）。
- `packages/@cap/registry.json`: 扩充至 **790 条**双生态高频映射（**npm=395 / crates.io=395 绝对 1:1 对称**）。

### 2026-11-27 (Batch 89 — **800 映射终极里程碑 400/400 绝对对称！** 9 个全新零堆包全部 6/6 PASS + 规则固化入 `GEMINI.md`)
- `GEMINI.md`: 固化第 9 条（子智能体 6~8 包轻量定时轮换防上下文膨胀与 Token 激增）与第 10 条（每波闭环交付、git push 及 LIVE 发布铁律）。
- `packages/@cap/pbkdf2-sha256/` & `cap/test/testCapPbkdf2Sha256.cp`: crates.io `pbkdf2`（跨包复用 `@cap/cookie-signature`，通过 RFC 7914 全 4 组官方向量，6/6 PASS）。
- `packages/@cap/iri-string/` & `cap/test/testCapIriString.cp`: crates.io #540 `iri-string`（2.65 亿下载，RFC 3986 §5.4 官方向量，6/6 PASS）。
- `packages/@cap/fast-deep-equal/` & `cap/test/testCapFastDeepEqual.cp`: npm `fast-deep-equal`（2 亿+/周，6/6 PASS）。
- `packages/@cap/poly1305/` & `cap/test/testCapPoly1305.cp`: crates.io #550 `poly1305`（跨包复用 `@cap/subtle-ct`，RFC 8439 §2.5.2 官方向量，6/6 PASS）。
- `packages/@cap/unicode-properties/` & `cap/test/testCapUnicodeProperties.cp`: crates.io #530 `unicode-properties`（2.69 亿下载，UAX #31/UTS #51，6/6 PASS）。
- `packages/@cap/chacha20-poly1305/` & `cap/test/testCapChacha20Poly1305.cp`: crates.io #520 `chacha20poly1305`（2.8 亿+，3 包组合复用 `@cap/poly1305`+`@cap/chacha-rng`+`@cap/subtle-ct`，通过 RFC 8439 §2.8.2 & XChaCha20 §2.2.1 官方向量，6/6 PASS）。
- `packages/@cap/slice-ansi/` & `cap/test/testCapSliceAnsi.cp`: npm Top 100 `slice-ansi`（1.2 亿+/周，跨包复用 `@cap/anstyle-parse`+`@cap/unicode-width`，6/6 PASS）。
- `packages/@cap/has-flag/` & `cap/test/testCapHasFlag.cp`: npm Top 50 `has-flag`（2.5 亿+/周，6/6 PASS）。
- `packages/@cap/toidentifier/` & `cap/test/testCapToidentifier.cp`: npm Top 80 `toidentifier`（9000 万+/周，6/6 PASS）。
- `packages/@cap/registry.json`: 正式突破 **800 条**双生态高频映射（**npm=400 / crates.io=400 绝对 1:1 对称**，官方 `.cap` 包 515+）。
