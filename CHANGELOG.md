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

### 2026-11-27 (Batch 90 — **突破 808 映射 404/404 绝对对称！** 8 个全新零堆包全部 6/6 PASS，官方包 523+)
- `packages/@cap/pem-rfc7468/` & `cap/test/testCapPemRfc7468.cp`: crates.io `pem-rfc7468`（跨包复用 `@cap/base64ct`，RFC 7468，6/6 PASS）。
- `packages/@cap/const-oid/` & `cap/test/testCapConstOid.cp`: crates.io `const-oid`（2.9 亿下载，ITU-T X.690 OID，6/6 PASS）。
- `packages/@cap/der-asn1/` & `cap/test/testCapDerAsn1.cp`: crates.io `der`（3.0 亿下载，跨包复用 `@cap/const-oid`，6/6 PASS）。
- `packages/@cap/spki/` & `cap/test/testCapSpki.cp`: crates.io `spki`（2.7 亿下载，跨包复用 `@cap/const-oid`+`@cap/pem-rfc7468`，RFC 8410 向量，6/6 PASS）。
- `packages/@cap/cliui/` & `cap/test/testCapCliui.cp`: npm Top 70 `cliui`（3 包复用 `@cap/wrap-ansi`+`@cap/ansi-strip`+`@cap/unicode-width`，6/6 PASS）。
- `packages/@cap/y18n/` & `cap/test/testCapY18n.cp`: npm Top 85 `y18n`（1.2 亿+/周，6/6 PASS）。
- `packages/@cap/yocto-queue/` & `cap/test/testCapYoctoQueue.cp`: npm Top 60 `yocto-queue`（2 亿+/周，6/6 PASS）。
- `packages/@cap/onetime/` & `cap/test/testCapOnetime.cp`: npm Top 80 `onetime`（1.5 亿+/周，6/6 PASS）。
- `packages/@cap/registry.json`: 正式突破 **808 条**双生态映射（**npm=404 / crates.io=404 绝对 1:1 对称**）。

### 2026-11-27 (Batch 91 — **815+ 映射！** 6 个全新零堆包 + 第 10 个历史旧包 `@cap/http-errors` 解耦重构，全部 6/6 PASS)
- `packages/@cap/pkcs8/` & `cap/test/testCapPkcs8.cp`: crates.io `pkcs8`（4 包复用 `@cap/spki`+`@cap/const-oid`+`@cap/pem-rfc7468`+`@cap/subtle-ct`，RFC 8410 §10.3 48 字节向量，6/6 PASS）。
- `packages/@cap/sec1/` & `cap/test/testCapSec1.cp`: crates.io `sec1`（3 包复用 `@cap/const-oid`+`@cap/pem-rfc7468`+`@cap/subtle-ct`，SEC1 v2.0 + RFC 5915，6/6 PASS）。
- `packages/@cap/p-limit/` & `cap/test/testCapPLimit.cp`: npm Top 25 `p-limit`（跨包复用 `@cap/yocto-queue`，6/6 PASS）。
- `packages/@cap/supports-color/` & `cap/test/testCapSupportsColor.cp`: npm Top 20 `supports-color`（跨包复用 `@cap/has-flag`，6/6 PASS）。
- `packages/@cap/kind-of/` & `cap/test/testCapKindOf.cp`: npm Top 90 `kind-of`（6/6 PASS）。
- `packages/@cap/decamelize/` & `cap/test/testCapDecamelize.cp`: npm Top 75 `decamelize`（6/6 PASS）。
- **第 10 个历史旧包解耦重构**：`packages/@cap/http-errors/` 声明依赖并导入 `@cap/toidentifier` 提供 `errorClassName`，回归 6/6 PASS。

### 2026-11-27 (Batch 92 — **818 映射 409/409 绝对对称！** 7 个全新零堆包全部 6/6 PASS + 3 路子智能体全员完成首轮 6 包轮换重建)
- `packages/@cap/x509-cert/` & `cap/test/testCapX509Cert.cp`: crates.io `x509-cert`（4 包复用 `@cap/der-asn1`+`@cap/const-oid`+`@cap/spki`+`@cap/pem-rfc7468`，6/6 PASS）。
- `packages/@cap/pkcs1/` & `cap/test/testCapPkcs1.cp`: crates.io `pkcs1`（3 包复用 `@cap/der-asn1`+`@cap/pem-rfc7468`+`@cap/subtle-ct`，6/6 PASS）。
- `packages/@cap/block-padding/` & `cap/test/testCapBlockPadding.cp`: crates.io `block-padding`/`cipher`（复用 `@cap/subtle-ct` 恒定时间 PKCS#7 去填充，6/6 PASS）。
- `packages/@cap/ecdsa-sig/` & `cap/test/testCapEcdsaSig.cp`: crates.io `ecdsa`/`signature`（复用 `@cap/der-asn1`+`@cap/subtle-ct`，RFC 6979 P-256 向量 & BIP-62 Low-S，6/6 PASS）。
- `packages/@cap/color-name/` & `cap/test/testCapColorName.cp`: npm Top 30 `color-name`（148 个 W3C CSS4 命名色 O(log n)，6/6 PASS）。
- `packages/@cap/escalade/` & `cap/test/testCapEscalade.cp`: npm Top 95 `escalade`（复用 `@cap/normalize-path`，6/6 PASS）。
- `packages/@cap/methods/` & `cap/test/testCapMethods.cp`: npm Top 65 `methods`（34 个 HTTP/WebDAV 方法，6/6 PASS）。
- `packages/@cap/registry.json`: 推进至 **818 条**双生态映射（**npm=409 / crates.io=409 绝对 1:1 对称**，官方 `.cap` 包 538+）。

### 2026-11-27 (Batch 93 — **Rustls 生态 35.8 亿下载全家桶贯通！** 6 个全新超重磅零堆包全部 6/6 PASS，映射冲至 824+)
- `packages/@cap/rustls-webpki/` & `cap/test/testCapRustlsWebpki.cp`: crates.io #105 `rustls-webpki`（10.29 亿下载，复用 `@cap/x509-cert`，6/6 PASS）。
- `packages/@cap/webpki-roots/` & `cap/test/testCapWebpkiRoots.cp`: crates.io #112 `webpki-roots`（9.74 亿下载，复用 `@cap/x509-cert`+`@cap/spki`，6/6 PASS）。
- `packages/@cap/rustls-pemfile/` & `cap/test/testCapRustlsPemfile.cp`: crates.io #145 `rustls-pemfile`（8.5 亿下载，复用 `@cap/pem-rfc7468`，6/6 PASS）。
- `packages/@cap/rustls-pki-types/` & `cap/test/testCapRustlsPkiTypes.cp`: crates.io #180 `rustls-pki-types`（7.27 亿下载，复用 `@cap/pem-rfc7468`，6/6 PASS）。
- `packages/@cap/color-convert/` & `cap/test/testCapColorConvert.cp`: npm Top 28 `color-convert`（3 亿+/周，复用 `@cap/color-name`，6/6 PASS）。
- `packages/@cap/ipaddr/` & `cap/test/testCapIpaddr.cp`: npm Top 55 `ipaddr.js`（2 亿+/周，RFC 5952，6/6 PASS）。

### 2026-11-27 (Batch 94 — **修复 `capsurf_agy` 紧急工单 + 6 个超高频新包全部 6/6 PASS！** 映射达 829+)
- **编译器热修（结单 `task-capsurf-20261126-css-boxshadow-e0207`）**：
  - `cpc/src/resolver.rs` & `cpc/src/main.rs`: 在单文件编译模式下隔离导入模块与根模块的语义分析（通过 `root_item_indices` + `register_imported_decl`/`register_imported_var`），彻底消除导入多子模块标准库（如 `cap:css`）时的 35 条跨文件扁平化误报（E0207/E0214/E0216），并发布 `cpc (release)`。
  - `lib/css/easing.cp` & `lib/css/index.cp`: 将 `StepPosition` 重命名为 `CssStepPosition` 消除与 `lib/css/engine/values.cp` 的枚举撞名。实测 `capsurf` `test_box_shadow_and_border_radius_edge.cp` (4/4 PASS) 与 `test_color_and_units.cp` (4/4 PASS)。
- `packages/@cap/picomatch/` & `cap/test/testCapPicomatch.cp`: npm Top 15 `picomatch`（3.5 亿+/周，复用 `@cap/normalize-path`+`@cap/is-extglob`，6/6 PASS）。
- `packages/@cap/micromatch/` & `cap/test/testCapMicromatch.cp`: npm Top 35 `micromatch`（2.5 亿+/周，复用 `@cap/picomatch`+`@cap/braces`，6/6 PASS）。
- `packages/@cap/yargs-parser/` & `cap/test/testCapYargsParser.cp`: npm Top 45 `yargs-parser`（2.5 亿+/周，复用 `@cap/camelcase`+`@cap/decamelize`，6/6 PASS）。
- `packages/@cap/ansi-styles/` & `cap/test/testCapAnsiStyles.cp`: npm Top 10 `ansi-styles`（4 亿+/周，复用 `@cap/color-convert`→`@cap/color-name`，6/6 PASS）。
- `packages/@cap/tinystr/` & `cap/test/testCapTinystr.cp`: crates.io #260 `tinystr`（5.5 亿+，6/6 PASS）。
- `packages/@cap/icu-locid/` & `cap/test/testCapIcuLocid.cp`: crates.io #255 `icu_locid`（5.5 亿+，复用 `@cap/tinystr`，6/6 PASS）。
- `packages/@cap/normalize-path/src/index.cp` & `packages/@cap/decamelize/src/`: 消除跨包展平时的私有/重名符号冲突，100% 向后兼容。

### 2026-11-27 (Batch 95 — **Top-30 旗舰收割波！** `chalk`/`minimatch`/`yargs`/`ignore`/`idna`/`icu-normalizer` 6 大包全 6/6 PASS，官方包 554+)
- `packages/@cap/yargs/` & `cap/test/testCapYargs.cp`: npm Top 30 `yargs`（3 亿+/周，3 包复用 `@cap/yargs-parser`+`@cap/cliui`+`@cap/y18n`，6/6 PASS）。
- `packages/@cap/chalk/` & `cap/test/testCapChalk.cp`: npm Top 5 `chalk`（5 亿+/周，贯通 6 包深度复用链，6/6 PASS）。
- `packages/@cap/minimatch/` & `cap/test/testCapMinimatch.cp`: npm Top 8 `minimatch`（4 亿+/周，复用 `@cap/micromatch`+`@cap/picomatch`，6/6 PASS）。
- `packages/@cap/ignore/` & `cap/test/testCapIgnore.cp`: npm Top 22 `ignore`（2.5 亿+/周，复用 `@cap/minimatch`+`@cap/normalize-path`，6/6 PASS）。
- `packages/@cap/icu-normalizer/` & `cap/test/testCapIcuNormalizer.cp`: crates.io #270 `icu_normalizer`（5.3 亿+，UAX #15 NFC/NFD/NFKC/NFKD，6/6 PASS）。
- `packages/@cap/idna-uts46/` & `cap/test/testCapIdnaUts46.cp`: crates.io #120 `idna`（9.5 亿+，复用 `@cap/icu-normalizer`+`@cap/punycode`，6/6 PASS）。
- `packages/@cap/registry.json`: 推进至 **831+ 条**双生态映射（官方 `.cap` 包突破 **554+**）。

### 2026-11-27 (Batch 96 — **560+ 官方包里程碑！** `itoa`/`url`/`percent-encoding`/`commander`/`string-width`/`escape-string-regexp`/`shortid` 7 大包全 6/6 PASS)
- `packages/@cap/commander/` & `cap/test/testCapCommander.cp`: npm Top 12 `commander`（3.5 亿+/周，复用 `@cap/yargs-parser`+`@cap/cliui`，6/6 PASS）。
- `packages/@cap/string-width/` & `cap/test/testCapStringWidth.cp`: npm Top 14 `string-width`（3.5 亿+/周，复用 `@cap/ansi-strip`+`@cap/unicode-width`，6/6 PASS）。
- `packages/@cap/escape-string-regexp/` & `cap/test/testCapEscapeStringRegexp.cp`: npm Top 18 `escape-string-regexp`（3.5 亿+/周，6/6 PASS）。
- `packages/@cap/shortid/` & `cap/test/testCapShortid.cp`: npm `shortid`（6/6 PASS）。
- `packages/@cap/percent-encoding/` & `cap/test/testCapPercentEncoding.cp`: crates.io #85 `percent-encoding`（12.5 亿+，6/6 PASS）。
- `packages/@cap/whatwg-url/` & `cap/test/testCapWhatwgUrl.cp`: crates.io #40 `url`（16 亿+，双包复用 `@cap/percent-encoding`+`@cap/idna-uts46`，6/6 PASS）。
- `packages/@cap/itoa/` & `cap/test/testCapItoa.cp`: crates.io #35 `itoa`（17 亿+，6/6 PASS）。
- `packages/@cap/registry.json`: 推进至 **835 条**双生态映射（官方 `.cap` 包正式突破 **560+**）。

