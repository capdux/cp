# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:robots_txt` RFC 9309 robots.txt 解析、通配符匹配、Crawl-delay 提取与 Sitemap 迭代器（5/5 PASS，自动动态挂载 `cap.robots_txt`）
新增 `cap:robots_txt`（`lib/robots_txt/{robots,index}.cp`、`doc/lib/robots_txt.md`、`cap/test/test_robots_txt_production_deep.cp`），实现 RFC 9309 最具体组匹配、最长规则优先与同长度 Allow 优先。

## [2026-11-22] - 新建 `cap:netrc` .netrc / _netrc 认证凭据文件零分配流式解析器（支持引号、注释、macdef 与 default 回退）（5/5 PASS，自动动态挂载 `cap.netrc`）
新增 `cap:netrc`（`lib/netrc/{netrc,index}.cp`、`doc/lib/netrc.md`、`cap/test/test_netrc_production_deep.cp`），完整支持单行/多行、引号含空格、跳过 `macdef` 与大小写不敏感 `machine`/`default` 查找。

## [2026-11-22] - 新建 `cap:hosts_file` /etc/hosts 零分配流式解析器与 IPv4/IPv6 正反向解析查找（5/5 PASS，自动动态挂载 `cap.hosts_file`）
新增 `cap:hosts_file`（`lib/hosts_file/{hosts,index}.cp`、`doc/lib/hosts_file.md`、`cap/test/test_hosts_file_production_deep.cp`），支持 UTF-8 BOM、多别名、`#` 注释与正反向查询。

## [2026-11-22] - 新建 `cap:ip_addr` RFC 4291 / RFC 5952 IPv4 与 IPv6 零分配解析器、规范化零压缩格式化器、地址分类与 CIDR 子网匹配（5/5 PASS，自动动态挂载 `cap.ip_addr`）
新增 `cap:ip_addr`（`lib/ip_addr/{ip,index}.cp`、`doc/lib/ip_addr.md`、`cap/test/test_ip_addr_production_deep.cp`），实现 IPv4/IPv6 解析、RFC 5952 最长零段压缩、IPv4 映射 IPv6 互转与子网匹配。

## [2026-11-22] - 新建 `cap:http_cache_control` RFC 9111 Cache-Control 指令零分配流式解析器、缓存存储/有效 TTL 判定与规范化格式化器（5/5 PASS，自动动态挂载 `cap.http_cache_control`）
新增 `cap:http_cache_control`（`lib/http_cache_control/{cache_control,index}.cp`、`doc/lib/http_cache_control.md`、`cap/test/test_http_cache_control_production_deep.cp`），完整覆盖 15 项核心 RFC 9111 指令。

## [2026-11-22] - 新建 `cap:json_pointer` RFC 6901 JSON Pointer 解析、~0/~1 转义反转义、数组索引校验与路径段迭代器（5/5 PASS，自动动态挂载 `cap.json_pointer`）
新增 `cap:json_pointer`（`lib/json_pointer/{pointer,index}.cp`、`doc/lib/json_pointer.md`、`cap/test/test_json_pointer_production_deep.cp`），实现 RFC 6901 全套零分配解析与转义操作。

## [2026-11-22] - 新建 `cap:http_accept` RFC 9110 Accept / Accept-Encoding / Accept-Language 内容协商与 q 值解析器（5/5 PASS，自动动态挂载 `cap.http_accept`）
新增 `cap:http_accept`（`lib/http_accept/{accept,index}.cp`、`doc/lib/http_accept.md`、`cap/test/test_http_accept_production_deep.cp`），实现严格 q 值解析与三类内容协商最佳匹配。

## [2026-11-22] - 新建 `cap:token_bucket` 纯整数毫令牌确定性令牌桶与固定时间窗口限流器（防时钟回拨与溢出）（5/5 PASS，自动动态挂载 `cap.token_bucket`）
新增 `cap:token_bucket`（`lib/token_bucket/{bucket,index}.cp`、`doc/lib/token_bucket.md`、`cap/test/test_token_bucket_production_deep.cp`），实现亚秒级零精度损失令牌桶与固定窗口限流器。

## [2026-11-22] - 新建 `cap:sse_parser` WHATWG Server-Sent Events (text/event-stream) 零分配流式解析器与格式化器（5/5 PASS，自动动态挂载 `cap.sse_parser`）
新增 `cap:sse_parser`（`lib/sse_parser/{sse,index}.cp`、`doc/lib/sse_parser.md`、`cap/test/test_sse_parser_production_deep.cp`），实现 WHATWG SSE 规范完整解析与格式化往返。

## [2026-11-22] - 新建 `cap:content_disposition` RFC 6266 / RFC 5987 Content-Disposition 响应头解析、文件名安全消毒与 UTF-8 filename* 格式化器（5/5 PASS，自动动态挂载 `cap.content_disposition`）
新增 `cap:content_disposition`（`lib/content_disposition/{disposition,index}.cp`、`doc/lib/content_disposition.md`、`cap/test/test_content_disposition_production_deep.cp`），完整实现 RFC 6266 解析、RFC 5987 UTF-8 解码与路径穿越消毒。

## [2026-11-22] - 新建 `cap:multipart_boundary` RFC 7578 / RFC 2046 multipart/form-data 零堆分配流式边界切分器与分片头部查找库（吸收自 bun/src/http/HTTPRequestBody.cp）（5/5 PASS，自动动态挂载 `cap.multipart_boundary`）
新增 cap:multipart_boundary 标准库模块（lib/multipart_boundary/{multipart,index}.cp, doc/lib/multipart_boundary.md, cap/test/test_multipart_boundary_production_deep.cp）

## [2026-11-22] - 新建 `cap:retry_backoff` 零堆分配指数退避重试调度器（Full/Equal Jitter + SplitMix64 确定性种子）与 RFC 9110 Retry-After 解析库（吸收自 bun/src/http/HTTPContext.cp）（5/5 PASS，自动动态挂载 `cap.retry_backoff`）
新增 cap:retry_backoff 标准库模块（lib/retry_backoff/{backoff,index}.cp, doc/lib/retry_backoff.md, cap/test/test_retry_backoff_production_deep.cp）

