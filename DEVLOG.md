## 2026-11-22: 新建 `cap:http_vary` RFC 9110 §12.5.5 Vary 响应头零分配迭代、通配符检测与大小写不敏感去重合并器（5/5 PASS，文档直写 `doc/lib/http_vary.md`）
主智能体吸收 `cap:http_vary`，确保 `*` 出现时自动塌缩为 `"*"`，并避免大小写重复字段污染缓存键。

## 2026-11-22: 新建 `cap:hex_dump` 零分配十六进制编解码与规范化 hexdump -C 多行诊断格式化器（5/5 PASS，文档直写 `doc/lib/hex_dump.md`）
Subagent 2 吸收 `cap:hex_dump`，零堆分配实现大小写编码、分隔符容错解码与 8+8 分组 ASCII 侧栏转储。

## 2026-11-22: 新建 `cap:http_prefer` RFC 7240 Prefer 与 Preference-Applied HTTP 头零分配解析器与格式化器（5/5 PASS，文档直写 `doc/lib/http_prefer.md`）
主智能体吸收 `cap:http_prefer`，支持引号内逗号参数切分与 RFC 7240 §2 首现优先生效规则。

## 2026-11-22: 新建 `cap:mime_sniff` WHATWG MIME Sniffing 魔数嗅探引擎（图像/音视频/归档可执行/HTML-XML-JSON/UTF-8 文本与二进制判定）（5/5 PASS，文档直写 `doc/lib/mime_sniff.md`）
Subagent 1 吸收 `cap:mime_sniff`，覆盖 PNG/JPEG/GIF/WebP/BMP/ICO/SVG、PDF/ZIP/GZIP/ZSTD/WASM/ELF/SQLite、WAV/OGG/FLAC/MP3/MP4/WebM 及 UTF-8 BOM/过长编码校验。

## 2026-11-22: 新建 `cap:ansi_strip` 零分配 ANSI/VT100 转义序列（CSI/OSC8 超链接/DCS/2 字节 ESC）剥离与可见 UTF-8 字符计数（5/5 PASS，文档直写 `doc/lib/ansi_strip.md`）
主智能体吸收 `cap:ansi_strip`，零堆分配剥离全部常见终端转义序列并精确计算剥离后字节长与可见 UTF-8 码点数。

## 2026-11-22: 新建 `cap:shell_quote` POSIX Shell 安全单引号转义、参数拼接与支持单双引号/反斜杠/相邻拼接的命令行拆分器（5/5 PASS，文档直写 `doc/lib/shell_quote.md`）
Subagent 2 吸收 `cap:shell_quote`，支持 `joinArgs` -> `ShellArgIter` 精确往返与未闭合引号/尾部孤立反斜杠检测。

## 2026-11-22: 新建 `cap:jwt_claims` RFC 7519 JWT 分段器、Base64URL 解码器、Header/Payload 声明提取与 exp/nbf/iss/sub/aud 校验器（5/5 PASS，文档直写 `doc/lib/jwt_claims.md`）
主智能体吸收 `cap:jwt_claims`，安全跳过嵌套 JSON 对象/数组提取顶层声明，支持 `leeway_sec` 时钟漂移容差。

## 2026-11-22: 新建 `cap:robots_txt` RFC 9309 robots.txt 解析、通配符匹配、Crawl-delay 提取与 Sitemap 迭代器（5/5 PASS，文档直写 `doc/lib/robots_txt.md`）
Subagent 1 吸收 `cap:robots_txt`，采用线性双指针回溯实现 `*`/`$` 通配符匹配，支持 UTF-8 BOM、注释忽略、`Crawl-delay` 与 `SitemapIter`。

## 2026-11-22: 新建 `cap:netrc` .netrc / _netrc 认证凭据文件零分配流式解析器（支持引号、注释、macdef 与 default 回退）（5/5 PASS，文档直写 `doc/lib/netrc.md`）
Subagent 2 吸收 `cap:netrc`，零堆分配解析 `.netrc` 凭据，正确保留密码内 `#` 并跳过 `macdef` 宏体。

## 2026-11-22: 新建 `cap:hosts_file` /etc/hosts 零分配流式解析器与 IPv4/IPv6 正反向解析查找（5/5 PASS，文档直写 `doc/lib/hosts_file.md`）
主智能体吸收 `cap:hosts_file`，零堆分配迭代 `/etc/hosts`，提供 `lookupHost`/`lookupHostV4`/`lookupHostV6`/`reverseLookup`。

## 2026-11-22: 新建 `cap:ip_addr` RFC 4291 / RFC 5952 IPv4 与 IPv6 零分配解析器、规范化零压缩格式化器、地址分类与 CIDR 子网匹配（5/5 PASS，文档直写 `doc/lib/ip_addr.md`）
主智能体吸收 `cap:ip_addr`，严格拒绝 IPv4 前导零与非法双 `::`，完整实现 RFC 5952 §4.2 规范化输出与 IPv4/IPv6 CIDR 掩码匹配。

## 2026-11-22: 新建 `cap:http_cache_control` RFC 9111 Cache-Control 指令零分配流式解析器、缓存存储/有效 TTL 判定与规范化格式化器（5/5 PASS，文档直写 `doc/lib/http_cache_control.md`）
Subagent 2 吸收 `cap:http_cache_control`，支持引号内逗号与反斜杠转义、共享/私有缓存 `s-maxage` 优先级、`delta-seconds` 防溢出校验及规范化序列化。

## 2026-11-22: 新建 `cap:json_pointer` RFC 6901 JSON Pointer 解析、~0/~1 转义反转义、数组索引校验与路径段迭代器（5/5 PASS，文档直写 `doc/lib/json_pointer.md`）
Subagent 1 吸收 `cap:json_pointer`，实现单遍 `~1`/`~0` 反转义、RFC 6901 §4 数组索引校验、`PointerIter` 迭代及 `appendToken`/`parentPointer`。

## 2026-11-22: 新建 `cap:http_accept` RFC 9110 Accept / Accept-Encoding / Accept-Language 内容协商与 q 值解析器（5/5 PASS，文档直写 `doc/lib/http_accept.md`）
主智能体吸收 `cap:http_accept`，支持带引号参数切分、媒体类型特异度排序、`q=0` 精确排除覆盖通配符、BCP-47 主子标签前缀匹配。