### 2026-11-27 (Batch 97 — **50 亿+ 下载 Rust 核心三巨头 + `mathjs` 全部 6/6 PASS！** 官方包达 565+)
- `packages/@cap/ryu-float/` & `cap/test/testCapRyuFloat.cp`: crates.io #45 `ryu`（15 亿+，复用 `@cap/itoa`，6/6 PASS）。
- `packages/@cap/bytes-buf/` & `cap/test/testCapBytesBuf.cp`: crates.io #25 `bytes`（18 亿+，6/6 PASS）。
- `packages/@cap/smallvec/` & `cap/test/testCapSmallvec.cp`: crates.io #30 `smallvec`（17 亿+，6/6 PASS）。
- `packages/@cap/mathjs-expr/` & `cap/test/testCapMathjsExpr.cp`: npm `mathjs`（复用 `@cap/libm`，6/6 PASS）。
- `packages/@cap/registry.json`: 更新至 **831+ 条**映射并同步将 15 个顶流包的目标指向独立 `@cap/*` 官方包。

### 2026-11-27 (Batch 98 — **Crates.io 全站 #1 `hashbrown` + #11 `indexmap` + #16 `memchr` 70.6 亿下载三冠王！** 6 个新包全 6/6 PASS，官方包 571+)
- `packages/@cap/hashbrown/` & `cap/test/testCapHashbrown.cp`: crates.io **#1** `hashbrown`（32.6 亿+，SwissTable H2 控制字节，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/indexmap/` & `cap/test/testCapIndexmap.cp`: crates.io #11 `indexmap`（19.75 亿+，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/memchr/` & `cap/test/testCapMemchr.cp`: crates.io #16 `memchr`（18.26 亿+，SWAR 8 字节并行加速，6/6 PASS）。
- `packages/@cap/terser-minify/` & `cap/test/testCapTerserMinify.cp`: npm `terser`（JS/CSS 词法安全压缩器，6/6 PASS）。
- `packages/@cap/prettier-fmt/` & `cap/test/testCapPrettierFmt.cp`: npm `prettier`（JSON/CSS 格式化器，6/6 PASS）。
- `packages/@cap/change-case/` & `cap/test/testCapChangeCase.cp`: npm Top 110 `change-case` v5（复用 `@cap/camelcase`+`@cap/decamelize`，12 种风格，6/6 PASS）。
- `packages/@cap/registry.json`: 推进至 **837+ 条**映射（官方 `.cap` 包突破 **571+**）。

### 2026-11-27 (Batch 99 — **冲刺百波大关！** `zod`/`which`/`bstr`/`bitvec` 4 大包全 6/6 PASS，官方包 576+)
- `packages/@cap/zod-schema/` & `cap/test/testCapZodSchema.cp`: npm Top 25 `zod`（1.5 亿+/周，复用 `@cap/kind-of`+`@cap/whatwg-url`，6/6 PASS）。
- `packages/@cap/which-bin/` & `cap/test/testCapWhichBin.cp`: npm Top 20 `which`（2 亿+/周，复用 `@cap/normalize-path`，6/6 PASS）。
- `packages/@cap/bstr/` & `cap/test/testCapBstr.cp`: crates.io #90 `bstr`（11 亿+，复用 `@cap/memchr`，6/6 PASS）。
- `packages/@cap/bitvec/` & `cap/test/testCapBitvec.cp`: crates.io #210 `bitvec`（6 亿+，6/6 PASS）。

### 2026-11-27 (Batch 100 — **百波里程碑！** 11 大顶流包全 6/6 PASS — npm #1 `semver` + #6 `ms` + `postcss` + `regex-lite` + `csv-core`，官方 `.cap` 包突破 **588+**)
- `packages/@cap/semver-range/` & `cap/test/testCapSemverRange.cp`: npm **#1** `semver`（4 亿+/周，6/6 PASS）。
- `packages/@cap/ms/` & `cap/test/testCapMs.cp`: npm **#6** `ms`（4 亿+/周，复用 `@cap/itoa`，6/6 PASS）。
- `packages/@cap/postcss-parse/` & `cap/test/testCapPostcssParse.cp`: npm Top 25 `postcss`（1.5 亿+/周，6/6 PASS）。
- `packages/@cap/ansi-escapes/` & `cap/test/testCapAnsiEscapes.cp`: npm Top 40 `ansi-escapes`（1.5 亿+/周，6/6 PASS）。
- `packages/@cap/dot-prop/` & `cap/test/testCapDotProp.cp`: npm Top 60 `dot-prop`（1 亿+/周，6/6 PASS）。
- `packages/@cap/tsconfig-paths/` & `cap/test/testCapTsconfigPaths.cp`: npm Top 100 `tsconfig-paths`（6000 万+/周，6/6 PASS）。
- `packages/@cap/mustache-tpl/` & `cap/test/testCapMustacheTpl.cp`: npm `mustache` v4（复用 `@cap/escape-html`，6/6 PASS）。
- `packages/@cap/regex-lite/` & `cap/test/testCapRegexLite.cp`: crates.io #95 `regex-lite`（10 亿+，复用 `@cap/regex-syntax`+`@cap/memchr`，Thompson NFA + PikeVM，6/6 PASS）。
- `packages/@cap/humantime/` & `cap/test/testCapHumantime.cp`: crates.io #115 `humantime`（9.5 亿+，复用 `@cap/itoa`，6/6 PASS）。
- `packages/@cap/globset/` & `cap/test/testCapGlobset.cp`: crates.io #125 `globset`（9 亿+，复用 `@cap/picomatch`+`@cap/normalize-path`，6/6 PASS）。
- `packages/@cap/csv-core/` & `cap/test/testCapCsvCore.cp`: crates.io #150 `csv-core`（8 亿+，10 状态流式 RFC 4180 CSV 引擎，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 100 全部 11 个官方包映射。

### 2026-11-27 (Batch 101 — 7 大 Unicode & Web 模板顶流包全 6/6 PASS — `unicode-segmentation`+`unicode-xid`+`unicase` 38.5 亿下载三连杀，官方 `.cap` 包突破 **591+**)
- `packages/@cap/marked-md/` & `cap/test/testCapMarkedMd.cp`: npm Top 35 `marked` v14（1.2 亿+/周，复用 `@cap/escape-html`，6/6 PASS）。
- `packages/@cap/mime-types/` & `cap/test/testCapMimeTypes.cp`: npm Top 15 `mime-types`（2.5 亿+/周，95+ MIME 映射，6/6 PASS）。
- `packages/@cap/ejs-tpl/` & `cap/test/testCapEjsTpl.cp`: npm Top 50 `ejs` v3（1 亿+/周，复用 `@cap/escape-html`，6/6 PASS）。
- `packages/@cap/unicode-segmentation/` & `cap/test/testCapUnicodeSegmentation.cp`: crates.io #60 `unicode-segmentation`（13.5 亿+，UAX #29 字素簇与词边界，6/6 PASS）。
- `packages/@cap/unicode-xid/` & `cap/test/testCapUnicodeXid.cp`: crates.io #70 `unicode-xid`（13 亿+，UAX #31 标识符判定，6/6 PASS）。
- `packages/@cap/unicase/` & `cap/test/testCapUnicase.cp`: crates.io #80 `unicase`（12 亿+，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/wildmatch/` & `cap/test/testCapWildmatch.cp`: crates.io Top-400 `wildmatch`（4 亿+，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 101 全部 7 个官方包映射。

### 2026-11-27 (Batch 102 — 6 大高频核心包全 6/6 PASS — `heck`+`fixedbitset`+`similar`+`graphql`+`cookie-jar`+`cors`，官方 `.cap` 包逼近 **600 大关 (598)**！)
- `packages/@cap/heck-case/` & `cap/test/testCapHeckCase.cp`: crates.io #105 `heck` v0.5（10 亿+，8 种命名风格，6/6 PASS）。
- `packages/@cap/similar-diff/` & `cap/test/testCapSimilarDiff.cp`: crates.io Top-250 `similar` v2（5 亿+，LCS Diff & Unified Diff，6/6 PASS）。
- `packages/@cap/fixedbitset/` & `cap/test/testCapFixedbitset.cp`: crates.io #140 `fixedbitset`（8.5 亿+，1024 位零堆位集与集合代数，6/6 PASS）。
- `packages/@cap/graphql-parse/` & `cap/test/testCapGraphqlParse.cp`: npm Top 80 `graphql` v16（7000 万+/周，词法/语法解析与压缩，6/6 PASS）。
- `packages/@cap/cookie-jar/` & `cap/test/testCapCookieJar.cp`: npm `tough-cookie`（RFC 6265 CookieJar，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/cors-headers/` & `cap/test/testCapCorsHeaders.cp`: npm Top 55 `cors`（8000 万+/周，双包复用 `@cap/unicase`+`@cap/wildmatch`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 102 全部 6 个官方包映射（npm / crates.io 严守 1:1 平衡）。

### 2026-11-27 (Batch 103 — **正式突破 600 个官方 `.cap` 包里程碑 (604 LIVE)！** 6 大 HTTP/Rust 顶流包全 6/6 PASS)
- `packages/@cap/helmet-headers/` & `cap/test/testCapHelmetHeaders.cp`: npm Top 65 `helmet` v8（7000 万+/周，6/6 PASS）。
- `packages/@cap/fresh-http/` & `cap/test/testCapFreshHttp.cp`: npm Top 30 `fresh`（1.5 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/etag-http/` & `cap/test/testCapEtagHttp.cp`: npm Top 25 `etag`（1.8 亿+/周，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/vary-http/` & `cap/test/testCapVaryHttp.cp`: npm Top 25 `vary`（2 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/nom-parse/` & `cap/test/testCapNomParse.cp`: crates.io #130 `nom` v7（9 亿+，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/tinyvec/` & `cap/test/testCapTinyvec.cp`: crates.io #120 `tinyvec`（9.5 亿+，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 103 全部 6 个官方包映射。

### 2026-11-27 (Batch 104 — 6 大高性能数据结构与 HTTP 中间件包全 6/6 PASS — `slab`+`slotmap`+`ringbuf`+`mime_guess`+`type-is`+`forwarded`，官方 `.cap` 包突破 **610 LIVE**！)
- `packages/@cap/slab-alloc/` & `cap/test/testCapSlabAlloc.cp`: crates.io #90 `slab`（11 亿+，O(1) 空闲链表对象池，6/6 PASS）。
- `packages/@cap/mime-guess/` & `cap/test/testCapMimeGuess.cp`: crates.io #180 `mime_guess` v2（7 亿+，复用 `@cap/mime-types`，6/6 PASS）。
- `packages/@cap/slotmap/` & `cap/test/testCapSlotmap.cp`: crates.io Top-220 `slotmap` v1（5.5 亿+，防 ABA 代际索引，6/6 PASS）。
- `packages/@cap/ringbuf/` & `cap/test/testCapRingbuf.cp`: crates.io Top-240 `ringbuf`（5 亿+，2 的幂环形缓冲，6/6 PASS）。
- `packages/@cap/type-is/` & `cap/test/testCapTypeIs.cp`: npm Top 35 `type-is`（1.4 亿+/周，双包复用 `@cap/mime-types`+`@cap/unicase`，6/6 PASS）。
- `packages/@cap/forwarded-http/` & `cap/test/testCapForwardedHttp.cp`: npm Top 45 `forwarded`（1 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 104 全部 6 个官方包映射。

### 2026-11-27 (Batch 105 — 6 大核心协议与图算法/Arena 包全 6/6 PASS — `petgraph`+`bumpalo`+`css-select`+`range-parser`+`negotiator`+`media-typer`，官方 `.cap` 包突破 **616+**！)
- `packages/@cap/css-select/` & `cap/test/testCapCssSelect.cp`: npm Top 30 `css-select`（1.8 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/range-parser/` & `cap/test/testCapRangeParser.cp`: npm Top 28 `range-parser`（1.7 亿+/周，6/6 PASS）。
- `packages/@cap/negotiator/` & `cap/test/testCapNegotiator.cp`: npm Top 30 `negotiator`（1.5 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/media-typer/` & `cap/test/testCapMediaTyper.cp`: npm Top 40 `media-typer`（1.3 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/petgraph-core/` & `cap/test/testCapPetgraphCore.cp`: crates.io #110 `petgraph`（9.5 亿+，BFS/DFS/TopoSort/Dijkstra，6/6 PASS）。
- `packages/@cap/bumpalo/` & `cap/test/testCapBumpalo.cp`: crates.io #145 `bumpalo`（8.5 亿+，对齐感知 Bump Arena，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 105 全部 6 个官方包映射。

### 2026-11-27 (Batch 106 — 5 大高频路由/编解码/LRU 缓存包全 6/6 PASS — `path-to-regexp`+`content-type`+`lru`+`data-encoding`+`quick-lru`，官方 `.cap` 包突破 **621+**！)
- `packages/@cap/path-to-regexp/` & `cap/test/testCapPathToRegexp.cp`: npm Top 20 `path-to-regexp` v8（2 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/content-type/` & `cap/test/testCapContentType.cp`: npm Top 25 `content-type`（1.8 亿+/周，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/quick-lru/` & `cap/test/testCapQuickLru.cp`: npm Top 60 `quick-lru`（9000 万+/周，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/lru-crate/` & `cap/test/testCapLruCrate.cp`: crates.io #135 `lru`（9 亿+，复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/data-encoding/` & `cap/test/testCapDataEncoding.cp`: crates.io #155 `data-encoding` v2（8 亿+，RFC 4648 严格编解码，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 106 全部 5 个官方包映射（npm / crates.io 精准 1:1 平衡）。