## [2026-11-22] - 新建 `cap:cors_policy` WHATWG Fetch CORS 策略配置、Simple/Preflight 预检评估与凭证回显/Vary: Origin 决策引擎（吸收自 bun/src/http_types/FetchRequestMode.cp）（5/5 PASS，自动动态挂载 `cap.cors_policy`）
新增 cap:cors_policy 标准库模块（lib/cors_policy/{cors,index}.cp, doc/lib/cors_policy.md, cap/test/test_cors_policy_production_deep.cp）

## [2026-11-22] - 新建 `cap:csv_stream` RFC 4180 CSV / TSV 零堆分配流式字段/行解析器（自定义分隔符/引号内换行/"" 转义还原）与编码器（吸收自 bun/src/bun_core/fmt.cp）（5/5 PASS，自动动态挂载 `cap.csv_stream`）
新增 cap:csv_stream 标准库模块（lib/csv_stream/{csv,index}.cp, doc/lib/csv_stream.md, cap/test/test_csv_stream_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_forwarded` RFC 7239 Forwarded 与 X-Forwarded-For 反向代理链路零堆分配解析器及客户端真实 IP 提取库（吸收自 bun/src/http/HeaderValueIterator.cp）（5/5 PASS，自动动态挂载 `cap.http_forwarded`）
新增 cap:http_forwarded 标准库模块（lib/http_forwarded/{forwarded,index}.cp, doc/lib/http_forwarded.md, cap/test/test_http_forwarded_production_deep.cp）

## [2026-11-22] - 新建 `cap:js_ident` ECMAScript/TS 56 关键字分类、ASCII/私有字段标识符校验、属性加引号判定与 JS 字符串转义库（吸收自 bun/src/js_parser/lexer/）（5/5 PASS，自动动态挂载 `cap.js_ident`）
新增 cap:js_ident 标准库模块（lib/js_ident/{ident,index}.cp, doc/lib/js_ident.md, cap/test/test_js_ident_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_auth` RFC 9110 §11 / RFC 7617 Basic / RFC 6750 Bearer HTTP Authorization 认证头零堆分配解析与编解码库（吸收自 bun/src/http/Headers.cp）（5/5 PASS，自动动态挂载 `cap.http_auth`）
新增 cap:http_auth 标准库模块（lib/http_auth/{auth,index}.cp, doc/lib/http_auth.md, cap/test/test_http_auth_production_deep.cp）

## [2026-11-22] - 新建 `cap:dotenv_parse` 零堆分配 .env 配置文件解析器（BOM/export/4 种引号/多行/行尾注释）与 ${VAR:-default} 变量展开器（吸收自 bun/src/bun_core/env.cp）（5/5 PASS，自动动态挂载 `cap.dotenv_parse`）
新增 cap:dotenv_parse 标准库模块（lib/dotenv_parse/{dotenv,index}.cp, doc/lib/dotenv_parse.md, cap/test/test_dotenv_parse_production_deep.cp）

## [2026-11-22] - 新建 `cap:mime_lookup` 65+ Web/系统高频扩展名 MIME 查表、Content-Type 头部（charset/boundary）解析与可压缩性判定库（吸收自 bun/src/http_types/{MimeType,mime_type_list_enum}.cp）（5/5 PASS，自动动态挂载 `cap.mime_lookup`）
新增 cap:mime_lookup 标准库模块（lib/mime_lookup/{mime,index}.cp, doc/lib/mime_lookup.md, cap/test/test_mime_lookup_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_link_header` RFC 8288 Web Linking HTTP Link 头部零堆分配解析、rel 查找与格式化库（分页/Preload/Early Hints，吸收自 bun/src/http/HeaderValueIterator.cp）（5/5 PASS，自动动态挂载 `cap.http_link_header`）
新增 cap:http_link_header 标准库模块（lib/http_link_header/{link,index}.cp, doc/lib/http_link_header.md, cap/test/test_http_link_header_production_deep.cp）

## [2026-11-22] - 新建 `cap:lcov_report` LCOV 代码覆盖率报告零堆分配记录、百分比统计、阈值校验与双向解析/序列化库（吸收自 bun/src/options_types/CodeCoverageOptions.cp）（5/5 PASS，自动动态挂载 `cap.lcov_report`）
新增 cap:lcov_report 标准库模块（lib/lcov_report/{lcov,index}.cp, doc/lib/lcov_report.md, cap/test/test_lcov_report_production_deep.cp）

## [2026-11-22] - 新建 `cap:ansi_style` ANSI 终端 16 色/样式/24-bit TrueColor 格式化、CSI & OSC 8 超链接剥离与可见宽度计算库（吸收自 bun/src/bun_core/output.cp）（5/5 PASS，自动动态挂载 `cap.ansi_style`）
新增 cap:ansi_style 标准库模块（lib/ansi_style/{ansi,index}.cp, doc/lib/ansi_style.md, cap/test/test_ansi_style_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_date` RFC 9110 §5.6.7 HTTP-date（IMF-fixdate）零堆分配解析、格式化与 Howard Hinnant 历法 Unix 时间戳互转库（吸收自 bun/src/http/Headers.cp）（5/5 PASS，自动动态挂载 `cap.http_date`）
新增 cap:http_date 标准库模块（lib/http_date/{http_date,index}.cp, doc/lib/http_date.md, cap/test/test_http_date_production_deep.cp）

## [2026-11-22] - 新建 `cap:diff_unified` 基于栈上 LCS 动态规划的行级 Unified Diff 计算与相邻上下文 Hunk 自动合并格式化库（吸收自 bun/src/test_runner/diff/printDiff.cp 与 diff_format.cp）（5/5 PASS，自动动态挂载 `cap.diff_unified`）
新增 cap:diff_unified 标准库模块（lib/diff_unified/{unified,index}.cp, doc/lib/diff_unified.md, cap/test/test_diff_unified_production_deep.cp）

## [2026-11-22] - 新建 `cap:content_range` RFC 9110 §14 HTTP Range 请求头与 Content-Range 响应头零堆分配解析与格式化库（206/416 分片传输，吸收自 bun/src/http/SendFile.cp）（5/5 PASS，自动动态挂载 `cap.content_range`）
新增 cap:content_range 标准库模块（lib/content_range/{range,index}.cp, doc/lib/content_range.md, cap/test/test_content_range_production_deep.cp）