## 2026-11-22: 新建 `cap:token_bucket` 纯整数毫令牌确定性令牌桶与固定时间窗口限流器（防时钟回拨与溢出）（5/5 PASS，文档直写 `doc/lib/token_bucket.md`）
Subagent 2 吸收 `cap:token_bucket`，采用 `tokens * 1000` 毫令牌整数模型消除浮点误差，具备时钟回拨防护与 `timeUntilReadyMs` 精确等待时长计算。

## 2026-11-22: 新建 `cap:sse_parser` WHATWG Server-Sent Events (text/event-stream) 零分配流式解析器与格式化器（5/5 PASS，文档直写 `doc/lib/sse_parser.md`）
Subagent 1 吸收 `cap:sse_parser`，内置 4096B 固定缓冲拼接多行 `data:`，完整支持 UTF-8 BOM、CRLF/CR/LF、注释忽略、`id:` NUL 拒绝与 `retry:` 校验。

## 2026-11-22: 新建 `cap:content_disposition` RFC 6266 / RFC 5987 Content-Disposition 响应头解析、文件名安全消毒与 UTF-8 filename* 格式化器（5/5 PASS，文档直写 `doc/lib/content_disposition.md`）
吸收 `cap:content_disposition`（零堆分配），支持 `filename` / `filename*` 优先级解析、路径分隔符与控制字符清洗，以及 ASCII 回退 + UTF-8 双参数 `formatAttachment`/`formatInline`。

## 2026-11-22: 新建 `cap:multipart_boundary` RFC 7578 / RFC 2046 multipart/form-data 零堆分配流式边界切分器与分片头部查找库（吸收自 bun/src/http/HTTPRequestBody.cp）（5/5 PASS，文档直写 `doc/lib/multipart_boundary.md`）
子智能体 1 实现零堆分配 multipart/form-data 流式切分与二进制安全解析，5/5 单测通过

## 2026-11-22: 新建 `cap:retry_backoff` 零堆分配指数退避重试调度器（Full/Equal Jitter + SplitMix64 确定性种子）与 RFC 9110 Retry-After 解析库（吸收自 bun/src/http/HTTPContext.cp）（5/5 PASS，文档直写 `doc/lib/retry_backoff.md`）
子智能体 2 实现指数退避/抖动调度与 RFC 9110 Retry-After 秒数/IMF-fixdate 双格式解析，5/5 单测通过

## 2026-11-22: 新建 `cap:cors_policy` WHATWG Fetch CORS 策略配置、Simple/Preflight 预检评估与凭证回显/Vary: Origin 决策引擎（吸收自 bun/src/http_types/FetchRequestMode.cp）（5/5 PASS，文档直写 `doc/lib/cors_policy.md`）
子智能体 1 实现零堆分配 WHATWG Fetch CORS 预检评估引擎，5/5 单测通过

## 2026-11-22: 新建 `cap:csv_stream` RFC 4180 CSV / TSV 零堆分配流式字段/行解析器（自定义分隔符/引号内换行/"" 转义还原）与编码器（吸收自 bun/src/bun_core/fmt.cp）（5/5 PASS，文档直写 `doc/lib/csv_stream.md`）
子智能体 2 实现完整 RFC 4180 CSV/TSV 流式解析与编码器，5/5 单测通过

## 2026-11-22: 新建 `cap:http_forwarded` RFC 7239 Forwarded 与 X-Forwarded-For 反向代理链路零堆分配解析器及客户端真实 IP 提取库（吸收自 bun/src/http/HeaderValueIterator.cp）（5/5 PASS，文档直写 `doc/lib/http_forwarded.md`）
子智能体 1 吸收 HTTP 多值头迭代模式，实现 RFC 7239 Forwarded 与 XFF 可信代理 IP 提取，5/5 单测通过

## 2026-11-22: 新建 `cap:js_ident` ECMAScript/TS 56 关键字分类、ASCII/私有字段标识符校验、属性加引号判定与 JS 字符串转义库（吸收自 bun/src/js_parser/lexer/）（5/5 PASS，文档直写 `doc/lib/js_ident.md`）
子智能体 2 吸收 js_parser 词法规则，实现 56 关键字分类与安全 JS 字符串转义，5/5 单测通过

## 2026-11-22: 新建 `cap:http_auth` RFC 9110 §11 / RFC 7617 Basic / RFC 6750 Bearer HTTP Authorization 认证头零堆分配解析与编解码库（吸收自 bun/src/http/Headers.cp）（5/5 PASS，文档直写 `doc/lib/http_auth.md`）
实现 RFC 7617 Basic Base64 凭据编解码与 Bearer/AWS4 认证方案解析，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:dotenv_parse` 零堆分配 .env 配置文件解析器（BOM/export/4 种引号/多行/行尾注释）与 ${VAR:-default} 变量展开器（吸收自 bun/src/bun_core/env.cp）（5/5 PASS，文档直写 `doc/lib/dotenv_parse.md`）
子智能体 1 吸收 bun_core/env.cp，实现完整 .env 迭代解析与 ${VAR:-default} 展开，5/5 单测通过

## 2026-11-22: 新建 `cap:mime_lookup` 65+ Web/系统高频扩展名 MIME 查表、Content-Type 头部（charset/boundary）解析与可压缩性判定库（吸收自 bun/src/http_types/{MimeType,mime_type_list_enum}.cp）（5/5 PASS，文档直写 `doc/lib/mime_lookup.md`）
子智能体 2 吸收 Bun MimeType 表，实现 65+ 扩展名零分配查表与 Content-Type 解析，5/5 单测通过

## 2026-11-22: 新建 `cap:http_link_header` RFC 8288 Web Linking HTTP Link 头部零堆分配解析、rel 查找与格式化库（分页/Preload/Early Hints，吸收自 bun/src/http/HeaderValueIterator.cp）（5/5 PASS，文档直写 `doc/lib/http_link_header.md`）
吸收 HTTP 多值头迭代模式，实现 RFC 8288 Link 头部解析与格式化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:lcov_report` LCOV 代码覆盖率报告零堆分配记录、百分比统计、阈值校验与双向解析/序列化库（吸收自 bun/src/options_types/CodeCoverageOptions.cp）（5/5 PASS，文档直写 `doc/lib/lcov_report.md`）
子智能体 1 吸收 CodeCoverageOptions LCOV 规范，实现零堆分配覆盖率记录/阈值判定与 LCOV 双向编解码，5/5 单测通过