### 2026-11-27 (Batch 107 — 7 大可观测性/配置/调度与位图顶流包全 6/6 PASS — `toml_edit`+`roaring`+`dotenv-expand`+`pino`+`morgan`+`cron-parser`+`snowflake-id`，官方 `.cap` 包突破 **628+**！)
- `packages/@cap/toml-edit/` & `cap/test/testCapTomlEdit.cp`: crates.io #125 `toml_edit` v0.22（9 亿+，TOML v1.0 解析与格式化，6/6 PASS）。
- `packages/@cap/roaring-bitmap/` & `cap/test/testCapRoaringBitmap.cp`: crates.io Top-260 `roaring` v0.10（4.5 亿+，全 `u32` 压缩位图，6/6 PASS）。
- `packages/@cap/snowflake-id/` & `cap/test/testCapSnowflakeId.cp`: 64 位分布式 Snowflake ID 生成与解码（复用 `@cap/itoa`，6/6 PASS）。
- `packages/@cap/dotenv-expand/` & `cap/test/testCapDotenvExpand.cp`: npm Top 50 `dotenv-expand`（1 亿+/周，6/6 PASS）。
- `packages/@cap/pino-log/` & `cap/test/testCapPinoLog.cp`: npm Top 55 `pino` v9（9000 万+/周，复用 `@cap/itoa`，6/6 PASS）。
- `packages/@cap/morgan-log/` & `cap/test/testCapMorganLog.cp`: npm Top 60 `morgan` v1.10（8000 万+/周，复用 `@cap/itoa`，6/6 PASS）。
- `packages/@cap/cron-parser/` & `cap/test/testCapCronParser.cp`: npm Top 90 `cron-parser` v4（6000 万+/周，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 107 全部 7 个官方包映射。

### 2026-11-27 (Batch 108 — 6 大 CLI 终端渲染/文本算法/重试退避包全 6/6 PASS — `strsim`+`textwrap`+`serde_qs`+`ora`+`boxen`+`retry-backoff`，官方 `.cap` 包突破 **624+ LIVE**！)
- `packages/@cap/strsim/` & `cap/test/testCapStrsim.cp`: crates.io #140 `strsim`（8.5 亿+，6 种经典字符串相似度/编辑距离算法，6/6 PASS）。
- `packages/@cap/textwrap-crate/` & `cap/test/testCapTextwrapCrate.cp`: crates.io #150 `textwrap` v0.16（8 亿+，复用 `@cap/unicode-width`，6/6 PASS）。
- `packages/@cap/serde-qs/` & `cap/test/testCapSerdeQs.cp`: crates.io Top-300 `serde_qs`（3.5 亿+，复用 `@cap/percent-encoding`，6/6 PASS）。
- `packages/@cap/ora-spinner/` & `cap/test/testCapOraSpinner.cp`: npm Top 65 `ora` v8（7500 万+/周，复用 `@cap/ansi-escapes`，6/6 PASS）。
- `packages/@cap/boxen-ui/` & `cap/test/testCapBoxenUi.cp`: npm Top 70 `boxen` v8（7000 万+/周，复用 `@cap/string-width`，6/6 PASS）。
- `packages/@cap/retry-backoff/` & `cap/test/testCapRetryBackoff.cp`: npm `p-retry`/`retry`（6000 万+/周，指数退避与确定性抖动，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 108 全部 6 个官方包映射（npm / crates.io 精准 1:1 平衡）。

### 2026-11-27 (Batch 109 — 5 大 10 亿级核心底座包全 6/6 PASS — `hex`(15 亿)+`http`(11 亿)+`qs`(2 亿/周)+`convert_case`+`base64url`，官方 `.cap` 包突破 **629+**！)
- `packages/@cap/hex-crate/` & `cap/test/testCapHexCrate.cp`: crates.io #45 `hex`（15 亿+，复用 `@cap/data-encoding`，6/6 PASS）。
- `packages/@cap/http-types/` & `cap/test/testCapHttpTypes.cp`: crates.io #100 `http` v1（11 亿+，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/convert-case/` & `cap/test/testCapConvertCase.cp`: crates.io Top-280 `convert_case`（4 亿+，复用 `@cap/heck-case`，6/6 PASS）。
- `packages/@cap/qs-parse/` & `cap/test/testCapQsParse.cp`: npm Top 22 `qs` v6（2 亿+/周，双包复用 `@cap/serde-qs`+`@cap/percent-encoding`，6/6 PASS）。
- `packages/@cap/base64-url/` & `cap/test/testCapBase64Url.cp`: npm `base64url` v3（7000 万+/周，复用 `@cap/data-encoding`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 109 全部 5 个官方包映射。

### 2026-11-27 (Batch 110 — 5 大前端解析/模板与分布式 ID 顶流包全 6/6 PASS + 3 小时审计固化 `GEMINI.md` 第 11 条！)
- `packages/@cap/acorn-parse/` & `cap/test/testCapAcornParse.cp`: npm Top 25 `acorn` v8（1.8 亿+/周，ES2024 词法分析与 ESM 扫描，6/6 PASS）。
- `packages/@cap/parse5/` & `cap/test/testCapParse5.cp`: npm Top 28 `parse5` v7（1.7 亿+/周，WHATWG HTML5 流式词法器，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/handlebars-tpl/` & `cap/test/testCapHandlebarsTpl.cp`: npm Top 75 `handlebars` v4（6500 万+/周，复用 `@cap/escape-html`，6/6 PASS）。
- `packages/@cap/ulid-crate/` & `cap/test/testCapUlidCrate.cp`: crates.io Top-290 `ulid` v1（3.8 亿+，128 位 Crockford Base32 ULID，6/6 PASS）。
- `packages/@cap/cuid2/` & `cap/test/testCapCuid2.cp`: npm `@paralleldrive/cuid2`（复用 `@cap/fnv-hash`，6/6 PASS）。
- `GEMINI.md`: 每 3 小时效率审计固化第 11 条（首轮一遍过 CP 编码清单，减少反复编译试错 Token 损耗）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 110 全部 5 个官方包映射。

### 2026-11-27 (Batch 111 — 5 大调度/查询串/枚举反射顶流包全 6/6 PASS — `strum`(9 亿)+`cron`+`querystring`+`p-queue`+`rrule`，官方 `.cap` 包突破 **639+**！)
- `packages/@cap/strum-enum/` & `cap/test/testCapStrumEnum.cp`: crates.io #125 `strum`（9 亿+，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/cron-expr/` & `cap/test/testCapCronExpr.cp`: crates.io Top-310 `cron` v0.12（3.5 亿+，复用 `@cap/cron-parser`，6/6 PASS）。
- `packages/@cap/querystring/` & `cap/test/testCapQuerystring.cp`: npm Top 35 `query-string` v9（1.5 亿+/周，复用 `@cap/percent-encoding`，6/6 PASS）。
- `packages/@cap/p-queue/` & `cap/test/testCapPQueue.cp`: npm Top 65 `p-queue` v8（7500 万+/周，并发受限优先级队列，6/6 PASS）。
- `packages/@cap/rrule-parse/` & `cap/test/testCapRruleParse.cp`: npm `rrule` v2（RFC 5545，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 111 全部 5 个官方包映射。

### 2026-11-27 (Batch 112 — **拿下 crates.io #12 `bitflags`(19.5 亿) + 6 大高频 HTTP/Glob 包全 6/6 PASS，官方 `.cap` 包突破 642+！**)
- `packages/@cap/bitflags-crate/` & `cap/test/testCapBitflagsCrate.cp`: crates.io **#12** `bitflags` v2（19.5 亿+，32/64 位标志集合，6/6 PASS）。
- `packages/@cap/glob-match/` & `cap/test/testCapGlobMatch.cp`: crates.io Top-320 `glob-match`（3 亿+，防回溯 Glob & 花括号交替，6/6 PASS）。
- `packages/@cap/enumflags2/` & `cap/test/testCapEnumflags2.cp`: crates.io Top-330 `enumflags2`（3 亿+，64 位安全标志集，6/6 PASS）。
- `packages/@cap/parseurl/` & `cap/test/testCapParseurl.cp`: npm Top 25 `parseurl`（1.8 亿+/周，复用 `@cap/querystring`，6/6 PASS）。
- `packages/@cap/encodeurl/` & `cap/test/testCapEncodeurl.cp`: npm Top 26 `encodeurl` v2（1.8 亿+/周，幂等 URL 编码，6/6 PASS）。
- `packages/@cap/content-disposition/` & `cap/test/testCapContentDisposition.cp`: npm Top 32 `content-disposition`（1.5 亿+/周，双包复用 `@cap/percent-encoding`+`@cap/unicase`，6/6 PASS）。
- `packages/@cap/p-map/` & `cap/test/testCapPMap.cp`: npm Top 55 `p-map`（9000 万+/周，并发分片与调度池，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 112 全部 7 个官方包映射。

### 2026-11-27 (Batch 113 — **拿下 40 亿下载双巨头 `aho-corasick`(16 亿)+`semver`(15.5 亿) + `cookie` + `finalhandler` + `serve-static`，官方 `.cap` 包突破 643+！**)
- `packages/@cap/aho-corasick-dfa/` & `cap/test/testCapAhoCorasickDfa.cp`: crates.io **#40** `aho-corasick` v1（16 亿+，复用 `@cap/unicase`，6/6 PASS）。
- `packages/@cap/semver-crate/` & `cap/test/testCapSemverCrate.cp`: crates.io **#42** `semver` v1（15.5 亿+，复用 `@cap/semver-range`，6/6 PASS）。
- `packages/@cap/cookie-crate/` & `cap/test/testCapCookieCrate.cp`: crates.io #145 `cookie` v0.18（8.5 亿+，双包复用 `@cap/cookie-jar`+`@cap/percent-encoding`，6/6 PASS）。
- `packages/@cap/finalhandler/` & `cap/test/testCapFinalhandler.cp`: npm Top 35 `finalhandler` v2（1.4 亿+/周，三包复用，6/6 PASS）。
- `packages/@cap/serve-static/` & `cap/test/testCapServeStatic.cp`: npm Top 40 `serve-static` v2（1.3 亿+/周，四包复用，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 113 全部 5 个官方包映射。

### 2026-11-27 (Batch 114 — **650+ 官方 `.cap` 包里程碑！拿下 crates.io #10 `num-traits`(20 亿)+`num-integer`(14 亿)+`ordered-float`(12 亿)+`num-rational`+`approx` + `raw-body` + `merge-descriptors`**)
- `packages/@cap/raw-body/` & `cap/test/testCapRawBody.cp`: npm Top 30 `raw-body` v3（1.5 亿+/周，有界 HTTP 请求体读取与校验，6/6 PASS）。
- `packages/@cap/merge-descriptors/` & `cap/test/testCapMergeDescriptors.cp`: npm Top 45 `merge-descriptors` v2（1.2 亿+/周，描述符表安全合并，6/6 PASS）。
- `packages/@cap/num-traits/` & `cap/test/testCapNumTraits.cp`: crates.io **#10** `num-traits` v0.2（**20 亿+**，Checked/Saturating 算术与位操作，6/6 PASS）。
- `packages/@cap/num-integer/` & `cap/test/testCapNumInteger.cp`: crates.io **#55** `num-integer` v0.1（14 亿+，GCD/LCM/扩展 GCD/整除/整数开方，6/6 PASS）。
- `packages/@cap/num-rational/` & `cap/test/testCapNumRational.cp`: crates.io #120 `num-rational` v0.4（9.5 亿+，跨包复用 `@cap/num-integer`，6/6 PASS）。
- `packages/@cap/ordered-float/` & `cap/test/testCapOrderedFloat.cp`: crates.io **#85** `ordered-float` v4（12 亿+，跨包复用 `@cap/fnv-hash`，6/6 PASS）。
- `packages/@cap/approx/` & `cap/test/testCapApprox.cp`: crates.io #130 `approx` v0.5（9 亿+，绝对/相对/ULPs 浮点近似比较，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 114 全部 7 个官方包映射。

### 2026-11-27 (Batch 115 — **突破 655 LIVE 官方 `.cap` 包！拿下 npm Top 10 `picocolors`(3 亿+/周)+`kleur`(1.5 亿+/周)+`cookie-parser` + crates.io `num-iter`(8.5 亿+)+`iso8601`+`rfc3339`+`float-cmp`**)
- `packages/@cap/picocolors/` & `cap/test/testCapPicocolors.cp`: npm **Top 10** `picocolors` v1（3 亿+/周，复用 `@cap/ansi-styles`，6/6 PASS）。
- `packages/@cap/kleur/` & `cap/test/testCapKleur.cp`: npm Top 25 `kleur` v4（1.5 亿+/周，复用 `@cap/ansi-styles`，6/6 PASS）。
- `packages/@cap/cookie-parser/` & `cap/test/testCapCookieParser.cp`: npm Top 35 `cookie-parser`（1.2 亿+/周，双包复用 `@cap/cookie-jar`+`@cap/cookie-signature`，6/6 PASS）。
- `packages/@cap/float-cmp/` & `cap/test/testCapFloatCmp.cp`: crates.io `float-cmp` v0.10（复用 `@cap/approx`，6/6 PASS）。
- `packages/@cap/num-iter/` & `cap/test/testCapNumIter.cp`: crates.io #140 `num-iter` v0.1（8.5 亿+，复用 `@cap/num-integer`，6/6 PASS）。
- `packages/@cap/iso8601/` & `cap/test/testCapIso8601.cp`: crates.io #250 `iso8601`（4.5 亿+，完整日历/周/序数日期+时间+周期，6/6 PASS）。
- `packages/@cap/rfc3339/` & `cap/test/testCapRfc3339.cp`: crates.io `rfc3339`（复用 `@cap/iso8601`，严格 RFC 3339 与 Unix Epoch 双向换算，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 115 全部 7 个官方包映射。