## [2026-11-22] - 新建 `cap:s3_meta` AWS S3 / R2 / MinIO 8 种 ACL、11 种存储类、Bucket 命名校验与 3 种 S3 URI 格式解析库（吸收自 bun/src/s3_signing/{acl,storage_class,credentials}.cp）（5/5 PASS，自动动态挂载 `cap.s3_meta`）
新增 cap:s3_meta 标准库模块（lib/s3_meta/{s3_meta,index}.cp, doc/lib/s3_meta.md, cap/test/test_s3_meta_production_deep.cp）

## [2026-11-22] - 新建 `cap:human_fmt` 零堆分配人类可读格式化工具库（字节大小/纳秒耗时/URL 密码与 UUID/npm 令牌脱敏/Unicode&ASCII 终端表格渲染，吸收自 bun/src/bun_core/fmt.cp）（5/5 PASS，自动动态挂载 `cap.human_fmt`）
新增 cap:human_fmt 标准库模块（lib/human_fmt/{fmt,index}.cp, doc/lib/human_fmt.md, cap/test/test_human_fmt_production_deep.cp）

## [2026-11-22] - 新建 `cap:pg_sqlstate` PostgreSQL 5 字符 SQLSTATE 错误码 15 类划分、43 项官方条件名与可重试/约束/连接错误判定库（吸收自 bun/src/sql/postgres/AnyPostgresError.cp）（5/5 PASS，自动动态挂载 `cap.pg_sqlstate`）
新增 cap:pg_sqlstate 标准库模块（lib/pg_sqlstate/{sqlstate,index}.cp, doc/lib/pg_sqlstate.md, cap/test/test_pg_sqlstate_production_deep.cp）

## [2026-11-22] - 新建 `cap:pg_oids` PostgreSQL 45 项高频标量与数组 OID 类型目录、双向数组映射与二进制编码偏好查询库（吸收自 bun/src/sql/postgres/types/Tag.cp 与 PostgresTypes.cp）（5/5 PASS，自动动态挂载 `cap.pg_oids`）
新增 cap:pg_oids 标准库模块（lib/pg_oids/{oids,index}.cp, doc/lib/pg_oids.md, cap/test/test_pg_oids_production_deep.cp）

## [2026-11-22] - 新建 `cap:sourcemap_index` SourceMap v3 Base64-VLQ mappings 零堆分配解码器与 O(log N) 二分查找原始源码定位器（吸收自 bun/src/sourcemap/Mapping.cp）（5/5 PASS，自动动态挂载 `cap.sourcemap_index`）
新增 cap:sourcemap_index 标准库模块（lib/sourcemap_index/{mapping,index}.cp, doc/lib/sourcemap_index.md, cap/test/test_sourcemap_index_production_deep.cp）

## [2026-11-22] - 新建 `cap:no_proxy` HTTP NO_PROXY 零堆分配代理绕过匹配器与 IPv4 CIDR 网段引擎（吸收自 bun/src/http/ProxyTunnel.cp）（5/5 PASS，自动动态挂载 `cap.no_proxy`）
新增 cap:no_proxy 标准库模块（lib/no_proxy/{no_proxy,index}.cp, doc/lib/no_proxy.md, cap/test/test_no_proxy_production_deep.cp）

## [2026-11-22] - 新建 `cap:mysql_caps` MySQL 4.1-8.4 协议 32-bit 客户端/服务端能力位协商与 16-bit StatusFlags 事务/多结果集状态解析库（吸收自 bun/src/sql/mysql/{Capabilities,StatusFlags}.cp）（5/5 PASS，自动动态挂载 `cap.mysql_caps`）
新增 cap:mysql_caps 标准库模块（lib/mysql_caps/{caps,index}.cp, doc/lib/mysql_caps.md, cap/test/test_mysql_caps_production_deep.cp）

## [2026-11-22] - 新建 `cap:sql_conn_uri` PostgreSQL / MySQL / SQLite 连接串零堆分配解析器、SSLMode 策略与 PostgreSQL CommandTag 解析库（吸收自 bun/src/sql/postgres/{CommandTag,SSLMode}.cp 与 mysql/SSLMode.cp）（5/5 PASS，自动动态挂载 `cap.sql_conn_uri`）
新增 cap:sql_conn_uri 标准库模块（lib/sql_conn_uri/{conn_uri,index}.cp, doc/lib/sql_conn_uri.md, cap/test/test_sql_conn_uri_production_deep.cp）

## [2026-11-22] - 新建 `cap:header_value_iter` RFC 9110 HTTP 逗号/分号多值头部零堆分配迭代器（保护引号逗号）与 Cache-Control 指令解析器（吸收自 bun/src/http/HeaderValueIterator.cp）（5/5 PASS，自动动态挂载 `cap.header_value_iter`）
新增 cap:header_value_iter 标准库模块（lib/header_value_iter/{iter,index}.cp, doc/lib/header_value_iter.md, cap/test/test_header_value_iter_production_deep.cp）