## 2026-11-22: 新建 `cap:ansi_style` ANSI 终端 16 色/样式/24-bit TrueColor 格式化、CSI & OSC 8 超链接剥离与可见宽度计算库（吸收自 bun/src/bun_core/output.cp）（5/5 PASS，文档直写 `doc/lib/ansi_style.md`）
子智能体 2 吸收 bun/src/bun_core/output.cp，实现 16 色/TrueColor 样式渲染与 CSI/OSC 剥离，5/5 单测通过

## 2026-11-22: 新建 `cap:http_date` RFC 9110 §5.6.7 HTTP-date（IMF-fixdate）零堆分配解析、格式化与 Howard Hinnant 历法 Unix 时间戳互转库（吸收自 bun/src/http/Headers.cp）（5/5 PASS，文档直写 `doc/lib/http_date.md`）
实现零堆分配 RFC 9110 IMF-fixdate 解析/格式化与 Unix 纪元秒双向互转（含闰年校验），通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:diff_unified` 基于栈上 LCS 动态规划的行级 Unified Diff 计算与相邻上下文 Hunk 自动合并格式化库（吸收自 bun/src/test_runner/diff/printDiff.cp 与 diff_format.cp）（5/5 PASS，文档直写 `doc/lib/diff_unified.md`）
子智能体 1 吸收 Bun test_runner diff，实现零堆分配 LCS 行级 diff 与标准 Unified Diff Hunk 格式化，5/5 单测通过

## 2026-11-22: 新建 `cap:content_range` RFC 9110 §14 HTTP Range 请求头与 Content-Range 响应头零堆分配解析与格式化库（206/416 分片传输，吸收自 bun/src/http/SendFile.cp）（5/5 PASS，文档直写 `doc/lib/content_range.md`）
吸收 HTTP SendFile 分片传输需求，实现完整 RFC 9110 Range（闭区间/开放尾/后缀/多区间/416）与 Content-Range 双向编解码，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:s3_meta` AWS S3 / R2 / MinIO 8 种 ACL、11 种存储类、Bucket 命名校验与 3 种 S3 URI 格式解析库（吸收自 bun/src/s3_signing/{acl,storage_class,credentials}.cp）（5/5 PASS，文档直写 `doc/lib/s3_meta.md`）
子智能体 2 吸收 Bun s3_signing 元数据，实现 ACL/StorageClass/Bucket 校验与 s3:// / Virtual-Hosted / Path-Style URI 解析，5/5 单测通过

## 2026-11-22: 新建 `cap:human_fmt` 零堆分配人类可读格式化工具库（字节大小/纳秒耗时/URL 密码与 UUID/npm 令牌脱敏/Unicode&ASCII 终端表格渲染，吸收自 bun/src/bun_core/fmt.cp）（5/5 PASS，文档直写 `doc/lib/human_fmt.md`）
吸收 bun/src/bun_core/fmt.cp，实现字节/时间/URL 脱敏与 CLI 表格边框渲染，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:pg_sqlstate` PostgreSQL 5 字符 SQLSTATE 错误码 15 类划分、43 项官方条件名与可重试/约束/连接错误判定库（吸收自 bun/src/sql/postgres/AnyPostgresError.cp）（5/5 PASS，文档直写 `doc/lib/pg_sqlstate.md`）
子智能体 2 吸收 AnyPostgresError，实现 15 大类与 43 项官方 SQLSTATE 条件名及可重试判定，5/5 单测通过

## 2026-11-22: 新建 `cap:pg_oids` PostgreSQL 45 项高频标量与数组 OID 类型目录、双向数组映射与二进制编码偏好查询库（吸收自 bun/src/sql/postgres/types/Tag.cp 与 PostgresTypes.cp）（5/5 PASS，文档直写 `doc/lib/pg_oids.md`）
子智能体 1 吸收 Bun Postgres OID 表，实现 30 标量 + 15 数组 OID 双向映射与分类查询，5/5 单测通过

## 2026-11-22: 新建 `cap:sourcemap_index` SourceMap v3 Base64-VLQ mappings 零堆分配解码器与 O(log N) 二分查找原始源码定位器（吸收自 bun/src/sourcemap/Mapping.cp）（5/5 PASS，文档直写 `doc/lib/sourcemap_index.md`）
吸收 bun/src/sourcemap/Mapping.cp，实现 v3 VLQ 解码与二分查找定位，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:no_proxy` HTTP NO_PROXY 零堆分配代理绕过匹配器与 IPv4 CIDR 网段引擎（吸收自 bun/src/http/ProxyTunnel.cp）（5/5 PASS，文档直写 `doc/lib/no_proxy.md`）
子智能体 2 吸收 ProxyTunnel NO_PROXY 规则，实现 IPv4/CIDR、域名后缀防误匹配、端口约束与 IPv6 方括号支持，5/5 单测通过

## 2026-11-22: 新建 `cap:mysql_caps` MySQL 4.1-8.4 协议 32-bit 客户端/服务端能力位协商与 16-bit StatusFlags 事务/多结果集状态解析库（吸收自 bun/src/sql/mysql/{Capabilities,StatusFlags}.cp）（5/5 PASS，文档直写 `doc/lib/mysql_caps.md`）
子智能体 1 吸收 bun/src/sql/mysql/{Capabilities,StatusFlags}.cp，实现 32 位能力位协商与 14 项服务器状态解析，5/5 单测通过

