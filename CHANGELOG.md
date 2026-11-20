# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-20] - `cap:http` Web/AI Streaming 原语（+1,713 行）与 `cap:logger` 结构化日志/W3C Trace 追踪（+1,671 行）双路落地

### `cap:http` 生产级 Web / AI Streaming 协议原语升级 (`lib/http/`，0 `std.posix`，0 `@import("bun")`)
- 新增 [`lib/http/sse.cp`](file:///data/dev/cp/lib/http/sse.cp)（486 行）：WHATWG Server-Sent Events / LLM 流式解析器与编码器（`SseEvent`、`SseParser` 跨 chunk 切分/BOM 剥离/多行 `data:` 拼接/`[DONE]` 识别/单事件大小 DoS 上限防护、`SseEncoder`）；
- 新增 [`lib/http/multipart.cp`](file:///data/dev/cp/lib/http/multipart.cp)（417 行）：RFC 7578 `multipart/form-data` 二进制安全打包与解析器（`MultipartBoundary`、`MultipartBuilder`、`MultipartParser`）；
- 升级 [`lib/http/cookie.cp`](file:///data/dev/cp/lib/http/cookie.cp)（810 行）：保留原有 `CookieMap` 兼容，新增 RFC 6265 `SameSite`/`HttpCookie`/`CookieCodec`（`Set-Cookie` 与 IMF-fixdate 编解码）及 `HttpCookieJar`（域名后缀与公共后缀边界防护、路径前缀匹配、`Secure`/过期过滤）；
- 修复 [`lib/http/decompressor.cp`](file:///data/dev/cp/lib/http/decompressor.cp) 模块化模式缺失 `WINDOW_BITS_AUTO` 显式导入，打通 `cap:http` 全量 `-m` 编译；增量导出至 [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp)，更新 [`docs/http/http.md`](file:///data/dev/cp/docs/http/http.md)；新增 [`cap/test/test_http_web_primitives_deep.cp`](file:///data/dev/cp/cap/test/test_http_web_primitives_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有 HTTP 回归 14/14 零回归）。

### `cap:logger` 结构化日志与 W3C Trace Context / OpenTelemetry 追踪升级 (`lib/logger/`，0 `std.posix`，0 `@import("bun")`)
- 新增 [`lib/logger/structured.cp`](file:///data/dev/cp/lib/logger/structured.cp)（713 行）：`StructuredLogger`/`LogFormatter`（RFC 8259 JSON 转义、Logfmt、PrettyAnsi）、`withField` 子 Logger、可插拔 Sink 及不区分大小写敏感字段自动脱敏（`password`/`token`/`authorization`/`secret` -> `"[REDACTED]"`）；
- 新增 [`lib/logger/trace.cp`](file:///data/dev/cp/lib/logger/trace.cp)（691 行）：W3C Trace Context Level 1 `TraceParent`（严格校验/全零拒绝/子 Span 派生）、`TraceState` 及 `Span`/`SpanEvent`/`SpanStatus`（含 OTel JSON 导出）；
- 新增 [`lib/logger/sampler.cp`](file:///data/dev/cp/lib/logger/sampler.cp)（267 行）：`RatioSampler`（FNV-1a+SplitMix64 确定性采样）、`BurstSampler`（防日志风暴降采样）与零堆 `RingLogBuffer<CAP>`；
- 增量导出至 [`lib/logger/index.cp`](file:///data/dev/cp/lib/logger/index.cp)，更新 [`docs/logger/logger.md`](file:///data/dev/cp/docs/logger/logger.md)；新增 [`cap/test/test_logger_production_deep.cp`](file:///data/dev/cp/cap/test/test_logger_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有 `test_shell_napi_elf_logger_deep.cp` 4/4 零回归）。
