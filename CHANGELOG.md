# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:oauth2` 纯 CP RFC 6749/7636 OAuth 2.0、PKCE S256 与 OIDC 协议引擎（支持 `cap.oauth2` 自动动态挂载，详见 `doc/lib/oauth2.md`）（5/5 PASS，自动动态挂载 `cap.oauth2`）
- **新增标准库**：`lib/oauth2/{types,pkce,authorize,token,index}.cp`，实现拒绝采样 CSPRNG PKCE `S256`、常量时间 `state`/Challenge 校验、RFC 3986 授权 URL 与回调解析、三大 Grant/Revocation 表单编码及 `TokenResponse` 解析。
- **新增文档与测试**：`doc/lib/oauth2.md` 扁平文档与 `cap/test/test_oauth2_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:histogram` 纯 CP HDR 延迟直方图与分位统计引擎（5/5 PASS，自动动态挂载 `cap.histogram`）
- **新增标准库**：`lib/histogram/{hdr,ewma,export,index}.cp`，实现 Gil Tene 对数-线性子分桶 `HdrHistogram`（1..5 位有效数字精度、`recordValueWithExpectedInterval`、`merge`、P50/P95/P99/P99.9/P99.99）、`Ewma`（1m/5m/15m）、固定环形槽位 `SlidingWindowCounter` 及 `formatSummary`/`formatPrometheusBuckets` 导出。
- **新增文档与测试**：`doc/lib/histogram.md` 扁平文档与 `cap/test/test_histogram_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:unicode` 纯 CP UTF-8/UTF-16、UAX #29 Grapheme 与终端显示宽度引擎（5/5 PASS，自动动态挂载 `cap.unicode`）
- **新增标准库**：`lib/unicode/{utf,props,grapheme,width,index}.cp`（吸收对标 Bun `src/unicode/`），实现 UTF-8/UTF-16 代理对编解码、Unicode 空白/标识符/大小写属性、零分配 `GraphemeIterator`（ZWJ/国旗/组合音标/CRLF）及跳过 ANSI 转义的 `displayWidth`/`truncateByWidth`。
- **新增文档与测试**：`doc/lib/unicode.md` 扁平文档与 `cap/test/test_unicode_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:picohttp` 纯 CP 零堆分配 HTTP/1.1 & Chunked 流式解析引擎（5/5 PASS，自动动态挂载 `cap.picohttp`）
- **新增标准库**：`lib/picohttp/{picohttp,index}.cp`，基于 caller-provided 缓冲实现 RFC 9110/9112 `parseRequest`/`parseResponse`/`parseHeaders`、`ChunkedDecoder`（支持任意 `1..N` 字节半包跨 chunk、`chunk-ext` 与 Trailer 跳过）以及 `findHeader`/`parseContentLength` 冲突校验。
- **新增文档与测试**：`doc/lib/picohttp.md` 扁平文档与 `cap/test/test_picohttp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:interchange` 纯 CP JSON/JSONC/JSON5/TOML/YAML 统一数据交换引擎（5/5 PASS，自动动态挂载 `cap.interchange`）
- **新增标准库**：`lib/interchange/{value,json5,bridge,interchange,index}.cp`（吸收对标 Bun `src/interchange/`），提供统一 `InterchangeValue`（含 `max_depth = 64`、`getPath`、`clone`、`equals`）、原生 JSON5/JSONC 解析序列化，并桥接复用 `cap:toml`/`cap:yaml` 实现五格式统一 `parse`/`parseAuto`/`stringify`/`convert`。
- **新增文档与测试**：`doc/lib/interchange.md` 扁平文档与 `cap/test/test_interchange_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:valkey` 纯 CP RESP2/RESP3 Valkey/Redis 协议编解码与增量流式解析引擎（5/5 PASS，自动动态挂载 `cap.valkey`）
- **新增标准库**：`lib/valkey/{valkey,index}.cp`（吸收对标 Bun `src/valkey/`），完整建模 RESP2/RESP3 14 种核心类型（含 `Map`/`Set`/`Push`/`VerbatimString`/`BigNumber`/`Double` 与 `Attribute` 跳过），提供 `RespStreamParser` 增量半包黏包解析、`encodeCommand`/`encodeValue` 及 `max_depth`/`max_bulk_len` 防资源耗尽守卫。
- **新增文档与测试**：`doc/lib/valkey.md` 扁平文档与 `cap/test/test_valkey_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