## [2026-11-22] - 新建 `cap:hive_array` 基于 64-bit 分块位图的零堆分配固定容量 Slab 槽池（O(1) 分配/防重复释放/稀疏 @ctz 跳跃迭代，吸收自 bun/src/collections/hive_array.cp）（5/5 PASS，自动动态挂载 `cap.hive_array`）
新增 cap:hive_array 标准库模块（lib/hive_array/{hive,index}.cp, doc/lib/hive_array.md, cap/test/test_hive_array_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_cookie` RFC 6265 / 6265bis HTTP Cookie & Set-Cookie 零堆分配解析、域/路径匹配与序列化库（吸收自 bun/src/jsc/bindings/Cookie.cpp 与 CookieMap.cpp）（5/5 PASS，自动动态挂载 `cap.http_cookie`）
新增 cap:http_cookie 标准库模块（lib/http_cookie/{cookie,index}.cp, doc/lib/http_cookie.md, cap/test/test_http_cookie_production_deep.cp）

## [2026-11-22] - 新建 `cap:yaml_scalar` YAML 1.2 Core Schema 零堆分配标量类型推断、单/双引号解转义与安全引用序列化库（吸收自 bun/src/interchange/yaml.cp）（5/5 PASS，自动动态挂载 `cap.yaml_scalar`）
新增 cap:yaml_scalar 标准库模块（lib/yaml_scalar/{scalar,index}.cp, doc/lib/yaml_scalar.md, cap/test/test_yaml_scalar_production_deep.cp）

## [2026-11-22] - 新建 `cap:toml_lexer` TOML v1.0 零堆分配流式词法扫描器（21 种记号/4 种字符串/4 进制整数/ISO 8601 日期时间与 Unicode 解转义，吸收自 bun/src/interchange/toml/lexer.cp）（5/5 PASS，自动动态挂载 `cap.toml_lexer`）
新增 cap:toml_lexer 标准库模块（lib/toml_lexer/{lexer,index}.cp, doc/lib/toml_lexer.md, cap/test/test_toml_lexer_production_deep.cp）

## [2026-11-22] - 新建 `cap:linear_fifo` 零堆分配泛型环形缓冲区 FIFO（支持 item/slice 读写、peek、unget、discard 与环形回绕，吸收自 bun/src/collections/linear_fifo.cp）（5/5 PASS，自动动态挂载 `cap.linear_fifo`）
新增 cap:linear_fifo 标准库模块（lib/linear_fifo/{fifo,index}.cp, doc/lib/linear_fifo.md, cap/test/test_linear_fifo_production_deep.cp）

## [2026-11-22] - 新建 `cap:alt_svc` RFC 7838 HTTP Alt-Svc 头部解析器与零堆分配 64 源 TTL 缓存（HTTP/3 升级发现，吸收自 bun/src/http/h3_client/AltSvc.cp）（5/5 PASS，自动动态挂载 `cap.alt_svc`）
新增 cap:alt_svc 标准库模块（lib/alt_svc/{alt_svc,index}.cp, doc/lib/alt_svc.md, cap/test/test_alt_svc_production_deep.cp）

## [2026-11-22] - 新建 `cap:fs_router` Next.js / Bun FileSystemRouter 风格文件路由解析、静态>动态>捕获优先级排序与零拷贝参数匹配引擎（吸收自 bun/src/router/{Route,route_types}.cp）（5/5 PASS，自动动态挂载 `cap.fs_router`）
新增 cap:fs_router 标准库模块（lib/fs_router/{router,index}.cp, doc/lib/fs_router.md, cap/test/test_fs_router_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_method` HTTP / WebDAV / QUERY 全 36 种方法枚举、RFC 9110 安全/幂等/可缓存/CORS 谓词与 64-bit MethodSet（吸收自 bun/src/http_types/Method.cp）（5/5 PASS，自动动态挂载 `cap.http_method`）
新增 cap:http_method 标准库模块（lib/http_method/{method,index}.cp, doc/lib/http_method.md, cap/test/test_http_method_production_deep.cp）

## [2026-11-22] - 新建 `cap:http_fetch_meta` WHATWG Fetch 请求元数据枚举、RFC 9110 Accept-Encoding q 权重内容协商与零拷贝 URLPath 拆分库（吸收自 bun/src/http_types/{Encoding,Fetch*,URLPath}.cp）（5/5 PASS，自动动态挂载 `cap.http_fetch_meta`）
新增 cap:http_fetch_meta 标准库模块（lib/http_fetch_meta/{meta,index}.cp, doc/lib/http_fetch_meta.md, cap/test/test_http_fetch_meta_production_deep.cp）

## [2026-11-22] - 新建 `cap:compile_target` 跨平台二进制分发与交叉编译目标三元组解析、支持矩阵校验与 Zig 三元组映射库（吸收自 bun/src/options_types/CompileTarget.cp）（5/5 PASS，自动动态挂载 `cap.compile_target`）
新增 cap:compile_target 标准库模块（lib/compile_target/{target,index}.cp, doc/lib/compile_target.md, cap/test/test_compile_target_production_deep.cp）

## [2026-11-22] - 新建 `cap:bit_ops` 零开销 u64 位掩码、位域提取/插入、2 的幂对齐、popcount/clz/ctz 与置位迭代器库（吸收自 bun/src/meta/bits.cp）（5/5 PASS，自动动态挂载 `cap.bit_ops`）
新增 cap:bit_ops 标准库模块（lib/bit_ops/{bits,index}.cp, doc/lib/bit_ops.md, cap/test/test_bit_ops_production_deep.cp）

## [2026-11-22] - 新建 `cap:path_resolve` POSIX 路径零堆分配规范化、拼接、相对路径、最长公共目录前缀与防目录穿越校验库（吸收自 bun/src/paths/resolve_path.cp）（5/5 PASS，自动动态挂载 `cap.path_resolve`）
新增 cap:path_resolve 标准库模块（lib/path_resolve/{resolve,index}.cp, doc/lib/path_resolve.md, cap/test/test_path_resolve_production_deep.cp）

## [2026-11-22] - 新建 `cap:pkg_exports` Node.js / Bun package.json exports 条件子路径解析引擎（精确优先/PATTERN_KEY_COMPARE/null 阻断/.. 防穿越，吸收自 bun/src/resolver/package_json.cp）（5/5 PASS，自动动态挂载 `cap.pkg_exports`）
新增 cap:pkg_exports 标准库模块（lib/pkg_exports/{exports,index}.cp, doc/lib/pkg_exports.md, cap/test/test_pkg_exports_production_deep.cp）

## [2026-11-22] - 新建 `cap:json5_lexer` JSON5 零堆分配词法扫描器（注释/单引号/无引号键/十六进制/Infinity/NaN）与 RFC 8259 标准 JSON 转译器（吸收自 bun/src/interchange/json5.cp）（5/5 PASS，自动动态挂载 `cap.json5_lexer`）
新增 cap:json5_lexer 标准库模块（lib/json5_lexer/{json5,index}.cp, doc/lib/json5_lexer.md, cap/test/test_json5_lexer_production_deep.cp）