## 2026-11-22: 新建 `cap:sql_conn_uri` PostgreSQL / MySQL / SQLite 连接串零堆分配解析器、SSLMode 策略与 PostgreSQL CommandTag 解析库（吸收自 bun/src/sql/postgres/{CommandTag,SSLMode}.cp 与 mysql/SSLMode.cp）（5/5 PASS，文档直写 `doc/lib/sql_conn_uri.md`）
吸收 Bun SQL 连接与 CommandTag 实现，实现零堆分配三方言连接串解析（含 IPv6/默认端口/sslmode）与 CommandTag 解析，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:header_value_iter` RFC 9110 HTTP 逗号/分号多值头部零堆分配迭代器（保护引号逗号）与 Cache-Control 指令解析器（吸收自 bun/src/http/HeaderValueIterator.cp）（5/5 PASS，文档直写 `doc/lib/header_value_iter.md`）
子智能体 1 吸收 bun/src/http/HeaderValueIterator.cp，实现逗号/分号多值迭代器与 Cache-Control 解析，5/5 单测通过

## 2026-11-22: 新建 `cap:hive_array` 基于 64-bit 分块位图的零堆分配固定容量 Slab 槽池（O(1) 分配/防重复释放/稀疏 @ctz 跳跃迭代，吸收自 bun/src/collections/hive_array.cp）（5/5 PASS，文档直写 `doc/lib/hive_array.md`）
子智能体 2 吸收 bun/src/collections/hive_array.cp，实现支持任意容量的 64-bit 位图 Slab 槽池，5/5 单测通过

## 2026-11-22: 新建 `cap:http_cookie` RFC 6265 / 6265bis HTTP Cookie & Set-Cookie 零堆分配解析、域/路径匹配与序列化库（吸收自 bun/src/jsc/bindings/Cookie.cpp 与 CookieMap.cpp）（5/5 PASS，文档直写 `doc/lib/http_cookie.md`）
吸收 Bun Cookie/CookieMap，实现完整 RFC 6265 Set-Cookie 解析/序列化、域/路径匹配与 Cookie 请求头零分配迭代，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:yaml_scalar` YAML 1.2 Core Schema 零堆分配标量类型推断、单/双引号解转义与安全引用序列化库（吸收自 bun/src/interchange/yaml.cp）（5/5 PASS，文档直写 `doc/lib/yaml_scalar.md`）
子智能体 1 吸收 bun/src/interchange/yaml.cp，实现 YAML 1.2 Core Schema 标量解析与安全序列化，5/5 单测通过

## 2026-11-22: 新建 `cap:toml_lexer` TOML v1.0 零堆分配流式词法扫描器（21 种记号/4 种字符串/4 进制整数/ISO 8601 日期时间与 Unicode 解转义，吸收自 bun/src/interchange/toml/lexer.cp）（5/5 PASS，文档直写 `doc/lib/toml_lexer.md`）
子智能体 2 吸收 bun/src/interchange/toml/lexer.cp，实现完整 TOML v1.0 词法扫描与转义解码器，5/5 单测通过

## 2026-11-22: 新建 `cap:linear_fifo` 零堆分配泛型环形缓冲区 FIFO（支持 item/slice 读写、peek、unget、discard 与环形回绕，吸收自 bun/src/collections/linear_fifo.cp）（5/5 PASS，文档直写 `doc/lib/linear_fifo.md`）
吸收 bun/src/collections/linear_fifo.cp，实现泛型 StaticLinearFifo(T, capacity) 环形缓冲区，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:alt_svc` RFC 7838 HTTP Alt-Svc 头部解析器与零堆分配 64 源 TTL 缓存（HTTP/3 升级发现，吸收自 bun/src/http/h3_client/AltSvc.cp）（5/5 PASS，文档直写 `doc/lib/alt_svc.md`）
吸收 bun/src/http/h3_client/AltSvc.cp，实现 RFC 7838 Alt-Svc 解析、clear 驱逐与 64 源 TTL 缓存，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:fs_router` Next.js / Bun FileSystemRouter 风格文件路由解析、静态>动态>捕获优先级排序与零拷贝参数匹配引擎（吸收自 bun/src/router/{Route,route_types}.cp）（5/5 PASS，文档直写 `doc/lib/fs_router.md`）
子智能体 1 吸收 bun/src/router/{Route,route_types}.cp，实现支持静态/动态/catch-all/optional-catch-all 的零堆分配文件路由器，5/5 单测通过

## 2026-11-22: 新建 `cap:http_method` HTTP / WebDAV / QUERY 全 36 种方法枚举、RFC 9110 安全/幂等/可缓存/CORS 谓词与 64-bit MethodSet（吸收自 bun/src/http_types/Method.cp）（5/5 PASS，文档直写 `doc/lib/http_method.md`）
吸收 bun/src/http_types/Method.cp，实现 36 种 HTTP 方法双射解析、RFC 9110 谓词与 Allow 头格式化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:http_fetch_meta` WHATWG Fetch 请求元数据枚举、RFC 9110 Accept-Encoding q 权重内容协商与零拷贝 URLPath 拆分库（吸收自 bun/src/http_types/{Encoding,Fetch*,URLPath}.cp）（5/5 PASS，文档直写 `doc/lib/http_fetch_meta.md`）
子智能体 2 吸收 bun/src/http_types/*，实现 Accept-Encoding 权重协商、Fetch 枚举与 UrlPath 零拷贝拆分，5/5 单测通过

## 2026-11-22: 新建 `cap:compile_target` 跨平台二进制分发与交叉编译目标三元组解析、支持矩阵校验与 Zig 三元组映射库（吸收自 bun/src/options_types/CompileTarget.cp）（5/5 PASS，文档直写 `doc/lib/compile_target.md`）
子智能体 1 吸收 bun/src/options_types/CompileTarget.cp，实现全平台别名解析、支持矩阵校验与 Zig 三元组输出，5/5 单测通过

## 2026-11-22: 新建 `cap:bit_ops` 零开销 u64 位掩码、位域提取/插入、2 的幂对齐、popcount/clz/ctz 与置位迭代器库（吸收自 bun/src/meta/bits.cp）（5/5 PASS，文档直写 `doc/lib/bit_ops.md`）
吸收 bun/src/meta/bits.cp，实现全套底层位运算原语与 BitSetIter64 升序迭代器，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:path_resolve` POSIX 路径零堆分配规范化、拼接、相对路径、最长公共目录前缀与防目录穿越校验库（吸收自 bun/src/paths/resolve_path.cp）（5/5 PASS，文档直写 `doc/lib/path_resolve.md`）
吸收 bun/src/paths/resolve_path.cp，实现完整 POSIX 路径规范化/相对路径/最长公共前缀与安全边界校验，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:pkg_exports` Node.js / Bun package.json exports 条件子路径解析引擎（精确优先/PATTERN_KEY_COMPARE/null 阻断/.. 防穿越，吸收自 bun/src/resolver/package_json.cp）（5/5 PASS，文档直写 `doc/lib/pkg_exports.md`）
子智能体 2 吸收 bun/src/resolver/package_json.cp，实现标准 PACKAGE_EXPORTS_RESOLVE 条件子路径解析引擎，5/5 单测通过

## 2026-11-22: 新建 `cap:json5_lexer` JSON5 零堆分配词法扫描器（注释/单引号/无引号键/十六进制/Infinity/NaN）与 RFC 8259 标准 JSON 转译器（吸收自 bun/src/interchange/json5.cp）（5/5 PASS，文档直写 `doc/lib/json5_lexer.md`）
子智能体 1 吸收 bun/src/interchange/json5.cp，实现完整 JSON5 词法扫描与零堆分配标准 JSON 规范化转译，5/5 单测通过

## 2026-11-22: 新建 `cap:etag` RFC 9110 / RFC 7232 HTTP ETag 解析、强/弱比较、If-None-Match/If-Match 求值与 FNV-1a 内容 ETag 生成库（吸收自 bun/src/http_types/ETag.cp）（5/5 PASS，文档直写 `doc/lib/etag.md`）
吸收 bun/src/http_types/ETag.cp，实现 RFC 9110 强/弱 ETag 比较、引号内逗号保护与内容 ETag 生成，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:gitconfig_ini` Git Config / INI 零堆分配解析器（支持 subsection、注释、引号、布尔解析与 url.<base>.insteadOf 重写，吸收自 bun/src/install/repository.cp）（5/5 PASS，文档直写 `doc/lib/gitconfig_ini.md`）
子智能体 2 吸收 bun/src/install/repository.cp，实现完整 GitConfig/INI 解析与 insteadOf URL 重写，5/5 单测通过