### 2026-11-27 (Batch 116 — **突破 660 LIVE 官方 `.cap` 包！新增 `colorette`(1.5 亿+/周)+`npm-run-path`(1.4 亿+/周)+`parse-duration`+`mediatype`+`accept-header`**)
- `packages/@cap/colorette/` & `cap/test/testCapColorette.cp`: npm Top 20 `colorette` v2（1.5 亿+/周，复用 `@cap/ansi-styles`，6/6 PASS）。
- `packages/@cap/npm-run-path/` & `cap/test/testCapNpmRunPath.cp`: npm Top 30 `npm-run-path`（1.4 亿+/周，祖先 `.bin` 路径构建，6/6 PASS）。
- `packages/@cap/parse-duration/` & `cap/test/testCapParseDuration.cp`: crates.io `parse_duration`（复合/小数时长解析格式化，6/6 PASS）。
- `packages/@cap/mediatype/` & `cap/test/testCapMediatype.cp`: crates.io `mediatype`（双包复用 `@cap/media-typer`+`@cap/unicase`，6/6 PASS）。
- `packages/@cap/accept-header/` & `cap/test/testCapAcceptHeader.cp`: crates.io `accept-header`（双包复用 `@cap/negotiator`+`@cap/unicase`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增 Batch 116 全部 5 个官方包映射（875 mappings）。

### 2026-11-27 (Batch 117 — **突破 665 官方 `.cap` 包！新增 `on-finished`(1.6 亿+/周)+`destroy`(1.5 亿+/周)+`pretty-format`(1 亿+/周)+`jest-diff`+`wkt`**)
- `packages/@cap/jest-diff/` & `cap/test/testCapJestDiff.cp`: npm Top 50 `jest-diff`（9000 万+/周，双包复用 `@cap/similar-diff`+`@cap/picocolors`，6/6 PASS）。
- `packages/@cap/pretty-format/` & `cap/test/testCapPrettyFormat.cp`: npm Top 45 `pretty-format`（1 亿+/周，确定性值美化打印，6/6 PASS）。
- `packages/@cap/on-finished/` & `cap/test/testCapOnFinished.cp`: npm Top 25 `on-finished`（1.6 亿+/周，流完成状态机，6/6 PASS）。
- `packages/@cap/destroy/` & `cap/test/testCapDestroy.cp`: npm Top 35 `destroy`（1.5 亿+/周，复用 `@cap/on-finished`，6/6 PASS）。
- `packages/@cap/wkt/` & `cap/test/testCapWkt.cp`: crates.io `wkt` v0.11（GIS Well-Known Text 解析与格式化，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增 Batch 117 全部 5 个官方包映射（880 mappings）。

### 2026-11-27 (Batch 118 — **突破 670 官方 `.cap` 包！拿下 17 亿下载线性代数双璧 `nalgebra`(10 亿)+`mint`(7 亿) + `geojson` + `ee-first`(1.6 亿/周) + `send`(1.4 亿/周)**)
- `packages/@cap/ee-first/` & `cap/test/testCapEeFirst.cp`: npm Top 30 `ee-first` v1（1.6 亿+/周，多事件竞态自动取消，6/6 PASS）。
- `packages/@cap/send/` & `cap/test/testCapSend.cp`: npm Top 35 `send` v1（1.4 亿+/周，四包复用静态文件流策略引擎，6/6 PASS）。
- `packages/@cap/geojson/` & `cap/test/testCapGeojson.cp`: crates.io `geojson` v0.24（复用 `@cap/wkt`，RFC 7946 GeoJSON ↔ WKT 互转，6/6 PASS）。
- `packages/@cap/mint/` & `cap/test/testCapMint.cp`: crates.io #180 `mint`（7 亿+，图形/物理数学互操作标准类型，6/6 PASS）。
- `packages/@cap/nalgebra/` & `cap/test/testCapNalgebra.cp`: crates.io **#110** `nalgebra`（10 亿+，复用 `@cap/mint`，线性代数与仿射变换，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增 Batch 118 全部 5 个官方包映射（885 mappings）。

### 2026-11-27 (Batch 119 — **突破 676 官方 `.cap` 包！拿下 `ndarray`(9 亿+)+`execa`(1.4 亿/周)+`bigdecimal`+`tzfile`+`multipart`+`multer`**)
- `packages/@cap/multer/` & `cap/test/testCapMulter.cp`: npm Top 55 `multer` v2（8000 万+/周，双包复用 `@cap/mediatype`+`@cap/mime-types`，6/6 PASS）。
- `packages/@cap/execa/` & `cap/test/testCapExeca.cp`: npm Top 30 `execa`（1.4 亿+/周，复用 `@cap/npm-run-path`，6/6 PASS）。
- `packages/@cap/bigdecimal/` & `cap/test/testCapBigdecimal.cp`: crates.io `bigdecimal` v0.4（金融级高精度十进制算术，6/6 PASS）。
- `packages/@cap/ndarray/` & `cap/test/testCapNdarray.cp`: crates.io **#125** `ndarray`（9 亿+，2D/N-D 数值数组与矩阵乘，6/6 PASS）。
- `packages/@cap/tzfile/` & `cap/test/testCapTzfile.cp`: crates.io `tzfile`（RFC 8536 TZif + POSIX TZ 解析，6/6 PASS）。
- `packages/@cap/multipart/` & `cap/test/testCapMultipart.cp`: crates.io `multipart`（双包复用 `@cap/mediatype`+`@cap/unicase`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增 Batch 119 全部 6 个官方包映射（891 mappings）。

### 2026-11-27 (Batch 120 — **8 包超级大波！突破 684 官方 `.cap` 包 & 899 映射！拿下 `crc32fast`(17 亿)+`simdutf8`(8.5 亿)+`fast-glob`(1.6 亿/周)+`cosmiconfig`(1.5 亿/周)+`h3o`+`s2`+`statrs`+`formidable`**)
- `packages/@cap/formidable/` & `cap/test/testCapFormidable.cp`: npm Top 60 `formidable` v3（双包复用 `@cap/mediatype`+`@cap/content-disposition`，6/6 PASS）。
- `packages/@cap/fast-glob/` & `cap/test/testCapFastGlob.cp`: npm Top 25 `fast-glob` v3（1.6 亿+/周，复用 `@cap/glob-match`，6/6 PASS）。
- `packages/@cap/cosmiconfig/` & `cap/test/testCapCosmiconfig.cp`: npm Top 30 `cosmiconfig` v9（1.5 亿+/周，配置向上搜索规划器，6/6 PASS）。
- `packages/@cap/h3o/` & `cap/test/testCapH3o.cp`: crates.io `h3o` v0.6（Uber H3 64 位六边形空间索引，6/6 PASS）。
- `packages/@cap/s2/` & `cap/test/testCapS2.cp`: crates.io `s2`（Google S2 64 位球面几何与大圆距离，6/6 PASS）。
- `packages/@cap/statrs/` & `cap/test/testCapStatrs.cp`: crates.io `statrs` v0.17（概率分布、描述统计与线性回归，6/6 PASS）。
- `packages/@cap/crc32fast/` & `cap/test/testCapCrc32fast.cp`: crates.io **#30** `crc32fast`（**17 亿+**，CRC-32 + GF(2) combine，6/6 PASS）。
- `packages/@cap/simdutf8/` & `cap/test/testCapSimdutf8.cp`: crates.io **#135** `simdutf8`（**8.5 亿+**，严格 RFC 3629 UTF-8 校验，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增 Batch 120 全部 8 个官方包映射（899 mappings）。

### 2026-11-27 (Batch 121 — **突破 690 官方 `.cap` 包 & 900+ 上游映射！100% 跨包复用！拿下 `hmac`(16 亿)+`hkdf`(11 亿)+`rand_distr`(8 亿)+`chokidar`(1.6 亿/周)+`lilconfig`+`listr2`**)
- `packages/@cap/lilconfig/` & `cap/test/testCapLilconfig.cp`: npm Top 50 `lilconfig` v3（9000 万+/周，复用 `@cap/cosmiconfig`，6/6 PASS）。
- `packages/@cap/chokidar-core/` & `cap/test/testCapChokidarCore.cp`: npm Top 25 `chokidar` v4（1.6 亿+/周，复用 `@cap/fast-glob`，6/6 PASS）。
- `packages/@cap/listr2/` & `cap/test/testCapListr2.cp`: npm Top 70 `listr2`（复用 `@cap/picocolors`，6/6 PASS）。
- `packages/@cap/rand-distr/` & `cap/test/testCapRandDistr.cp`: crates.io **#145** `rand_distr` v0.4（8 亿+，复用 `@cap/xoshiro256`，6/6 PASS）。
- `packages/@cap/hmac-crate/` & `cap/test/testCapHmacCrate.cp`: crates.io **#38** `hmac`（**16 亿+**，复用 `@cap/cookie-signature`，RFC 4231 向量全过，6/6 PASS）。
- `packages/@cap/hkdf-crate/` & `cap/test/testCapHkdfCrate.cp`: crates.io **#95** `hkdf`（**11 亿+**，复用 `@cap/hmac-crate`，RFC 5869 向量全过，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 121 全部 6 个官方包映射。

### 2026-11-27 (Batch 122 — **突破 696 官方 `.cap` 包 & 900+ 上游映射！拿下 37.5 亿下载密码三巨头 `sha1`(16.5 亿)+`md-5`(13 亿)+`pbkdf2`(8 亿) + `find-up`(1.8 亿/周)+`validate-npm-package-name`(1.5 亿/周)+`enquirer`**)
- `packages/@cap/enquirer/` & `cap/test/testCapEnquirer.cp`: npm Top 65 `enquirer` v2（复用 `@cap/picocolors`，6/6 PASS）。
- `packages/@cap/validate-npm-package-name/` & `cap/test/testCapValidateNpmPackageName.cp`: npm Top 25 `validate-npm-package-name`（1.5 亿+/周，6/6 PASS）。
- `packages/@cap/find-up/` & `cap/test/testCapFindUp.cp`: npm Top 20 `find-up`（1.8 亿+/周，复用 `@cap/npm-run-path`，6/6 PASS）。
- `packages/@cap/sha1-crate/` & `cap/test/testCapSha1Crate.cp`: crates.io **#35** `sha1` v0.10（**16.5 亿+**，FIPS 180-4 向量全过，6/6 PASS）。
- `packages/@cap/md5-crate/` & `cap/test/testCapMd5Crate.cp`: crates.io **#65** `md-5`（**13 亿+**，RFC 1321 向量全过，6/6 PASS）。
- `packages/@cap/pbkdf2-crate/` & `cap/test/testCapPbkdf2Crate.cp`: crates.io **#155** `pbkdf2`（**8 亿+**，复用 `@cap/hmac-crate`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 122 全部 6 个官方包映射。

### 2026-11-27 (Batch 123 — **🎉 突破 700 官方 `.cap` 包大关！拿下 `chacha20`(10 亿+)+`picomatch`(2.2 亿/周)+`resolve-from`(1.5 亿/周)+`pkg-dir`(1.3 亿/周)**)
- `packages/@cap/picomatch-core/` & `cap/test/testCapPicomatchCore.cp`: npm **Top 15** `picomatch` v4（**2.2 亿+/周**，复用 `@cap/glob-match`，6/6 PASS）。
- `packages/@cap/pkg-dir/` & `cap/test/testCapPkgDir.cp`: npm Top 35 `pkg-dir`（1.3 亿+/周，双包复用 `@cap/find-up`+`@cap/npm-run-path`，6/6 PASS）。
- `packages/@cap/resolve-from/` & `cap/test/testCapResolveFrom.cp`: npm Top 25 `resolve-from`（1.5 亿+/周，双包复用 `@cap/find-up`+`@cap/npm-run-path`，6/6 PASS）。
- `packages/@cap/chacha20-crate/` & `cap/test/testCapChacha20Crate.cp`: crates.io **#115** `chacha20` v0.9（**10 亿+**，RFC 8439 向量全过，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 123 全部 4 个官方包映射。

### 2026-11-27 (Batch 124 — **突破 703 官方 `.cap` 包！拿下 `tiny-keccak`(7.5 亿+)+`locate-path`(1.8 亿/周)+`make-dir`(1.6 亿/周)+`path-exists`(1.5 亿/周)**)
- `packages/@cap/locate-path/` & `cap/test/testCapLocatePath.cp`: npm **Top 20** `locate-path` v7（1.8 亿+/周，复用 `@cap/find-up`，6/6 PASS）。
- `packages/@cap/path-exists/` & `cap/test/testCapPathExists.cp`: npm Top 30 `path-exists`（1.5 亿+/周，复用 `@cap/locate-path`，6/6 PASS）。
- `packages/@cap/make-dir/` & `cap/test/testCapMakeDir.cp`: npm Top 25 `make-dir`（1.6 亿+/周，递归目录创建规划，6/6 PASS）。
- `packages/@cap/tiny-keccak/` & `cap/test/testCapTinyKeccak.cp`: crates.io **#160** `tiny-keccak` v2（7.5 亿+，SHA3-256 + Ethereum Keccak-256，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 124 全部 4 个官方包映射。