## [2026-11-22] - 新建 `cap:etag` RFC 9110 / RFC 7232 HTTP ETag 解析、强/弱比较、If-None-Match/If-Match 求值与 FNV-1a 内容 ETag 生成库（吸收自 bun/src/http_types/ETag.cp）（5/5 PASS，自动动态挂载 `cap.etag`）
新增 cap:etag 标准库模块（lib/etag/{etag,index}.cp, doc/lib/etag.md, cap/test/test_etag_production_deep.cp）

## [2026-11-22] - 新建 `cap:gitconfig_ini` Git Config / INI 零堆分配解析器（支持 subsection、注释、引号、布尔解析与 url.<base>.insteadOf 重写，吸收自 bun/src/install/repository.cp）（5/5 PASS，自动动态挂载 `cap.gitconfig_ini`）
新增 cap:gitconfig_ini 标准库模块（lib/gitconfig_ini/{gitconfig,index}.cp, doc/lib/gitconfig_ini.md, cap/test/test_gitconfig_ini_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_image_set` CSS Images 4 <image> 与 image-set() 分辨率换算、MIME 过滤 DPR 选图与序列化库（吸收自 bun/src/css/values/image.cp）（5/5 PASS，自动动态挂载 `cap.css_image_set`）
新增 cap:css_image_set 标准库模块（lib/css_image_set/{image,index}.cp, doc/lib/css_image_set.md, cap/test/test_css_image_set_production_deep.cp）

## [2026-11-22] - 新建 `cap:tsconfig_paths` TSConfig compilerOptions.paths 最长前缀通配符路径映射解析器与 ${configDir} 模板展开器（吸收自 bun/src/resolver/tsconfig_json.cp）（5/5 PASS，自动动态挂载 `cap.tsconfig_paths`）
新增 cap:tsconfig_paths 标准库模块（lib/tsconfig_paths/{paths,index}.cp, doc/lib/tsconfig_paths.md, cap/test/test_tsconfig_paths_production_deep.cp）

## [2026-11-22] - 新建 `cap:data_url` RFC 2397 Data URL 零堆分配解析、percent/base64 双解码与最短编码择优器（吸收自 bun/src/resolver/data_url.cp）（5/5 PASS，自动动态挂载 `cap.data_url`）
新增 cap:data_url 标准库模块（lib/data_url/{data_url,index}.cp, doc/lib/data_url.md, cap/test/test_data_url_production_deep.cp）

## [2026-11-22] - 新建 `cap:html_asset_scanner` 零堆分配 HTML 资源引用扫描器（支持 script/link/img/video/audio/source、精确字节偏移原位重写与 srcset 解析，吸收自 bun/src/bundler/HTMLScanner.cp）（5/5 PASS，自动动态挂载 `cap.html_asset_scanner`）
新增 cap:html_asset_scanner 标准库模块（lib/html_asset_scanner/{scanner,index}.cp, doc/lib/html_asset_scanner.md, cap/test/test_html_asset_scanner_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_text` CSS Text 3/4 与 Text Decoration 属性族（text-transform/white-space/word-break/text-align/text-decoration 简写，吸收自 bun/src/css/properties/text.cp）（5/5 PASS，自动动态挂载 `cap.css_text`）
新增 cap:css_text 标准库模块（lib/css_text/{text,index}.cp, doc/lib/css_text.md, cap/test/test_css_text_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_list_style` CSS Lists & Counters Level 3 属性库（18 种预定义计数器样式、list-style 简写双 none 消歧与 marker-side，吸收自 bun/src/css/properties/list.cp）（5/5 PASS，自动动态挂载 `cap.css_list_style`）
新增 cap:css_list_style 标准库模块（lib/css_list_style/{list_style,index}.cp, doc/lib/css_list_style.md, cap/test/test_css_list_style_production_deep.cp）

## [2026-11-22] - 新建 `cap:hosted_git_info` GitHub/GitLab/Bitbucket/Gist 托管 Git URL 规范化解析与 https/ssh/sshurl/shortcut/tarball/browse/bugs 多格式生成器（吸收自 bun/src/install/hosted_git_info.cp）（5/5 PASS，自动动态挂载 `cap.hosted_git_info`）
新增 cap:hosted_git_info 标准库模块（lib/hosted_git_info/{git_info,index}.cp, doc/lib/hosted_git_info.md, cap/test/test_hosted_git_info_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_cascade_rules` CSS Cascade 5/6 与作用域规则头（@import layer/supports/media、@scope start/to、@layer 列表）零堆分配解析与序列化（吸收自 bun/src/css/rules/{import,scope,viewport}.cp）（5/5 PASS，自动动态挂载 `cap.css_cascade_rules`）
新增 cap:css_cascade_rules 标准库模块（lib/css_cascade_rules/{cascade_rules,index}.cp, doc/lib/css_cascade_rules.md, cap/test/test_css_cascade_rules_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_position` CSS <position> 1/2/3/4 值二维坐标解析与最短规范化序列化库（吸收自 bun/src/css/values/position.cp）（5/5 PASS，自动动态挂载 `cap.css_position`）
新增 cap:css_position 标准库模块（lib/css_position/{position,index}.cp, doc/lib/css_position.md, cap/test/test_css_position_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_svg` SVG CSS 渲染属性族（SVGPaint/StrokeLinecap/StrokeLinejoin/StrokeDasharray/PaintOrder 与 5 大渲染提示枚举，吸收自 bun/src/css/properties/svg.cp）（5/5 PASS，自动动态挂载 `cap.css_svg`）
新增 cap:css_svg 标准库模块（lib/css_svg/{svg,index}.cp, doc/lib/css_svg.md, cap/test/test_css_svg_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_transition` CSS Transitions 1/2 过渡属性族与多过渡简写列表解析/序列化库（吸收自 bun/src/css/properties/transition.cp）（5/5 PASS，自动动态挂载 `cap.css_transition`）
新增 cap:css_transition 标准库模块（lib/css_transition/{transition,index}.cp, doc/lib/css_transition.md, cap/test/test_css_transition_production_deep.cp）

## [2026-11-22] - 新建 `cap:pkg_matcher` PNPM 兼容三态包名匹配器（全包含/全排除/混合排除优先）与线性时间 */? 通配符引擎（吸收自 bun/src/install/PnpmMatcher.cp）（5/5 PASS，自动动态挂载 `cap.pkg_matcher`）
新增 cap:pkg_matcher 标准库模块（lib/pkg_matcher/{matcher,index}.cp, doc/lib/pkg_matcher.md, cap/test/test_pkg_matcher_production_deep.cp）

## [2026-11-22] - 新建 `cap:clap_stream` 零堆分配流式命令行参数解析器（支持 --long/--long=val/-abc 串联/-oval/-- 位置参数分隔与精确错误码，吸收自 bun/src/clap/streaming.cp）（5/5 PASS，自动动态挂载 `cap.clap_stream`）
新增 cap:clap_stream 标准库模块（lib/clap_stream/{clap,index}.cp, doc/lib/clap_stream.md, cap/test/test_clap_stream_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_border_image` CSS Backgrounds 3 border-image 属性族与双斜杠简写（slice/width/outset/repeat/fill）解析与序列化库（吸收自 bun/src/css/properties/border_image.cp）（5/5 PASS，自动动态挂载 `cap.css_border_image`）
新增 cap:css_border_image 标准库模块（lib/css_border_image/{border_image,index}.cp, doc/lib/css_border_image.md, cap/test/test_css_border_image_production_deep.cp）