## 2026-11-22: 新建 `cap:css_image_set` CSS Images 4 <image> 与 image-set() 分辨率换算、MIME 过滤 DPR 选图与序列化库（吸收自 bun/src/css/values/image.cp）（5/5 PASS，文档直写 `doc/lib/css_image_set.md`）
子智能体 1 吸收 bun/src/css/values/image.cp，实现 Resolution 单位换算、image-set() 选图与规范化序列化，5/5 单测通过

## 2026-11-22: 新建 `cap:tsconfig_paths` TSConfig compilerOptions.paths 最长前缀通配符路径映射解析器与 ${configDir} 模板展开器（吸收自 bun/src/resolver/tsconfig_json.cp）（5/5 PASS，文档直写 `doc/lib/tsconfig_paths.md`）
吸收 bun/src/resolver/tsconfig_json.cp，实现精确匹配优先+最长通配符前缀解析与 ${configDir} 替换，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:data_url` RFC 2397 Data URL 零堆分配解析、percent/base64 双解码与最短编码择优器（吸收自 bun/src/resolver/data_url.cp）（5/5 PASS，文档直写 `doc/lib/data_url.md`）
吸收 bun/src/resolver/data_url.cp，实现 RFC 2397 DataUrl 解析、Base64/Percent 解码与最短编码择优，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:html_asset_scanner` 零堆分配 HTML 资源引用扫描器（支持 script/link/img/video/audio/source、精确字节偏移原位重写与 srcset 解析，吸收自 bun/src/bundler/HTMLScanner.cp）（5/5 PASS，文档直写 `doc/lib/html_asset_scanner.md`）
子智能体 1 吸收 bun/src/bundler/HTMLScanner.cp，实现零外部依赖 HTML 资源扫描与 srcset CDN 逗号兼容解析，6/6 单测通过

## 2026-11-22: 新建 `cap:css_text` CSS Text 3/4 与 Text Decoration 属性族（text-transform/white-space/word-break/text-align/text-decoration 简写，吸收自 bun/src/css/properties/text.cp）（5/5 PASS，文档直写 `doc/lib/css_text.md`）
子智能体 2 吸收 bun/src/css/properties/text.cp，实现全套排版枚举与 text-decoration 简写，5/5 单测通过

