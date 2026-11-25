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