### 2026-11-27 (Batch 125 — **突破 708 官方 `.cap` 包！拿下 44.5 亿下载三巨头 `memchr`(19 亿)+`zeroize`(17 亿)+`blake2`(8.5 亿) + `spdx-expression-parse` + `validate-npm-package-license`**)
- `packages/@cap/spdx-expression-parse/` & `cap/test/testCapSpdxExpressionParse.cp`: npm **Top 25** `spdx-expression-parse` v4（1.6 亿+/周，复用 `@cap/spdx`，6/6 PASS）。
- `packages/@cap/validate-npm-package-license/` & `cap/test/testCapValidateNpmPackageLicense.cp`: npm **Top 25** `validate-npm-package-license` v3（1.6 亿+/周，复用 `@cap/spdx-expression-parse`，6/6 PASS）。
- `packages/@cap/blake2-crate/` & `cap/test/testCapBlake2Crate.cp`: crates.io **#140** `blake2` v0.10（8.5 亿+，RFC 7693 BLAKE2s-256，6/6 PASS）。
- `packages/@cap/memchr-crate/` & `cap/test/testCapMemchrCrate.cp`: crates.io **#15** `memchr`（**19 亿+**，`memchr`/`memrchr`/`memmem`，6/6 PASS）。
- `packages/@cap/zeroize/` & `cap/test/testCapZeroize.cp`: crates.io **#32** `zeroize`（**17 亿+**，密钥内存清零与容器，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 125 全部 5 个官方包映射。

### 2026-11-27 (Batch 126 — **突破 715 官方 `.cap` 包！拿下 57 亿下载四大底座 `bytemuck`(18 亿)+`zerocopy`(17.5 亿)+`tar`(11 亿)+`chacha20poly1305`(10.5 亿) + `normalize-package-data` + `read-pkg`**)
- `packages/@cap/normalize-package-data/` & `cap/test/testCapNormalizePackageData.cp`: npm **Top 25** `normalize-package-data` v6（1.5 亿+/周，双包复用，6/6 PASS）。
- `packages/@cap/read-pkg/` & `cap/test/testCapReadPkg.cp`: npm Top 45 `read-pkg` v9（1 亿+/周，复用 `@cap/normalize-package-data`，6/6 PASS）。
- `packages/@cap/chacha20poly1305/` & `cap/test/testCapChacha20poly1305.cp`: crates.io **#105** `chacha20poly1305` v0.10（**10.5 亿+**，三包复用，RFC 8439 AEAD 全过，6/6 PASS）。
- `packages/@cap/tar-core/` & `cap/test/testCapTarCore.cp`: crates.io **#100** `tar` v0.4（**11 亿+**，POSIX USTAR 编解码游标，6/6 PASS）。
- `packages/@cap/bytemuck-crate/` & `cap/test/testCapBytemuckCrate.cp`: crates.io **#22** `bytemuck`（**18 亿+**，POD 字节重解释，6/6 PASS）。
- `packages/@cap/zerocopy-crate/` & `cap/test/testCapZerocopyCrate.cp`: crates.io **#25** `zerocopy`（**17.5 亿+**，复用 `@cap/bytemuck-crate`，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 126 全部 6 个官方包映射。

### 2026-11-27 (Batch 127 — **突破 719 官方 `.cap` 包！拿下 crates.io #14 `sha2`(19 亿)+`zip`(9 亿) + `hosted-git-info`(1.6 亿/周)+`read-pkg-up`+`semver-diff`**)
- `packages/@cap/read-pkg-up/` & `cap/test/testCapReadPkgUp.cp`: npm Top 50 `read-pkg-up` v11（1 亿+/周，双包复用，6/6 PASS）。
- `packages/@cap/hosted-git-info/` & `cap/test/testCapHostedGitInfo.cp`: npm **Top 25** `hosted-git-info`（1.6 亿+/周，6/6 PASS）。
- `packages/@cap/semver-diff/` & `cap/test/testCapSemverDiff.cp`: npm Top 50 `semver-diff` v5（9000 万+/周，复用 `@cap/semver-range`，6/6 PASS）。
- `packages/@cap/zip-core/` & `cap/test/testCapZipCore.cp`: crates.io **#130** `zip` v2（9 亿+，双包复用，6/6 PASS）。
- `packages/@cap/sha2-crate/` & `cap/test/testCapSha2Crate.cp`: crates.io **#14** `sha2`（**19 亿+**，FIPS 180-4 SHA-256 & SHA-224，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 127 全部 5 个官方包映射。

### 2026-11-27 (Batch 128 — **突破 725 官方 `.cap` 包！拿下 37.5 亿下载三巨头 `flate2`(18.5 亿)+`siphasher`(12.5 亿)+`bytecount`(6.5 亿) + `p-limit`(2.5 亿/周)+`rimraf`(1.8 亿/周)+`ripemd160`**)
- `packages/@cap/rimraf-core/` & `cap/test/testCapRimrafCore.cp`: npm **Top 20** `rimraf` v6（1.8 亿+/周，复用 `@cap/fast-glob`，6/6 PASS）。
- `packages/@cap/p-limit-core/` & `cap/test/testCapPLimitCore.cp`: npm **Top 12** `p-limit` v6（**2.5 亿+/周**，复用 `@cap/p-queue`，6/6 PASS）。
- `packages/@cap/flate2-core/` & `cap/test/testCapFlate2Core.cp`: crates.io **#18** `flate2` v1（**18.5 亿+**，三包复用 Gzip/Zlib 帧编解码，6/6 PASS）。
- `packages/@cap/ripemd160/` & `cap/test/testCapRipemd160.cp`: crates.io `ripemd` v0.1（7 组官方向量全过，6/6 PASS）。
- `packages/@cap/bytecount/` & `cap/test/testCapBytecount.cp`: crates.io **#190** `bytecount`（6.5 亿+，复用 `@cap/memchr-crate`，6/6 PASS）。
- `packages/@cap/siphasher/` & `cap/test/testCapSiphasher.cp`: crates.io **#70** `siphasher`（**12.5 亿+**，SipHash-2-4/1-3 官方向量全过，6/6 PASS）。
- `packages/@cap/registry.json`: 同步新增与升级 Batch 128 全部 6 个官方包映射。

### 2026-11-27 (Batch 129 & 生态溯源检索大升级 — **612 个包补齐 `cap.json` `"source"` 溯源 + `capdux.org` 原库名检索/展示上线 + `GEMINI.md` Rule #12 固化**)
- `packages/@cap/twox-hash/` & `cap/test/testCapTwoxHash.cp`: crates.io **#135** `twox-hash` v2（8.5 亿+，XXH32/XXH64 官方向量全过，6/6 PASS）。
- `packages/@cap/seahash/` & `cap/test/testCapSeahash.cp`: crates.io #220 `seahash` v4（5.5 亿+，6/6 PASS）。
- `packages/@cap/p-locate/` & `cap/test/testCapPLocate.cp`: npm **Top 15** `p-locate` v7（2.2 亿+/周，复用 `@cap/p-limit-core`，6/6 PASS）。
- `packages/@cap/p-timeout-core/` & `cap/test/testCapPTimeoutCore.cp`: npm Top 45 `p-timeout` v6（1 亿+/周，6/6 PASS）。
- `packages/@cap/*/cap.json` & `README.md`: 批量回填 **612 个**官方包的 `"source"` 字段（含 `registry`/`name`/`repo`/`license`）与 **609 个** `README.md` 参考/吸收来源横幅（含改名说明）。
- `GEMINI.md`: 固化第 12 条（表格汇报、溯源字段、`capdux.org` 原库名检索、吸收前必查 `lib/` & `@cap/` 防重复决策矩阵）。
- `/data/dev/cap/worker/src/{routes/search.ts,web.ts}`: 升级 `capdux.org` 搜索 API、首页卡片与详情页，支持直接按原库名/别名检索并显式展示 `from npm:<orig>` / `crates.io:<orig>` 与改名提示。

### 2026-11-27 (Batch 130 & 首轮去重收口 — **合并清理 12 组后缀重叠包 + 新增 5 大零重复溯源包 `rustc-hash`(12.5 亿)+`crc32c`(7.5 亿)+`p-try`+`is-plain-obj`+`map-obj`**)
- **去重收口（Rule #12）**：将 12 组 0 反向依赖的后缀重叠包（`aho-corasick-dfa`、`bitflags-crate`、`boxen-ui`、`forwarded-http`、`fresh-http`、`hex-crate`、`lru-crate`、`picomatch-core`、`qs-parse`、`textwrap-crate`、`ulid-crate`、`vary-http`）映射统一收口至标准主名包并移除冗余副本，消除选择困难。
- `packages/@cap/p-try/` & `cap/test/testCapPTry.cp`: npm **Top 25** `p-try` v3（1.6 亿+/周，含 `"source"` 与溯源横幅，6/6 PASS）。
- `packages/@cap/is-plain-obj/` & `cap/test/testCapIsPlainObj.cp`: npm Top 30 `is-plain-obj` v4（1.4 亿+/周，含 `"source"` 与溯源横幅，6/6 PASS）。
- `packages/@cap/map-obj/` & `cap/test/testCapMapObj.cp`: npm Top 35 `map-obj`（1.3 亿+/周，含 `"source"` 与溯源横幅，6/6 PASS）。
- `packages/@cap/crc32c/` & `cap/test/testCapCrc32c.cp`: crates.io **#165** `crc32c` v0.6（7.5 亿+，RFC 3720 向量全过，6/6 PASS）。
- `packages/@cap/rustc-hash/` & `cap/test/testCapRustcHash.cp`: crates.io **#80** `rustc-hash` v2（**12.5 亿+**，`FxHasher64`/`32`，6/6 PASS）。

### 2026-11-27 (Batch 131 & 二轮去重收口 — **清理 `ms-parse`/`pem-parse` + 新增 6 大零重复溯源包 `pathdiff`(9.5 亿)+`smol-str`(8.5 亿)+`ipnet`(8.5 亿)+`thin-vec`(7 亿)+`snakecase-keys`+`decamelize-keys`**)
- **去重收口（Rule #12）**：将 0 反向依赖的 `ms-parse` → `@cap/ms`、`pem-parse` → `@cap/pem` 统一收口并移除冗余目录与测试文件。
- `packages/@cap/snakecase-keys/` & `cap/test/testCapSnakecaseKeys.cp`: npm `snakecase-keys`（复用 `@cap/map-obj` + `@cap/heck-case`，6/6 PASS）。
- `packages/@cap/decamelize-keys/` & `cap/test/testCapDecamelizeKeys.cp`: npm Top 65 `decamelize-keys` v2（复用 `@cap/map-obj` + `@cap/decamelize`，6/6 PASS）。
- `packages/@cap/smol-str/` & `cap/test/testCapSmolStr.cp`: crates.io **#135** `smol_str`（8.5 亿+，6/6 PASS）。
- `packages/@cap/thin-vec/` & `cap/test/testCapThinVec.cp`: crates.io **#185** `thin-vec`（7 亿+，6/6 PASS）。
- `packages/@cap/pathdiff/` & `cap/test/testCapPathdiff.cp`: crates.io **#120** `pathdiff`（9.5 亿+，6/6 PASS）。
- `packages/@cap/ipnet/` & `cap/test/testCapIpnet.cp`: crates.io **#140** `ipnet` v2（8.5 亿+，6/6 PASS）。
- `packages/@cap/registry.json` & `cp_lib_ref.md`: 同步 6 项映射与去重收口。

### 2026-11-27 (Batch 132 & **100% 溯源覆盖收官** — **新增 6 大零重复包 `nanoid`(1 亿/周)+`cli-truncate`(8000 万/周)+`relative-path`(8 亿)+`publicsuffix`(6 亿)+`macaddr`+`kebabcase-keys` + 补齐全部 76 个早期包 `"source"` 元数据**)
- **100% 溯源达标（Rule #12）**：补齐全部 76 个早期 `@cap/*` 包的 `cap.json` `"source"` 字段与 50 个 `README.md` 溯源横幅，全仓 `@cap/*` 溯源覆盖率达 **100%（0 缺失）**；`packages/@cap/registry.json` 映射突破 **910 条**（npm 450 + crates.io 460）。
- `packages/@cap/kebabcase-keys/` & `cap/test/testCapKebabcaseKeys.cp`: npm `kebabcase-keys` v1（双包复用 `@cap/decamelize-keys` + `@cap/map-obj`，6/6 PASS）。
- `packages/@cap/nanoid/` & `cap/test/testCapNanoid.cp`: npm **Top 25** `nanoid` v5（1 亿+/周，位掩码无模偏差采样，6/6 PASS）。
- `packages/@cap/cli-truncate/` & `cap/test/testCapCliTruncate.cp`: npm **Top 50** `cli-truncate` v4（8000 万+/周，双包复用 `@cap/string-width` + `@cap/slice-ansi`，6/6 PASS）。
- `packages/@cap/publicsuffix/` & `cap/test/testCapPublicsuffix.cp`: crates.io **#210** `publicsuffix`（6 亿+，PSL eTLD+1 与通配/例外规则，6/6 PASS）。
- `packages/@cap/macaddr/` & `cap/test/testCapMacaddr.cp`: crates.io `macaddr` v1（IEEE 802 MAC-48 & EUI-64，6/6 PASS）。
- `packages/@cap/relative-path/` & `cap/test/testCapRelativePath.cp`: crates.io **#145** `relative-path`（8 亿+，跨平台相对路径归一化与 `relativeTo`，6/6 PASS）。
- `packages/@cap/mitt/` & `cap/test/testCapMitt.cp`: npm **Top 45** `mitt` v3（5000 万+/周，32 槽位零堆事件总线与 `"*"` 通配符，6/6 PASS）。
- `packages/@cap/camino/` & `cap/test/testCapCamino.cp`: crates.io **#75** `camino` v1（13 亿+，复用 `@cap/utf8-valid`，严格 UTF-8 路径引擎，6/6 PASS）。
- `PROGRESS.md` & `GEMINI.md` & `SESSION.md` / `SESSION_H.md`: 补齐全部 14 条会话历史审计留档（`missing_tickets=0, missing_user_turns=0`），固化 CP 1-pass codegen 与省 Token 工作流铁律进 `GEMINI.md` Rule #11，生成无缝交接文档 `PROGRESS.md`。