## [2026-11-22] - 新建 `cap:shell_util` 零堆分配 POSIX Shell 安全引用/双引号转义、命令行拼接、basename/dirname 路径运算与环境变量解析库（吸收自 bun/src/shell/util.cp 与 builtin/{basename,dirname}.cp）（5/5 PASS，自动动态挂载 `cap.shell_util`）
新增 cap:shell_util 标准库模块（lib/shell_util/{util,index}.cp, doc/lib/shell_util.md, cap/test/test_shell_util_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_layout_ctrl` CSS 布局控制属性库（overflow/text-overflow/position/container-type/contain/z-index，吸收自 bun/src/css/properties/{overflow,position,contain}.cp）（5/5 PASS，自动动态挂载 `cap.css_layout_ctrl`）
新增 cap:css_layout_ctrl 标准库模块（lib/css_layout_ctrl/{layout_ctrl,index}.cp, doc/lib/css_layout_ctrl.md, cap/test/test_css_layout_ctrl_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_effects` CSS Filter Effects 11 种标准滤镜函数链（blur/brightness/contrast/grayscale/hue-rotate/invert/opacity/saturate/sepia/drop-shadow/url）零堆分配解析与序列化（吸收自 bun/src/css/properties/effects.cp）（5/5 PASS，自动动态挂载 `cap.css_effects`）
新增 cap:css_effects 标准库模块（lib/css_effects/{effects,index}.cp, doc/lib/css_effects.md, cap/test/test_css_effects_production_deep.cp）

## [2026-11-22] - 新建 `cap:md_links` CommonMark §6.3-6.5 内联链接、图片、引用链接与 URI/Email 自动链接零堆分配解析器及 URL 百分号编码（吸收自 bun/src/md/links.cp）（5/5 PASS，自动动态挂载 `cap.md_links`）
新增 cap:md_links 标准库模块（lib/md_links/{links,index}.cp, doc/lib/md_links.md, cap/test/test_md_links_production_deep.cp）

## [2026-11-22] - 新建 `cap:semver_query` 包管理器多协议依赖版本描述符解析器（npm/workspace/catalog/github/git/file/link/tarball/npm_alias）与 npm 包名校验器（吸收自 bun/src/semver/SemverQuery.cp）（5/5 PASS，自动动态挂载 `cap.semver_query`）
新增 cap:semver_query 标准库模块（lib/semver_query/{query,index}.cp, doc/lib/semver_query.md, cap/test/test_semver_query_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_selector_builder` CSS 选择器构建器（Servo 右向左复合匹配序布局）与 Selectors Level 4 特异度计算库（吸收自 bun/src/css/selectors/builder.cp）（5/5 PASS，自动动态挂载 `cap.css_selector_builder`）
新增 cap:css_selector_builder 标准库模块（lib/css_selector_builder/{builder,index}.cp, doc/lib/css_selector_builder.md, cap/test/test_css_selector_builder_production_deep.cp）

## [2026-11-22] - 新建 `cap:ci_info` 零堆分配 CI/CD 31 供应商环境与 PR 构建检测器（遵循 ci-info v4，吸收自 bun/src/cli/ci_info.cp 与 codegen/ci_info.ts）（5/5 PASS，自动动态挂载 `cap.ci_info`）
新增 cap:ci_info 标准库模块（lib/ci_info/{ci_info,index}.cp, doc/lib/ci_info.md, cap/test/test_ci_info_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_gradient` CSS Images 4 线性/径向/锥形渐变（含 repeating-* 与双位置色标）零堆分配解析与规范化序列化库（吸收自 bun/src/css/values/gradient.cp）（5/5 PASS，自动动态挂载 `cap.css_gradient`）
新增 cap:css_gradient 标准库模块（lib/css_gradient/{gradient,index}.cp, doc/lib/css_gradient.md, cap/test/test_css_gradient_production_deep.cp）

## [2026-11-22] - 新建 `cap:errno_map` 跨平台 POSIX/OS Errno 69 项核心错误码双射映射、OS 特定错误消息与瞬态重试判定（吸收自 bun/src/sys/coreutils_error_map.cp 与 errno/{linux,darwin}_errno.cp）（5/5 PASS，自动动态挂载 `cap.errno_map`）
新增 cap:errno_map 标准库模块（lib/errno_map/{errno,index}.cp, doc/lib/errno_map.md, cap/test/test_errno_map_production_deep.cp）

## [2026-11-22] - 新建 `cap:progress_tree` 零堆分配层级进度树、节流刷新判定与终端状态行格式化引擎（吸收自 bun/src/bun_core/Progress.cp）（5/5 PASS，自动动态挂载 `cap.progress_tree`）
新增 cap:progress_tree 标准库模块（lib/progress_tree/{progress,index}.cp, doc/lib/progress_tree.md, cap/test/test_progress_tree_production_deep.cp）