## 2026-11-22: 新建 `cap:css_list_style` CSS Lists & Counters Level 3 属性库（18 种预定义计数器样式、list-style 简写双 none 消歧与 marker-side，吸收自 bun/src/css/properties/list.cp）（5/5 PASS，文档直写 `doc/lib/css_list_style.md`）
吸收 bun/src/css/properties/list.cp，实现完整 list-style 简写、双 none 消歧与 18 种计数器样式，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:hosted_git_info` GitHub/GitLab/Bitbucket/Gist 托管 Git URL 规范化解析与 https/ssh/sshurl/shortcut/tarball/browse/bugs 多格式生成器（吸收自 bun/src/install/hosted_git_info.cp）（5/5 PASS，文档直写 `doc/lib/hosted_git_info.md`）
子智能体 2 吸收 bun/src/install/hosted_git_info.cp，实现 4 大 Git 平台多协议 URL 解析与 7 种规范输出，5/5 单测通过

## 2026-11-22: 新建 `cap:css_cascade_rules` CSS Cascade 5/6 与作用域规则头（@import layer/supports/media、@scope start/to、@layer 列表）零堆分配解析与序列化（吸收自 bun/src/css/rules/{import,scope,viewport}.cp）（5/5 PASS，文档直写 `doc/lib/css_cascade_rules.md`）
吸收 bun/src/css/rules/{import,scope,viewport}.cp，实现 @import/@scope/@layer 规则头解析与序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_position` CSS <position> 1/2/3/4 值二维坐标解析与最短规范化序列化库（吸收自 bun/src/css/values/position.cp）（5/5 PASS，文档直写 `doc/lib/css_position.md`）
吸收 bun/src/css/values/position.cp，实现完整 1-4 值 CSS 位置语法与反向关键字偏移规范化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_svg` SVG CSS 渲染属性族（SVGPaint/StrokeLinecap/StrokeLinejoin/StrokeDasharray/PaintOrder 与 5 大渲染提示枚举，吸收自 bun/src/css/properties/svg.cp）（5/5 PASS，文档直写 `doc/lib/css_svg.md`）
子智能体 1 吸收 bun/src/css/properties/svg.cp，实现完整 SVG 描边/绘制顺序/渲染提示属性解析与规范化序列化，6/6 单测通过

## 2026-11-22: 新建 `cap:css_transition` CSS Transitions 1/2 过渡属性族与多过渡简写列表解析/序列化库（吸收自 bun/src/css/properties/transition.cp）（5/5 PASS，文档直写 `doc/lib/css_transition.md`）
子智能体 2 吸收 bun/src/css/properties/transition.cp，实现单/多过渡简写、cubic-bezier/steps 与默认值省略序列化，5/5 单测通过

## 2026-11-22: 新建 `cap:pkg_matcher` PNPM 兼容三态包名匹配器（全包含/全排除/混合排除优先）与线性时间 */? 通配符引擎（吸收自 bun/src/install/PnpmMatcher.cp）（5/5 PASS，文档直写 `doc/lib/pkg_matcher.md`）
吸收 bun/src/install/PnpmMatcher.cp，实现零堆分配三态包名过滤与线性通配符匹配，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:clap_stream` 零堆分配流式命令行参数解析器（支持 --long/--long=val/-abc 串联/-oval/-- 位置参数分隔与精确错误码，吸收自 bun/src/clap/streaming.cp）（5/5 PASS，文档直写 `doc/lib/clap_stream.md`）
吸收 bun/src/clap/streaming.cp，实现零堆分配流式 CLI 参数解析器，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_border_image` CSS Backgrounds 3 border-image 属性族与双斜杠简写（slice/width/outset/repeat/fill）解析与序列化库（吸收自 bun/src/css/properties/border_image.cp）（5/5 PASS，文档直写 `doc/lib/css_border_image.md`）
子智能体 1 吸收 bun/src/css/properties/border_image.cp，实现完整 border-image 子属性与双斜杠简写，6/6 单测通过

## 2026-11-22: 新建 `cap:shell_util` 零堆分配 POSIX Shell 安全引用/双引号转义、命令行拼接、basename/dirname 路径运算与环境变量解析库（吸收自 bun/src/shell/util.cp 与 builtin/{basename,dirname}.cp）（5/5 PASS，文档直写 `doc/lib/shell_util.md`）
子智能体 2 吸收 bun/src/shell/util.cp 与 basename/dirname，实现全套 POSIX shell 转义与路径/环境变量工具，5/5 单测通过

## 2026-11-22: 新建 `cap:css_layout_ctrl` CSS 布局控制属性库（overflow/text-overflow/position/container-type/contain/z-index，吸收自 bun/src/css/properties/{overflow,position,contain}.cp）（5/5 PASS，文档直写 `doc/lib/css_layout_ctrl.md`）
吸收 bun/src/css/properties/{overflow,position,contain}.cp，实现完整布局控制属性与 contain strict/content 合并规范化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_effects` CSS Filter Effects 11 种标准滤镜函数链（blur/brightness/contrast/grayscale/hue-rotate/invert/opacity/saturate/sepia/drop-shadow/url）零堆分配解析与序列化（吸收自 bun/src/css/properties/effects.cp）（5/5 PASS，文档直写 `doc/lib/css_effects.md`）
吸收 bun/src/css/properties/effects.cp，实现 11 种 CSS 滤镜解析、范围裁剪与规范化序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:md_links` CommonMark §6.3-6.5 内联链接、图片、引用链接与 URI/Email 自动链接零堆分配解析器及 URL 百分号编码（吸收自 bun/src/md/links.cp）（5/5 PASS，文档直写 `doc/lib/md_links.md`）
子智能体 1 吸收 bun/src/md/links.cp，实现 CommonMark 全类型链接/图片/自动链接解析与 percentEncodeUrl，6/6 单测通过

## 2026-11-22: 新建 `cap:semver_query` 包管理器多协议依赖版本描述符解析器（npm/workspace/catalog/github/git/file/link/tarball/npm_alias）与 npm 包名校验器（吸收自 bun/src/semver/SemverQuery.cp）（5/5 PASS，文档直写 `doc/lib/semver_query.md`）
子智能体 2 吸收 bun/src/semver/SemverQuery.cp，实现 11 种依赖协议解析与标准 npm 包名校验，5/5 单测通过

## 2026-11-22: 新建 `cap:css_selector_builder` CSS 选择器构建器（Servo 右向左复合匹配序布局）与 Selectors Level 4 特异度计算库（吸收自 bun/src/css/selectors/builder.cp）（5/5 PASS，文档直写 `doc/lib/css_selector_builder.md`）
吸收 bun/src/css/selectors/builder.cp，实现零堆分配 SelectorBuilder、addNestingPrefix、特异度计算与源序/匹配序双输出，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:ci_info` 零堆分配 CI/CD 31 供应商环境与 PR 构建检测器（遵循 ci-info v4，吸收自 bun/src/cli/ci_info.cp 与 codegen/ci_info.ts）（5/5 PASS，文档直写 `doc/lib/ci_info.md`）
吸收 bun/src/cli/ci_info.cp，实现 31 家 CI 平台与 PR 状态确定性检测，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_gradient` CSS Images 4 线性/径向/锥形渐变（含 repeating-* 与双位置色标）零堆分配解析与规范化序列化库（吸收自 bun/src/css/values/gradient.cp）（5/5 PASS，文档直写 `doc/lib/css_gradient.md`）
子智能体 1 吸收 bun/src/css/values/gradient.cp，实现 6 种 CSS 渐变解析与默认值省略规范化序列化，6/6 单测通过

## 2026-11-22: 新建 `cap:errno_map` 跨平台 POSIX/OS Errno 69 项核心错误码双射映射、OS 特定错误消息与瞬态重试判定（吸收自 bun/src/sys/coreutils_error_map.cp 与 errno/{linux,darwin}_errno.cp）（5/5 PASS，文档直写 `doc/lib/errno_map.md`）
子智能体 2 吸收 coreutils_error_map 与 linux/darwin errno，实现 69 变体跨平台双射映射与零 std.posix 依赖查表，5/5 单测通过

## 2026-11-22: 新建 `cap:progress_tree` 零堆分配层级进度树、节流刷新判定与终端状态行格式化引擎（吸收自 bun/src/bun_core/Progress.cp）（5/5 PASS，文档直写 `doc/lib/progress_tree.md`）
吸收 bun/src/bun_core/Progress.cp，实现 32 节点层级进度树、initial_delay/refresh_rate 节流与字节/文件进度行渲染，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:md_line_analysis` CommonMark / GFM 块级行分析器（Setext/ATX 标题、HR、围栏代码块、HTML 块 Type 1-7、62 块级标签与 GFM 表格分隔行，吸收自 bun/src/md/line_analysis.cp）（5/5 PASS，文档直写 `doc/lib/md_line_analysis.md`）
吸收 bun/src/md/line_analysis.cp，实现零堆分配 CommonMark/GFM 块级行分析器，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:grapheme_cluster` Unicode UAX #29 扩展字素簇边界判定（GB3-GB13/Hangul/ZWJ Emoji/国旗）与零分配 UTF-8 迭代器（吸收自 bun/src/string/immutable/grapheme.cp）（5/5 PASS，文档直写 `doc/lib/grapheme_cluster.md`）
子智能体 1 吸收 bun/src/string/immutable/grapheme.cp，实现完整 UAX #29 15 类码点分类、状态机与容错 UTF-8 字素迭代器，6/6 单测通过

## 2026-11-22: 新建 `cap:css_font` CSS Fonts 4 属性族与完整 Font 简写解析/序列化库（吸收自 bun/src/css/properties/font.cp）（5/5 PASS，文档直写 `doc/lib/css_font.md`）
子智能体 2 吸收 bun/src/css/properties/font.cp，实现完整 FontWeight/Style/Stretch/Size/LineHeight/VariantCaps/GenericFontFamily 与 Font 简写，5/5 单测通过

## 2026-11-22: 新建 `cap:css_supports` CSS @supports 条件表达式零分配 Arena 解析、求值与规范化序列化库（吸收自 bun/src/css/rules/supports.cp）（5/5 PASS，文档直写 `doc/lib/css_supports.md`）
吸收 bun/src/css/rules/supports.cp，实现 not/and/or/declaration/selector/unknown 递归下降解析、自定义谓词求值与括号规范化序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:md_ref_defs` CommonMark §4.7 链接引用定义解析、零分配标签规范化与首定义优先查找表（吸收自 bun/src/md/ref_defs.cp）（5/5 PASS，文档直写 `doc/lib/md_ref_defs.md`）
吸收 bun/src/md/ref_defs.cp，实现 CommonMark 链接引用定义解析、目标/标题解析器与首定义优先查找表，通过 cpc check 与 cap test