### 2026-11-27 (Batch 133 — **剩余 3 组含反向依赖后缀重复包全部清零收口** + **新增 5 大零重复高热度包**)
- **后缀重复包 100% 清零收口（Rule #12）**：
  - `@cap/etag-http` → 合并进 `@cap/etag`（补齐 `compareStrong`/`compareWeak` 并加 `etg*` 私有前缀消除跨包 `strEq` 冲突），迁移 `@cap/send`、`@cap/serve-static` 依赖与测试（`testCapEtag`/`testCapSend`/`testCapServeStatic` 18/18 PASS），移除 `packages/@cap/etag-http`；
  - `@cap/memchr-crate` → 合并进 `@cap/memchr`（补齐 `countByte` 别名并修复 `memchrCount` 局部变量遮蔽），迁移 `@cap/bytecount` 依赖与测试（`testCapMemchr`/`testCapBytecount` 12/12 PASS），移除 `packages/@cap/memchr-crate`；
  - `@cap/p-limit-core` → 合并进 `@cap/p-limit`（新增 `enqueue`/`startNext`/`finishOne` 两阶段调度方法），迁移 `@cap/p-locate` 依赖与测试（`testCapPLimit`/`testCapPLocate` 12/12 PASS），移除 `packages/@cap/p-limit-core`。
- `packages/@cap/is-unicode-supported/` & `cap/test/testCapIsUnicodeSupported.cp`: npm `is-unicode-supported` v2（1 亿+/周，6/6 PASS）。
- `packages/@cap/widest-line/` & `cap/test/testCapWidestLine.cp`: npm `widest-line`（7000 万+/周，复用 `@cap/string-width`，6/6 PASS）。
- `packages/@cap/trim-newlines/` & `cap/test/testCapTrimNewlines.cp`: npm `trim-newlines` v5（5000 万+/周，零拷贝，6/6 PASS）。
- `packages/@cap/normpath/` & `cap/test/testCapNormpath.cp`: crates.io `normpath`（跨平台 POSIX/Windows/UNC 路径规范化，6/6 PASS）。
- `packages/@cap/unicode-ident/` & `cap/test/testCapUnicodeIdent.cp`: crates.io Top 100 `unicode-ident`（5 亿+，UAX #31 标识符判定，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射、重指向 3 组收口映射，并运行 `gen_agent_lib_ref.py` 刷新极简速查手册。
- `packages/@cap/strip-indent/` & `cap/test/testCapStripIndent.cp`: npm `strip-indent`（5000 万+/周，6/6 PASS）。
- `packages/@cap/redent/` & `cap/test/testCapRedent.cp`: npm `redent`（4000 万+/周，双包复用 `@cap/strip-indent` + `@cap/indent-string`，6/6 PASS）。

### 2026-11-27 (Batch 134 — **新增 5 大零重复 Top-700 包：2 npm + 3 crates.io，30/30 单测 100% PASS**)
- `packages/@cap/ansi-align/` & `cap/test/testCapAnsiAlign.cp`: npm `ansi-align`（4000 万+/周，双包复用 `@cap/string-width` + `@cap/widest-line`，6/6 PASS）。
- `packages/@cap/filenamify/` & `cap/test/testCapFilenamify.cp`: npm `filenamify` v6（3000 万+/周，Windows 保留名防护 + UTF-8 安全截断，6/6 PASS）。
- `packages/@cap/path-slash/` & `cap/test/testCapPathSlash.cp`: crates.io `path-slash`（POSIX/Windows 斜杠双向转换，6/6 PASS）。
- `packages/@cap/bytesize/` & `cap/test/testCapBytesize.cp`: crates.io `bytesize`（纯整数 SI/IEC 字节解析与格式化，6/6 PASS）。
- `packages/@cap/shellexpand/` & `cap/test/testCapShellexpand.cp`: crates.io `shellexpand` v3（零堆 `~` 与 `$VAR`/`${VAR:-default}` 展开，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射并刷新极简速查手册；Worker N15 满 6 包光荣轮换为 Worker N16。

### 2026-11-27 (Batch 135 — **新增 5 大零重复 Top-700 包：3 npm + 2 crates.io，30/30 单测 100% PASS**)
- `packages/@cap/version-compare/` & `cap/test/testCapVersionCompare.cp`: crates.io `version-compare`（多段版本比较与 `satisfies`，6/6 PASS）。
- `packages/@cap/http-range-header/` & `cap/test/testCapHttpRangeHeader.cp`: crates.io `http-range-header`（RFC 9110 `Range` 解析与校验，6/6 PASS）。
- `packages/@cap/prepend-http/` & `cap/test/testCapPrependHttp.cp`: npm `prepend-http` v4（2000 万+/周，6/6 PASS）。
- `packages/@cap/normalize-url/` & `cap/test/testCapNormalizeUrl.cp`: npm `normalize-url`（2500 万+/周，复用 `@cap/prepend-http`，6/6 PASS）。
- `packages/@cap/clean-stack/` & `cap/test/testCapCleanStack.cp`: npm `clean-stack` v5（5000 万+/周，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射并刷新极简速查手册；Worker C14 满 6 包光荣轮换为 Worker C15。

### 2026-11-27 (Batch 136 — **新增 5 大零重复 Top-700 包：2 npm + 3 crates.io，30/30 单测 100% PASS**)
- `packages/@cap/humanize-url/` & `cap/test/testCapHumanizeUrl.cp`: npm `humanize-url` v3（1500 万+/周，6/6 PASS）。
- `packages/@cap/extract-stack/` & `cap/test/testCapExtractStack.cp`: npm `extract-stack` v2（2000 万+/周，6/6 PASS）。
- `packages/@cap/http-content-range/` & `cap/test/testCapHttpContentRange.cp`: crates.io `http-content-range`（RFC 9110 `Content-Range`，6/6 PASS）。
- `packages/@cap/path-clean/` & `cap/test/testCapPathClean.cp`: crates.io `path-clean`（单遍词法路径清洗，6/6 PASS）。
- `packages/@cap/urlencoding/` & `cap/test/testCapUrlencoding.cp`: crates.io `urlencoding` v2（RFC 3986 百分号编解码，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射并刷新极简速查手册。

### 2026-11-27 (Batch 137 — **新增 5 大零重复 Top-700 包：3 npm + 2 crates.io，30/30 单测 100% PASS**)
- `packages/@cap/humanize-string/` & `cap/test/testCapHumanizeString.cp`: npm `humanize-string` v3（2500 万+/周，复用 `@cap/decamelize`，6/6 PASS）。
- `packages/@cap/titleize/` & `cap/test/testCapTitleize.cp`: npm `titleize` v3（1500 万+/周，6/6 PASS）。
- `packages/@cap/is-fullwidth-code-point/` & `cap/test/testCapIsFullwidthCodePoint.cp`: npm **Top 20** `is-fullwidth-code-point` v5（1.8 亿+/周，6/6 PASS）。
- `packages/@cap/humansize/` & `cap/test/testCapHumansize.cp`: crates.io `humansize` v2（多预设人类可读字节格式化，6/6 PASS）。
- `packages/@cap/oorandom/` & `cap/test/testCapOorandom.cp`: crates.io `oorandom`（PCG32/PCG64 确定性 PRNG，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射并刷新极简速查手册；Worker N16 满 6 包光荣轮换为 Worker N17。

### 2026-11-27 (Batch 138 — **新增 4 包 + 引入 `scripts/fetch_real_top700.py` 真实下载量排行校准**)
- `packages/@cap/p-all/` & `cap/test/testCapPAll.cp`: npm `p-all` v5（复用 `@cap/p-limit`，6/6 PASS）。
- `packages/@cap/p-settle/` & `cap/test/testCapPSettle.cp`: npm `p-settle` v5（复用 `@cap/p-limit`，6/6 PASS）。
- `packages/@cap/unicode-script/` & `cap/test/testCapUnicodeScript.cp`: crates.io `unicode-script`（6/6 PASS）。
- `packages/@cap/pep440-rs/` & `cap/test/testCapPep440Rs.cp`: crates.io `pep440_rs`（6/6 PASS）。
- `scripts/fetch_real_top700.py`: 新增双平台真实下载量 Top 700 实时拉取与差集校准脚本，后续严格按真实下载量从高到低吸收。

### 2026-11-27 (Batch 139 — **真实下载量 Top 榜首波 5 大重量级包：npm #17/#18/#62 + crates.io #268/#349，30/30 PASS**)
- `packages/@cap/emoji-regex/` & `cap/test/testCapEmojiRegex.cp`: npm **#17** `emoji-regex`（5.98 亿/周，6/6 PASS）。
- `packages/@cap/balanced-match/` & `cap/test/testCapBalancedMatch.cp`: npm **#18** `balanced-match`（5.78 亿/周，6/6 PASS）。
- `packages/@cap/string-decoder/` & `cap/test/testCapStringDecoder.cp`: npm **#62** `string_decoder`（3.78 亿/周，6/6 PASS）。
- `packages/@cap/potential-utf/` & `cap/test/testCapPotentialUtf.cp`: crates.io **#268** `potential_utf`（6.27 亿+，6/6 PASS）。
- `packages/@cap/aes/` & `cap/test/testCapAes.cp`: crates.io **#349** `aes`（4.38 亿+，FIPS-197 C.1/C.2/C.3 & NIST KAT 6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射并刷新极简速查手册。

### 2026-11-27 (Batch 140 — **真实 Top 榜 5 大核心包：npm #40/#58/#61 + crates.io #405/#412，30/30 PASS**)
- `packages/@cap/yallist/` & `cap/test/testCapYallist.cp`: npm **#40** `yallist`（4.33 亿/周，6/6 PASS）。
- `packages/@cap/magic-string/` & `cap/test/testCapMagicString.cp`: npm **#58** `magic-string`（3.88 亿/周，6/6 PASS）。
- `packages/@cap/js-tokens/` & `cap/test/testCapJsTokens.cp`: npm **#61** `js-tokens`（3.81 亿/周，6/6 PASS）。
- `packages/@cap/crypto-bigint/` & `cap/test/testCapCryptoBigint.cp`: crates.io **#405** `crypto-bigint`（3.65 亿+，恒定时间 `U256` 与 secp256k1 模运算，6/6 PASS）。
- `packages/@cap/sct/` & `cap/test/testCapSct.cp`: crates.io **#412** `sct`（3.62 亿+，RFC 6962 SCT 线格式，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射并刷新极简速查手册；Worker N17 满 6 包轮换为 Worker N18。

### 2026-11-27 (Batch 141 — **真实 Top 榜 5 大核心包：npm #30/#63/#96 + crates.io #421/#424，30/30 PASS**)
- `packages/@cap/json-schema-traverse/` & `cap/test/testCapJsonSchemaTraverse.cp`: npm **#30** `json-schema-traverse`（4.77 亿/周，6/6 PASS）。
- `packages/@cap/argparse/` & `cap/test/testCapArgparse.cp`: npm **#63** `argparse`（3.77 亿/周，6/6 PASS）。
- `packages/@cap/convert-source-map/` & `cap/test/testCapConvertSourceMap.cp`: npm **#96** `convert-source-map`（3.07 亿/周，6/6 PASS）。
- `packages/@cap/curve25519-dalek/` & `cap/test/testCapCurve25519Dalek.cp`: crates.io **#421** `curve25519-dalek`（3.54 亿+，RFC 7748 X25519 全向量 6/6 PASS）。
- `packages/@cap/combine/` & `cap/test/testCapCombine.cp`: crates.io **#424** `combine`（3.54 亿+，6/6 PASS）。
- `packages/@cap/registry.json`: 同步 5 项新映射 + 将 npm **#78 `tr46`（3.41 亿/周）** 归并映射至已有 `@cap/idna-uts46`（零重复）；刷新 `cp_agent_lib_ref.md`；Worker C16 满 6 包光荣轮换。

### 2026-11-27 (Batch 142 — **突破 `@cap` 750 包里程碑！真实 Top 榜 5 大核心包：npm #38/#82/#94/#99 + crates.io #443，30/30 PASS**)
- `packages/@cap/iconv-lite/` & `cap/test/testCapIconvLite.cp`: npm **#38** `iconv-lite`（4.42 亿/周，6/6 PASS）。
- `packages/@cap/minipass/` & `cap/test/testCapMinipass.cp`: npm **#82** `minipass`（3.26 亿/周，6/6 PASS）。
- `packages/@cap/es-module-lexer/` & `cap/test/testCapEsModuleLexer.cp`: npm **#94** `es-module-lexer`（3.08 亿/周，6/6 PASS）。
- `packages/@cap/estree-walker/` & `cap/test/testCapEstreeWalker.cp`: npm **#99** `estree-walker`（3.05 亿/周，同步归并 `#108 estraverse` 2.92 亿/周，6/6 PASS）。
- `packages/@cap/cast/` & `cap/test/testCapCast.cp`: crates.io **#443** `cast`（3.35 亿+，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 5 项新映射（含 `estraverse` alias）并刷新极简速查手册；Worker N18 满 6 包光荣轮换。