## [2026-11-22] - 新建 `cap:md_line_analysis` CommonMark / GFM 块级行分析器（Setext/ATX 标题、HR、围栏代码块、HTML 块 Type 1-7、62 块级标签与 GFM 表格分隔行，吸收自 bun/src/md/line_analysis.cp）（5/5 PASS，自动动态挂载 `cap.md_line_analysis`）
新增 cap:md_line_analysis 标准库模块（lib/md_line_analysis/{line_analysis,index}.cp, doc/lib/md_line_analysis.md, cap/test/test_md_line_analysis_production_deep.cp）

## [2026-11-22] - 新建 `cap:grapheme_cluster` Unicode UAX #29 扩展字素簇边界判定（GB3-GB13/Hangul/ZWJ Emoji/国旗）与零分配 UTF-8 迭代器（吸收自 bun/src/string/immutable/grapheme.cp）（5/5 PASS，自动动态挂载 `cap.grapheme_cluster`）
新增 cap:grapheme_cluster 标准库模块（lib/grapheme_cluster/{grapheme,index}.cp, doc/lib/grapheme_cluster.md, cap/test/test_grapheme_cluster_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_font` CSS Fonts 4 属性族与完整 Font 简写解析/序列化库（吸收自 bun/src/css/properties/font.cp）（5/5 PASS，自动动态挂载 `cap.css_font`）
新增 cap:css_font 标准库模块（lib/css_font/{font,index}.cp, doc/lib/css_font.md, cap/test/test_css_font_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_supports` CSS @supports 条件表达式零分配 Arena 解析、求值与规范化序列化库（吸收自 bun/src/css/rules/supports.cp）（5/5 PASS，自动动态挂载 `cap.css_supports`）
新增 cap:css_supports 标准库模块（lib/css_supports/{supports,index}.cp, doc/lib/css_supports.md, cap/test/test_css_supports_production_deep.cp）

## [2026-11-22] - 新建 `cap:md_ref_defs` CommonMark §4.7 链接引用定义解析、零分配标签规范化与首定义优先查找表（吸收自 bun/src/md/ref_defs.cp）（5/5 PASS，自动动态挂载 `cap.md_ref_defs`）
新增 cap:md_ref_defs 标准库模块（lib/md_ref_defs/{ref_defs,index}.cp, doc/lib/md_ref_defs.md, cap/test/test_md_ref_defs_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_transform` CSS 2D/3D Transform 21 种函数、4x4 齐次矩阵合成、Rodrigues 旋转与原点/样式属性库（吸收自 bun/src/css/properties/transform.cp）（5/5 PASS，自动动态挂载 `cap.css_transform`）
新增 cap:css_transform 标准库模块（lib/css_transform/{transform,index}.cp, doc/lib/css_transform.md, cap/test/test_css_transform_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_background` CSS Background 属性族与多图层简写解析/序列化（吸收自 bun/src/css/properties/background.cp）（5/5 PASS，自动动态挂载 `cap.css_background`）
新增 cap:css_background 标准库模块（lib/css_background/{background,index}.cp, doc/lib/css_background.md, cap/test/test_css_background_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_sizing` CSS Sizing（width/height/max-size/aspect-ratio/fit-content）与盒模型 Insets（margin/padding/inset）属性库（吸收自 bun/src/css/properties/{size,margin_padding}.cp）（5/5 PASS，自动动态挂载 `cap.css_sizing`）
新增 cap:css_sizing 标准库模块（lib/css_sizing/{sizing,index}.cp, doc/lib/css_sizing.md, cap/test/test_css_sizing_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_border` CSS Border 简写、四边宽/样式/颜色与逻辑边属性解析和压缩序列化（吸收自 bun/src/css/properties/border.cp）（5/5 PASS，自动动态挂载 `cap.css_border`）
新增 cap:css_border 标准库模块（lib/css_border/{border,index}.cp, doc/lib/css_border.md, cap/test/test_css_border_production_deep.cp）

## [2026-11-22] - 新建 `cap:semver_range` SemVer 版本比较器与范围匹配引擎（吸收自 bun/src/semver/{SemverRange,Version}.cp）（5/5 PASS，自动动态挂载 `cap.semver_range`）
新增 cap:semver_range 标准库模块（lib/semver_range/{range,index}.cp, doc/lib/semver_range.md, cap/test/test_semver_range_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_flex_grid` CSS Flexbox / Box Alignment / Grid 布局属性解析与序列化（吸收自 bun/src/css/properties/{flex,align,grid}.cp）（5/5 PASS，自动动态挂载 `cap.css_flex_grid`）
新增 cap:css_flex_grid 标准库模块（lib/css_flex_grid/{layout,index}.cp, doc/lib/css_flex_grid.md, cap/test/test_css_flex_grid_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_border_radius` CSS border-radius 椭圆/四值简写解析与规范化压缩序列化（吸收自 bun/src/css/properties/border_radius.cp）（5/5 PASS，自动动态挂载 `cap.css_border_radius`）
新增 cap:css_border_radius 标准库模块（lib/css_border_radius/{radius,index}.cp, doc/lib/css_border_radius.md, cap/test/test_css_border_radius_production_deep.cp）

## [2026-11-22] - CSS box-shadow 与 border/outline 线型属性（5/5 PASS，自动动态挂载 `cap.css_box_shadow`）
新增 cap:css_box_shadow 标准库（lib/css_box_shadow/、doc/lib/css_box_shadow.md、cap/test/test_css_box_shadow_production_deep.cp），2/2 回归通过。

## [2026-11-22] - PostgreSQL v3 COPY 协议与 SASL/Password 前端报文编解码（5/5 PASS，自动动态挂载 `cap.pg_copy_sasl`）
新增 cap:pg_copy_sasl 标准库（lib/pg_copy_sasl/、doc/lib/pg_copy_sasl.md、cap/test/test_pg_copy_sasl_production_deep.cp），5/5 回归通过（零泄漏），PostgreSQL v3 线协议全栈齐备。