# CP 研发日志 (DEVLOG) - 2026-11-22 (热数据)

## 2026-11-22: 新建 `cap:css_transform` CSS 2D/3D Transform 21 种函数、4x4 齐次矩阵合成、Rodrigues 旋转与原点/样式属性库（吸收自 bun/src/css/properties/transform.cp）（5/5 PASS，文档直写 `doc/lib/css_transform.md`）
子智能体 1 吸收 bun/src/css/properties/transform.cp，实现 21 种 2D/3D Transform、4x4 矩阵乘法/伴随行列式与 RAII TransformList，6/6 单测通过

## 2026-11-22: 新建 `cap:css_background` CSS Background 属性族与多图层简写解析/序列化（吸收自 bun/src/css/properties/background.cp）（5/5 PASS，文档直写 `doc/lib/css_background.md`）
子智能体 2 吸收 bun/src/css/properties/background.cp，实现完整背景子属性、双 box 规则与多图层简写，5/5 单测通过

## 2026-11-22: 新建 `cap:css_sizing` CSS Sizing（width/height/max-size/aspect-ratio/fit-content）与盒模型 Insets（margin/padding/inset）属性库（吸收自 bun/src/css/properties/{size,margin_padding}.cp）（5/5 PASS，文档直写 `doc/lib/css_sizing.md`）
吸收 bun/src/css/properties/{size,margin_padding}.cp，实现完整固有尺寸、厂商前缀、aspect-ratio 与 1-4 值盒模型边距简写，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_border` CSS Border 简写、四边宽/样式/颜色与逻辑边属性解析和压缩序列化（吸收自 bun/src/css/properties/border.cp）（5/5 PASS，文档直写 `doc/lib/css_border.md`）
吸收 bun/src/css/properties/border.cp，实现任意顺序 border 简写与 1-4 值四边压缩序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:semver_range` SemVer 版本比较器与范围匹配引擎（吸收自 bun/src/semver/{SemverRange,Version}.cp）（5/5 PASS，文档直写 `doc/lib/semver_range.md`）
子智能体 1 吸收 bun/src/semver/{SemverRange,Version}.cp，实现 ^/~/>=/<=/-/x-range/|| 与 prerelease 匹配引擎，6/6 单测通过

## 2026-11-22: 新建 `cap:css_flex_grid` CSS Flexbox / Box Alignment / Grid 布局属性解析与序列化（吸收自 bun/src/css/properties/{flex,align,grid}.cp）（5/5 PASS，文档直写 `doc/lib/css_flex_grid.md`）
子智能体 2 吸收 bun/src/css/properties/{flex,align,grid}.cp，实现完整 Flexbox/Alignment/Grid 布局属性解析与序列化，5/5 单测通过

## 2026-11-22: 新建 `cap:css_border_radius` CSS border-radius 椭圆/四值简写解析与规范化压缩序列化（吸收自 bun/src/css/properties/border_radius.cp）（5/5 PASS，文档直写 `doc/lib/css_border_radius.md`）
吸收 bun/src/css/properties/border_radius.cp，实现纯 CP 椭圆斜杠与 1-4 值压缩 border-radius 解析器与序列化器，通过 cpc check 与 cap test

## 2026-11-22: CSS box-shadow 与 border/outline 线型属性（5/5 PASS，文档直写 `doc/lib/css_box_shadow.md`）
从 Bun css/properties/box_shadow.cp 与 outline.cp 吸收 box-shadow 简写与线型枚举，自动化简零 blur/spread 与 currentcolor 并拦截负模糊半径。

## 2026-11-22: PostgreSQL v3 COPY 协议与 SASL/Password 前端报文编解码（5/5 PASS，文档直写 `doc/lib/pg_copy_sasl.md`）
从 Bun sql/postgres/protocol 吸收 COPY/SASL/Password/NegotiateProtocolVersion 全套编解码器，采用 ~init() 确定性 RAII 零泄漏释放，补齐 PG v3 协议全栈。

## 2026-11-22: CSS Animations & Transitions 简写解析与序列化（5/5 PASS，文档直写 `doc/lib/css_animation.md`）
从 Bun css/properties/animation.cp 与 transition.cp 吸收 CSS 动画与过渡简写解析器，跨模块复用 cap:css_time 与 cap:css_easing。