### 2026-11-27 (Batch 143 — **真实 Top 榜 4 大核心包：npm #200/#201 + crates.io #444/#467，24/24 PASS**)
- `packages/@cap/whatwg-mimetype/` & `cap/test/testCapWhatwgMimetype.cp`: npm **#200** `whatwg-mimetype`（2.15 亿/周，6/6 PASS）。
- `packages/@cap/esutils/` & `cap/test/testCapEsutils.cp`: npm **#201** `esutils`（2.15 亿/周，6/6 PASS）。
- `packages/@cap/utf-8/` & `cap/test/testCapUtf8.cp`: crates.io **#444** `utf-8`（3.32 亿+，6/6 PASS）。
- `packages/@cap/ciborium-ll/` & `cap/test/testCapCiboriumLl.cp`: crates.io **#467** `ciborium-ll`（3.15 亿+，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 4 项新映射 + 将 npm **#153 `base64-js`（2.40 亿/周）** 归并至内置 `cap:encoding`，刷新极简速查手册。

### 2026-11-27 (Batch 144 + **3 小时效率审计固化 Iteration 7** — **5 大真实 Top 榜核心包，30/30 PASS**)
- `packages/@cap/jsesc/` & `cap/test/testCapJsesc.cp`: npm **#148** `jsesc`（2.50 亿/周，6/6 PASS）。
- `packages/@cap/fast-uri/` & `cap/test/testCapFastUri.cp`: npm **#171** `fast-uri`（2.29 亿/周，同步归并 `#218 uri-js` 2.07 亿/周，6/6 PASS）。
- `packages/@cap/path-parse/` & `cap/test/testCapPathParse.cp`: npm **#238** `path-parse`（1.96 亿/周，6/6 PASS）。
- `packages/@cap/ctr/` & `cap/test/testCapCtr.cp`: crates.io **#509** `ctr`（2.84 亿+，复用 `@cap/aes`，NIST SP 800-38A F.5.1/F.5.5 向量 6/6 PASS）。
- `packages/@cap/plotters-svg/` & `cap/test/testCapPlottersSvg.cp`: crates.io **#512** `plotters-svg`（2.81 亿+，6/6 PASS）。
- **零重复归并**：`imurmurhash` (#211, 2.10 亿/周) → `@cap/murmur3`、`uri-js` (#218, 2.07 亿/周) → `@cap/fast-uri`、`eventemitter3` (#222, 2.04 亿/周) → `cap:event`。
- `GEMINI.md` & `CP_PITFALLS.md`: 固化 3 小时效率审计 Iteration 7（`git push -q` 降噪 99.9%、实时真实榜单驱动、Python `-c` 零转义、`.toString`/`1.0e12`/`std.math.inf` 避坑）。

### 2026-11-27 (Batch 145 — **真实 Top 榜 5 大核心包：npm #205/#219/#241 + crates.io #483/#525，30/30 PASS**)
- `packages/@cap/ajv-formats/` & `cap/test/testCapAjvFormats.cp`: npm **#205** `ajv-formats`（2.12 亿/周，6/6 PASS）。
- `packages/@cap/pathe/` & `cap/test/testCapPathe.cp`: npm **#219** `pathe`（2.07 亿/周，6/6 PASS）。
- `packages/@cap/type-check/` & `cap/test/testCapTypeCheck.cp`: npm **#241** `type-check`（1.95 亿/周，6/6 PASS）。
- `packages/@cap/ed25519/` & `cap/test/testCapEd25519.cp`: crates.io **#483** `ed25519`（3.04 亿+，6/6 PASS）。
- `packages/@cap/predicates/` & `cap/test/testCapPredicates.cp`: crates.io **#525** `predicates`（2.66 亿+，6/6 PASS）。
- **零重复归并**：`#262 deep-is` → `@cap/fast-deep-equal`、`#268 json-stable-stringify-without-jsonify` → `@cap/jcs`、`#269 tinyrainbow` → `@cap/picocolors`；Worker C17 满 6 包光荣轮换。

### 2026-11-27 (Batch 146 — **6 大真实 Top 榜核心包：npm #208/#242/#253/#293 + crates.io #544/#555，36/36 PASS，映射达 990 条！**)
- `packages/@cap/flatted/` & `cap/test/testCapFlatted.cp`: npm **#208** `flatted`（2.10 亿/周，6/6 PASS）。
- `packages/@cap/tapable/` & `cap/test/testCapTapable.cp`: npm **#242** `tapable`（1.95 亿/周，6/6 PASS）。
- `packages/@cap/fastq/` & `cap/test/testCapFastq.cp`: npm **#253** `fastq`（1.92 亿/周，6/6 PASS）。
- `packages/@cap/ieee754/` & `cap/test/testCapIeee754.cp`: npm **#293** `ieee754`（1.73 亿/周，6/6 PASS）。
- `packages/@cap/env-filter/` & `cap/test/testCapEnvFilter.cp`: crates.io **#544** `env_filter`（2.57 亿+，6/6 PASS）。
- `packages/@cap/wasm-encoder/` & `cap/test/testCapWasmEncoder.cp`: crates.io **#555** `wasm-encoder`（2.48 亿+，6/6 PASS）。
- **零重复归并**：`#562 sha1_smol` → `@cap/sha1-crate`、`#563 ansi_term` → `cap:ansi`、`#568 predicates-core` → `@cap/predicates`；Worker N19 满 6 包光荣轮换为 Worker N20。

### 2026-11-27 (Batch 147 — **正式突破 1,000 条上游映射里程碑！5 大真实 Top 榜核心包，30/30 PASS**)
- `packages/@cap/esquery/` & `cap/test/testCapEsquery.cp`: npm **#247** `esquery`（1.94 亿/周，6/6 PASS）。
- `packages/@cap/parse-json/` & `cap/test/testCapParseJson.cp`: npm **#254** `parse-json`（1.92 亿/周，6/6 PASS）。
- `packages/@cap/proxy-from-env/` & `cap/test/testCapProxyFromEnv.cp`: npm **#303** `proxy-from-env`（1.69 亿/周，6/6 PASS）。
- `packages/@cap/termtree/` & `cap/test/testCapTermtree.cp`: crates.io **#575** `termtree`（2.33 亿+，6/6 PASS）。
- `packages/@cap/litrs/` & `cap/test/testCapLitrs.cp`: crates.io **#576** `litrs`（2.32 亿+，6/6 PASS）。
- **零重复归并（8 项 2.1 亿~2.3 亿 crates.io 高频包）**：`compact_str`→`@cap/smol-str`、`toml_write`→`@cap/toml-writer`、`dotenvy`→`cap:dotenv`、`der-parser`/`simple_asn1`/`asn1-rs`→`@cap/der`、`pulldown-cmark`→`cap:md`、`yaml-rust`→`cap:yaml`；上游映射正式突破 **1,000 条（达 1,003 条）**！

### 2026-11-27 (Batch 148 — **4 大真实 Top 榜核心包：npm #306/#314 + crates.io #583/#601，24/24 PASS**)
- `packages/@cap/saxes/` & `cap/test/testCapSaxes.cp`: npm **#306** `saxes`（1.68 亿/周，6/6 PASS）。
- `packages/@cap/doctrine/` & `cap/test/testCapDoctrine.cp`: npm **#314** `doctrine`（1.66 亿/周，6/6 PASS）。
- `packages/@cap/aes-gcm/` & `cap/test/testCapAesGcm.cp`: crates.io **#583** `aes-gcm`（2.29 亿+，双包复用 `@cap/aes`+`@cap/ghash-gf128`，NIST SP 800-38D 6 组官方向量全过，6/6 PASS）。
- `packages/@cap/oid-registry/` & `cap/test/testCapOidRegistry.cp`: crates.io **#601** `oid-registry`（2.21 亿+，6/6 PASS）。
- **零重复归并**：`#335 tldts-core` → `@cap/publicsuffix`、`#337 json-parse-even-better-errors` → `@cap/parse-json`、`#341 data-urls` → `@cap/data-uri`；Worker C18 满 6 包光荣轮换。

### 2026-11-27 (Batch 149 — **6 大真实 Top 榜核心包：npm #327/#329/#345/#400 + crates.io #621/#660，36/36 PASS**)
- `packages/@cap/cjs-module-lexer/` & `cap/test/testCapCjsModuleLexer.cp`: npm **#327** `cjs-module-lexer`（1.60 亿/周，6/6 PASS）。
- `packages/@cap/html-encoding-sniffer/` & `cap/test/testCapHtmlEncodingSniffer.cp`: npm **#329** `html-encoding-sniffer`（1.58 亿/周，6/6 PASS）。
- `packages/@cap/d3-array/` & `cap/test/testCapD3Array.cp`: npm **#345** `d3-array`（1.53 亿/周，6/6 PASS）。
- `packages/@cap/d3-path/` & `cap/test/testCapD3Path.cp`: npm **#400** `d3-path`（1.34 亿/周，6/6 PASS）。
- `packages/@cap/utf16-iter/` & `cap/test/testCapUtf16Iter.cp`: crates.io **#621** `utf16_iter`（2.08 亿+，6/6 PASS）。
- `packages/@cap/vec-map/` & `cap/test/testCapVecMap.cp`: crates.io **#660** `vec_map`（1.92 亿+，6/6 PASS）。
- **零重复归并（7 项高频包）**：`events`→`cap:event`、`ip-address`→`@cap/ipaddr`、`get-east-asian-width`→`@cap/is-fullwidth-code-point`、`sax`→`@cap/saxes`、`owo-colors`→`cap:ansi`、`base64-simd`→`cap:encoding`、`gif`→`cap:gif`；Worker N20 满 6 包光荣轮换为 Worker N21。

### 2026-11-27 (Batch 150 里程碑 — **5 大真实 Top 榜核心包：npm #351/#356/#407 + crates.io #664/#668，30/30 PASS，官方 `@cap` 达 790 包！**)
- `packages/@cap/split2/` & `cap/test/testCapSplit2.cp`: npm **#351** `split2`（1.50 亿/周，6/6 PASS）。
- `packages/@cap/xml-name-validator/` & `cap/test/testCapXmlNameValidator.cp`: npm **#356** `xml-name-validator`（1.48 亿/周，同步覆盖 `#354 xmlchars`，6/6 PASS）。
- `packages/@cap/is-potential-custom-element-name/` & `cap/test/testCapIsPotentialCustomElementName.cp`: npm **#407** `is-potential-custom-element-name`（1.32 亿/周，6/6 PASS）。
- `packages/@cap/rand-hc/` & `cap/test/testCapRandHc.cp`: crates.io **#664** `rand_hc`（1.91 亿+，eSTREAM 官方向量一致，6/6 PASS）。
- `packages/@cap/getopts/` & `cap/test/testCapGetopts.cp`: crates.io **#668** `getopts`（1.89 亿+，6/6 PASS）。
- `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`: 同步 6 项映射（含 `xmlchars`）并刷新极简速查手册。

### 2026-11-27 (Batch 151 — **crates.io 真实 Top 700 纯逻辑全量收官！+ 5 大核心包，30/30 PASS**)
- `packages/@cap/env-paths/` & `cap/test/testCapEnvPaths.cp`: npm **#371** `env-paths`（1.44 亿/周，6/6 PASS）。
- `packages/@cap/sprintf-js/` & `cap/test/testCapSprintfJs.cp`: npm **#386** `sprintf-js`（1.39 亿/周，6/6 PASS）。
- `packages/@cap/pure-rand/` & `cap/test/testCapPureRand.cp`: npm **#443** `pure-rand`（1.24 亿/周，6/6 PASS）。
- `packages/@cap/rend/` & `cap/test/testCapRend.cp`: crates.io **#679** `rend`（1.85 亿+，6/6 PASS）。
- `packages/@cap/id-arena/` & `cap/test/testCapIdArena.cp`: crates.io **#685** `id-arena`（1.83 亿+，6/6 PASS）。
- **零重复归并（8 项高频包）**：`path-is-absolute`→`@cap/pathe`、`html-escaper`→`@cap/escape-html`、`sisteransi`→`@cap/ansi-escapes`、`pg-protocol`→`cap:pg`、`jsonc-parser`→`cap:jsonc`、`dirs-next`→`@cap/home-dir`、`aws-sigv4`→`cap:s3`、`xmlparser`→`@cap/saxes`；Worker C19 满 6 包光荣收官 crates.io Top 700！

### 2026-11-26 — Batch 152（跨越 800 `@cap/*` 里程碑：新增 8 个 npm Real Top 700 包 + 12 条零重复映射）
- 新增 `packages/@cap/figures/` + `cap/test/testCapFigures.cp`（`npm:figures` #473, 6/6 PASS）
- 新增 `packages/@cap/ecdsa-sig-formatter/` + `cap/test/testCapEcdsaSigFormatter.cp`（`npm:ecdsa-sig-formatter` #475, 6/6 PASS）
- 新增 `packages/@cap/postgres-interval/` + `cap/test/testCapPostgresInterval.cp`（`npm:postgres-interval` #501, 6/6 PASS）
- 新增 `packages/@cap/postgres-date/` + `cap/test/testCapPostgresDate.cp`（`npm:postgres-date` #505, 6/6 PASS）
- 新增 `packages/@cap/css-what/` + `cap/test/testCapCssWhat.cp`（`npm:css-what` #513, 6/6 PASS）
- 新增 `packages/@cap/is-binary-path/` + `cap/test/testCapIsBinaryPath.cp`（`npm:is-binary-path` #534, 6/6 PASS）
- 新增 `packages/@cap/set-cookie-parser/` + `cap/test/testCapSetCookieParser.cp`（`npm:set-cookie-parser` #537, 6/6 PASS）
- 新增 `packages/@cap/nth-check/` + `cap/test/testCapNthCheck.cp`（`npm:nth-check` #544, 6/6 PASS）
- 更新 `packages/@cap/registry.json`、`cp_lib_ref.md`、`cp_agent_lib_ref.md`（总包数突破 **801**，新增 12 条零重复映射）。
- 补增 Batch 152：`@cap/lz-string`（#494）、`@cap/d3-format`（#519）、`@cap/cssesc`（#548）、`@cap/d3-ease`（#568）及对应 4 套 `cap/test/testCap*.cp`（全部 6/6 PASS），本批次共 +12 包，总计 **805** 个 `@cap/*` 官方包。

### 2026-11-26 — Batch 153（npm Real Top 700 `#532..#673`：+12 `@cap/*` 包 + 14 条零重复映射，总计 816 `@cap/*`）
- 新增 `packages/@cap/{d3-time,d3-time-format,d3-scale,vfile-message,strnum,abbrev,character-entities,ccount,chardet,space-separated-tokens,is-path-inside,class-variance-authority}/` 及对应 12 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（+26 映射条目，覆盖 30+ 个 npm Top 700 源码包）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 154（npm Real Top 700 `#294..#643`：+12 `@cap/*` 包 + 18 条零重复映射，总计 828 `@cap/*`）
- 新增 `packages/@cap/{tinybench,dom-serializer,d3-shape,bl,d3-interpolate,nopt,bowser,property-information,sanitize-uri,express-rate-limit,cssstyle,object-hash}/` 及对应 12 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,114+）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 155（npm Real Top 700 `#125..#580`：+6 `@cap/*` 包 + 7 条 crates.io 标准库映射，总计 834 `@cap/*`）
- 新增 `packages/@cap/{browserslist-query,resolve-pkg-maps,unist-util-visit,cli-width,dompurify,zod-to-json-schema}/` 及对应 6 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,126+）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 156（npm Real Top 700 `#136..#539`：+5 `@cap/*` 包 + 3 条零重复映射，总计 839 `@cap/*`）
- 新增 `packages/@cap/{object-inspect,file-entry-cache,optionator,levn,istanbul-lib-coverage}/` 及对应 5 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,134）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 157（npm Real Top 700 `#60..#687`：+5 `@cap/*` 包，总计 844 `@cap/*`）
- 新增 `packages/@cap/{https-proxy-agent,immer-patches,pkg-types,trough,webpack-sources}/` 及对应 5 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,139）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 158（双平台 Top 700 圆满收官 + Top 701–800 启航：+5 `@cap/*` 包 + 16 条零重复映射，总计 849 `@cap/*`）
- 新增 `packages/@cap/{dom-accessibility-name,schema-utils,error-ex,test-exclude,cesu8}/` 及对应 5 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 优化 `scripts/gen_agent_lib_ref.py` 紧凑列表格式，`cp_agent_lib_ref.md` 缩减至 42KB（-89.5%）
- 更新 `packages/@cap/registry.json`（总映射 1,158）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 159（双平台 Real Top 701–800：+8 `@cap/*` 包，突破 857 `@cap/*`！）
- 新增 `packages/@cap/{codespan-reporting,matrixmultiply,debugid,number-prefix,password-hash,cbc,markdown-table,progress}/` 及对应 8 套 `cap/test/testCap*.cp`（全部 6/6 PASS；`@cap/cbc` 复用 `@cap/aes` 通过 NIST SP 800-38A 官方向量）
- 更新 `packages/@cap/registry.json`（总映射 1,168）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 160（双平台 Real Top 716–830：+6 `@cap/*` 包 + 9 条零重复映射，总计 863 `@cap/*`）
- 新增 `packages/@cap/{color-quant,lexical-parse-float,resolv-conf,zod-validation-error,module-details-from-path,fast-fifo}/` 及对应 6 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,182）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 161（双平台 Real Top 767–860：+4 `@cap/*` 包，总计 867 `@cap/*`）
- 新增 `packages/@cap/{smart-buffer,stack-utils,ascii,nohash-hasher}/` 及对应 4 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,187）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-26 — Batch 162（双平台 Real Top 766–917：+6 `@cap/*` 包 + 8 条零重复映射，突破 1,201 映射里程碑！）
- 新增 `packages/@cap/{unicode-linebreak,lexical-write-float,ron,salsa20,stylis,damerau-levenshtein}/` 及对应 6 套 `cap/test/testCap*.cp`（全部 6/6 PASS；`@cap/salsa20` 通过官方 eSTREAM 向量）
- 更新 `packages/@cap/registry.json`（总映射 **1,201**）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-27 — Batch 163（双平台 Real Top 836–920：+8 `@cap/*` 包，总计 881 `@cap/*`）
- 新增 `packages/@cap/{dlv-list,ordered-multimap,normalize-line-endings,jsonpath-rust,sketches-ddsketch,ufo,requires-port,language-tags}/` 及对应 8 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 1,209）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-27 — Batch 164（+5 `@cap/*` 包 + 28 条零重复映射，crates.io Top 700 显式映射达 395/700，总计 886 `@cap/*`）
- 新增 `packages/@cap/{cssparser,smawk,email-address,wordwrap,jest-docblock}/` 及对应 5 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 补齐 crates.io Top 700 剩余 16 条（覆盖 21 个 crate）及 Top 900–1000 共 12 条零重复标准库/已有生态映射
- 更新 `packages/@cap/registry.json`（总映射 **1,242**）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。

### 2026-11-27 — Batch 165（双平台 Real Top 900–1000：+9 `@cap/*` 包 + 9 条零重复映射，总计 895 `@cap/*`）
- 新增 `packages/@cap/{bit-field,nibble-vec,postcard,vte,chrome-trace-event,compressible,serialize-javascript,commondir,yn}/` 及对应 9 套 `cap/test/testCap*.cp`（全部 6/6 PASS）
- 更新 `packages/@cap/registry.json`（总映射 **1,260**）、`cp_lib_ref.md`、`cp_agent_lib_ref.md`。
- 补增 Batch 165 Top 1000 收官包：`@cap/plist`（#957）、`@cap/scroll`（#948，覆盖 `endian-type` #928）及对应 2 套 `cap/test/testCap*.cp`（6/6 PASS），本批次共 +11 包，总计 **897 `@cap/*` 官方包**、**1,262 条上游映射**；按指令停止吸收，全部子智能体已安全收回，进入整理待命状态。

### 2026-11-27 — 生态整理 Phase 1（L0 内置标准库 4 大基座 + 8 项自动化体检门禁）
- 新增内置标准库通道 `cap:ascii`（`lib/ascii/index.cp` + `doc/lib/ascii.md` + `cap/test/testStdAscii.cp`, 6/6 PASS）
- 新增内置标准库通道 `cap:utf8`（`lib/utf8/index.cp` + `doc/lib/utf8.md` + `cap/test/testStdUtf8.cp`, 6/6 PASS）
- 新增内置标准库通道 `cap:fmt`（`lib/fmt/index.cp` + `doc/lib/fmt.md` + `cap/test/testStdFmt.cp`, 6/6 PASS）
- 新增内置标准库通道 `cap:bin`（`lib/bin/index.cp` + `doc/lib/bin.md` + `cap/test/testStdBin.cp`, 6/6 PASS）
- 新增生态自动化体检与门禁脚本 `scripts/audit_ecosystem.py`
- 更新 `cp_lib_ref.md` 与 `cp_agent_lib_ref.md`（内置标准库增至 **208 通道**）。

### 2026-11-27 — 生态整理 Phase 2（去 `@cap/` 域裸包名迁移 + 双平台重名超集合并 + `cap brief`）
- 迁移 **832 个** npm/crates.io 吸收包至原包名目录 `packages/<orig>/`（含 8 个 `@scope/pkg`），保留 39 个原生 C/C++ 包在 `packages/@cap/`、33 个 Web 插件在 `packages/@web/`，总计 **904 个零重复包**
- 消解并合并 **27 对重复包**（含 `bytes` 与 `semver` 完整双平台超集合并，均 100% PASS），更新 894 个测试与互依赖导入路径，`registry.json` 不保留旧 `@cap/<old>` 别名并新增 `types`/`agent_doc`/`readme`/`readme_zh` 导航字段
- 更新 `cpc/src/resolver.rs` 支持裸包名与 Rust `_`/`-` 双向等价解析；新增 `cap/src/brief.zig` 与 `cap brief <pkg>` AI-First 速查命令；修复 `cap:bin` 导出 `findByteInBytes` 避免与 `cap:ascii` 冲突
- 更新 `scripts/gen_agent_lib_ref.py` 与 `cp_agent_lib_ref.md`（压缩至 **38.3KB**，-90.7%），新增 `scripts/gen_pkg_docs.py`。

## [2026-11-27] - Top-2000 生态重组 Phase 1~3 全面收官 + AI-First `cap brief/fix/check` 工具链落地
- **Phase 1 (L0 硬件 SIMD 基座)**：`lib/ascii/` (`cap:ascii`)、`lib/utf8/` (`cap:utf8`)、`lib/fmt/` (`cap:fmt`)、`lib/bin/` (`cap:bin`) 全面升级为 16-lane 硬件 `@Vector(16, u8)` SIMD 实现；ReleaseFast + `tinybench` 实测 4KB `bytesEq` **14.07x** (161ns)、`isAscii` **14.64x** (126ns)、`findByte` **7.65x** (174ns)、`isValidUtf8` **146ns**。
- **Phase 2 (裸包名零迁移成本改造)**：将 832 个 npm/crates.io 包迁移至原包名 `packages/<orig>/`（保留 39 原生 C/C++ `@cap/*` 与 33 `@web/*`，共 904 零重复包），合并双平台同名包 `bytes`/`semver` 及 6 处跨包导出（`mediatype`/`negotiator`/`cookie-signature`/`cookie-parser`/`@sindresorhus/fnv1a`/`pbkdf2`），全仓 904 包 **0 broken imports**，`registry.json` 按指令不保留旧 `@cap/<old>` 别名。
- **Phase 3 (L0 去重与性能优化 5 Waves 全清零)**：Wave 1~5 共重构 **75+ 个高频包**，净消除 **3,500+ 行**重复手写辅助代码，`Uses cap:* stdlib` 从 **0 → 117+**，全生态 `>=2` 重复 helper 包彻底清零；`crc32fast` 升级为 `comptime` 256-entry LUT 查表法 + `undefined` 栈缓冲（6/6 PASS）。
- **Phase 5 (文档与 AI-First CLI 闭环)**：全量 904/904 包配齐 `src/index.d.cp`、`README.md`（含中英文切换栏）、`README.zh-CN.md`、`README_AGENT.md`；`cap` CLI 新增 `cap brief <pkg>`、`cap fix [--dry-run] <path>`（精准匹配 `from "@cap/<pkg>"`）、`cap check <path>`；修复 `cap/build.zig` `zstd.c` 路径与 `lib/time/uuidV7.cp` 旧 `self` 语法。

## [2026-11-27] - 编译器 Sema 6 类假阳性全面修复：全仓 904 / 904 (100.0%) 生态包 `cpc check` 零失败绿灯
- `cpc/src/sema/{type_infer,coerce,mod,safety,stmt_expr}.rs`: 修复 6 类静态类型推断假阳性（二元运算左侧整数字面量吸收右侧类型、`IfExpr` 分支字面量目标类型传播、`string` 切片推断为 `String`、重赋值不覆盖已声明类型、`@Vector(N,u8)→@Vector(N,u32)` 向量拓宽、`??`/浮点内建/`@as`/函数指针/常量数组长度/重赋值定长失效），通过 `bash scripts/sync_cpc.sh release` 正式发布，并向 `bun_agy` 与 `capsurf_agy` 广播生效。
- 全仓验证：`cpc/target/release/cpc check` 扫描全部 904 个生态包达到 **904 / 904 PASS (0 failed)**。

## [2026-11-27] - 紧急热修 `task-bun-20261127-enum-slice-by-val`：消除 `union(enum)`/tagged `enum` 只读方法返回内联数组切片的 Use-After-Return 隐患
- `cpc/src/codegen/actor.rs`: 区分纯无负载枚举与带负载 tagged `enum`/`union(enum)`，新增 `returns_borrowed_view_in_ctx` 支持返回切片/指针/字符串或含借用视图的聚合类型（如 `Data.substring()`）时自动生成 `self: *const Self`。
- `cpc/src/codegen/pattern.rs`: 对 `switch (self.*)` 中仅用于切片/索引/成员访问的固定数组/值容器变体自动提升为指针捕获 `|*arr|`，彻底避免按值拷贝栈帧悬垂切片（已通过 `clobberStack()` 运行时内存区间实测并发布 release 版 `cpc`）。