## [2026-11-22] - CSS Animations & Transitions 简写解析与序列化（5/5 PASS，自动动态挂载 `cap.css_animation`）
新增 cap:css_animation 标准库（lib/css_animation/、doc/lib/css_animation.md、cap/test/test_css_animation_production_deep.cp），2/2 回归通过。

## [2026-11-22] - MySQL 认证密码学原语与 SSLRequest 编解码（5/5 PASS，自动动态挂载 `cap.mysql_auth`）
新增 cap:mysql_auth 标准库（lib/mysql_auth/、doc/lib/mysql_auth.md、cap/test/test_mysql_auth_production_deep.cp），6/6 回归通过。

## [2026-11-22] - PostgreSQL v3 后端控制与诊断消息解码器（5/5 PASS，自动动态挂载 `cap.pg_messages`）
新增 cap:pg_messages 标准库（lib/pg_messages/、doc/lib/pg_messages.md、cap/test/test_pg_messages_production_deep.cp），5/5 回归通过。

## [2026-11-22] - MySQL 协议服务端握手与控制报文解码器（5/5 PASS，自动动态挂载 `cap.mysql_control`）
新增 cap:mysql_control 标准库（lib/mysql_control/、doc/lib/mysql_control.md、cap/test/test_mysql_control_production_deep.cp），6/6 回归通过。

## [2026-11-22] - PostgreSQL v3 扩展查询协议前端编码器（5/5 PASS，自动动态挂载 `cap.pg_extended_query`）
新增 cap:pg_extended_query 标准库（lib/pg_extended_query/、doc/lib/pg_extended_query.md、cap/test/test_pg_extended_query_production_deep.cp），2/2 回归通过（零泄漏）。

## [2026-11-22] - CSS Basic UI Level 4 属性集（5/5 PASS，自动动态挂载 `cap.css_ui`）
新增 cap:css_ui 标准库（lib/css_ui/、doc/lib/css_ui.md、cap/test/test_css_ui_production_deep.cp），2/2 回归通过。

## [2026-11-22] - CSS Media Queries Level 4 媒体查询解析与求值引擎（5/5 PASS，自动动态挂载 `cap.css_media`）
新增 cap:css_media 标准库（lib/css_media/、doc/lib/css_media.md、cap/test/test_css_media_production_deep.cp），5/5 回归通过。

## [2026-11-22] - CSS Properties & Values API <syntax> 解析引擎（5/5 PASS，自动动态挂载 `cap.css_syntax`）
新增 cap:css_syntax 标准库（lib/css_syntax/、doc/lib/css_syntax.md、cap/test/test_css_syntax_production_deep.cp），6/6 回归通过（零泄漏）。

## [2026-11-22] - CSS @font-face Unicode 码点区间与通配符优化器（5/5 PASS，自动动态挂载 `cap.unicode_range`）
新增 cap:unicode_range 标准库（lib/unicode_range/、doc/lib/unicode_range.md、cap/test/test_unicode_range_production_deep.cp），2/2 回归通过。

## [2026-11-22] - CSS 基础值类型（Alpha / Percentage / URL 分类）（5/5 PASS，自动动态挂载 `cap.css_primitives`）
新增 cap:css_primitives 标准库（lib/css_primitives/、doc/lib/css_primitives.md、cap/test/test_css_primitives_production_deep.cp），2/2 回归通过。

## [2026-11-22] - CSS Container Queries @container 规则与求值引擎（5/5 PASS，自动动态挂载 `cap.css_container`）
新增 cap:css_container 标准库（lib/css_container/、doc/lib/css_container.md、cap/test/test_css_container_production_deep.cp），5/5 回归通过。

## [2026-11-22] - CSS <angle> 与 <resolution> 单位解析与换算库（5/5 PASS，自动动态挂载 `cap.css_units`）
新增 cap:css_units 标准库（lib/css_units/、doc/lib/css_units.md、cap/test/test_css_units_production_deep.cp），6/6 回归通过。

## [2026-11-22] - MySQL 4.1 握手响应报文编解码器（5/5 PASS，自动动态挂载 `cap.mysql_handshake`）
新增 cap:mysql_handshake 标准库（lib/mysql_handshake/、doc/lib/mysql_handshake.md、cap/test/test_mysql_handshake_production_deep.cp），2/2 回归通过（零泄漏）。

## [2026-11-22] - ECMAScript/Web/Node/Bun 已知全局符号与纯度分析器（5/5 PASS，自动动态挂载 `cap.known_globals`）
新增 cap:known_globals 标准库（lib/known_globals/、doc/lib/known_globals.md、cap/test/test_known_globals_production_deep.cp），5/5 回归通过。

## [2026-11-22] - 二分查找源码行列偏移表 LineOffsetTable（5/5 PASS，自动动态挂载 `cap.line_offset_table`）
新增 cap:line_offset_table 标准库（lib/line_offset_table/、doc/lib/line_offset_table.md、cap/test/test_line_offset_table_production_deep.cp），6/6 回归通过（零泄漏）。

## [2026-11-22] - CSS @keyframes/@layer/@counter-style/@property 规则集（5/5 PASS，自动动态挂载 `cap.css_at_rules`）
新增 cap:css_at_rules 标准库（lib/css_at_rules/、doc/lib/css_at_rules.md、cap/test/test_css_at_rules_production_deep.cp），5/5 回归通过。

## [2026-11-22] - CSS <time> 时间值解析、运算与规范序列化（5/5 PASS，自动动态挂载 `cap.css_time`）
新增 cap:css_time 标准库（lib/css_time/、doc/lib/css_time.md、cap/test/test_css_time_production_deep.cp），2/2 回归通过。

## [2026-11-22] - ThumbHash 超紧凑图片占位符编解码器（5/5 PASS，自动动态挂载 `cap.thumbhash`）
新增 cap:thumbhash 标准库（lib/thumbhash/、doc/lib/thumbhash.md、cap/test/test_thumbhash_production_deep.cp），3/3 回归通过（零泄漏）。

## [2026-11-22] - 零解码极速图片头格式与尺寸探测器（5/5 PASS，自动动态挂载 `cap.image_probe`）
新增 cap:image_probe 标准库（lib/image_probe/、doc/lib/image_probe.md、cap/test/test_image_probe_production_deep.cp），6/6 回归通过。