## 2026-11-22: MySQL 认证密码学原语与 SSLRequest 编解码（5/5 PASS，文档直写 `doc/lib/mysql_auth.md`）
从 Bun sql/mysql/protocol/Auth.cp 与 SSLRequest.cp 吸收 MySQL 认证密码学原语，补齐 MySQL 线协议全栈。

## 2026-11-22: PostgreSQL v3 后端控制与诊断消息解码器（5/5 PASS，文档直写 `doc/lib/pg_messages.md`）
从 Bun sql/postgres/protocol 吸收完整 PG v3 后端控制与 18 字段诊断消息零拷贝解码器，支持 SQLSTATE 分类与 CommandComplete 影响行数解析。

## 2026-11-22: MySQL 协议服务端握手与控制报文解码器（5/5 PASS，文档直写 `doc/lib/mysql_control.md`）
从 Bun sql/mysql/protocol/{HandshakeV10,OKPacket,ErrorPacket,EOFPacket,AuthSwitchRequest,CommandType}.cp 吸收 MySQL 服务端控制报文零拷贝解码器。

## 2026-11-22: PostgreSQL v3 扩展查询协议前端编码器（5/5 PASS，文档直写 `doc/lib/pg_extended_query.md`）
从 Bun sql/postgres/protocol/{Parse,Describe,Execute,Close,PortalOrPreparedStatement}.cp 吸收 PG v3 扩展查询协议前端流水线编码器，支持 ~init() 确定性零泄漏释放。

## 2026-11-22: CSS Basic UI Level 4 属性集（5/5 PASS，文档直写 `doc/lib/css_ui.md`）
从 Bun css/properties/ui.cp 吸收 CSS UI Level 4 六大交互属性模型，纯 CP 零堆分配实现规范化解析与输出。

## 2026-11-22: CSS Media Queries Level 4 媒体查询解析与求值引擎（5/5 PASS，文档直写 `doc/lib/css_media.md`）
从 Bun css/rules/media.cp 吸收 CSS Media Queries Level 4 解析与视口/用户偏好求值引擎，纯 CP 零堆分配内联存储。

## 2026-11-22: CSS Properties & Values API <syntax> 解析引擎（5/5 PASS，文档直写 `doc/lib/css_syntax.md`）
从 Bun css/values/syntax.cp 吸收 CSS <syntax> 解析器，支持 * 零分配、+/# 乘数与 <transform-list> 约束校验及 ~init() 零泄漏。

## 2026-11-22: CSS @font-face Unicode 码点区间与通配符优化器（5/5 PASS，文档直写 `doc/lib/unicode_range.md`）
从 Bun css/rules/font_face.cp 吸收 UnicodeRange 解析与十六进制边界 ? 通配符自动压缩算法，纯 CP 零堆分配。

## 2026-11-22: CSS 基础值类型（Alpha / Percentage / URL 分类）（5/5 PASS，文档直写 `doc/lib/css_primitives.md`）
从 Bun css/values/{alpha,percentage,url}.cp 吸收 CSS 基础值类型与 URL 绝对/相对/Data URI 分类器，纯 CP 零堆分配。

## 2026-11-22: CSS Container Queries @container 规则与求值引擎（5/5 PASS，文档直写 `doc/lib/css_container.md`）
从 Bun css/rules/container.cp 吸收 CSS Container Queries Level 3 解析与求值引擎，支持传统与现代范围语法及混合逻辑歧义拦截。

## 2026-11-22: CSS <angle> 与 <resolution> 单位解析与换算库（5/5 PASS，文档直写 `doc/lib/css_units.md`）
从 Bun css/values/angle.cp 与 resolution.cp 吸收 CSS 角度与分辨率单位库，支持无单位 0 角度、0..360 归一化、x 别名与负分辨率拦截。

## 2026-11-22: MySQL 4.1 握手响应报文编解码器（5/5 PASS，文档直写 `doc/lib/mysql_handshake.md`）
从 Bun sql/mysql/protocol/HandshakeResponse41.cp 吸收 MySQL 4.1 客户端握手响应编解码器，支持能力位自动切换与 ~init() 零泄漏释放。

## 2026-11-22: ECMAScript/Web/Node/Bun 已知全局符号与纯度分析器（5/5 PASS，文档直写 `doc/lib/known_globals.md`）
从 Bun js_parser/ast/KnownGlobal.cp 吸收 69 项 ECMAScript/Web/Node/Bun 全局符号纯度表，零堆分配支持静态属性访问与纯调用 DCE 判定。

## 2026-11-22: 二分查找源码行列偏移表 LineOffsetTable（5/5 PASS，文档直写 `doc/lib/line_offset_table.md`）
从 Bun sourcemap/LineOffsetTable.cp 吸收源码行列偏移表，单遍扫描识别 LF/CRLF/CR 并支持 O(log N) 二分定位与 ~init() 确定性零泄漏释放。

## 2026-11-22: CSS @keyframes/@layer/@counter-style/@property 规则集（5/5 PASS，文档直写 `doc/lib/css_at_rules.md`）
从 Bun css/rules/{keyframes,layer,counter_style,property}.cp 吸收四大 CSS @ 规则元数据与校验模型，零堆分配内联实现。

## 2026-11-22: CSS <time> 时间值解析、运算与规范序列化（5/5 PASS，文档直写 `doc/lib/css_time.md`）
从 Bun css/values/time.cp 吸收 CSS <time> 单位解析与最短规范序列化（0.05s→50ms、500ms→0.5s、0ms→0s），纯 CP 零堆分配。

## 2026-11-22: ThumbHash 超紧凑图片占位符编解码器（5/5 PASS，文档直写 `doc/lib/thumbhash.md`）
从 Bun runtime/image/thumbhash.cp 吸收完整 ThumbHash DCT/IDCT 编解码引擎，补充 approxAspectRatio/averageRGBA 零分配查询与 ~init() 确定性释放。

## 2026-11-22: 零解码极速图片头格式与尺寸探测器（5/5 PASS，文档直写 `doc/lib/image_probe.md`）
从 Bun runtime/image/{codec_png,codec_jpeg,codec_webp,codec_bmp}.cp 吸收零解码五格式图片头探测器，纯 CP 零堆分配且内置严密畸形头边界防护。
