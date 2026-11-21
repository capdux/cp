# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:git_ref_check` Git 引用名格式校验器（git-check-ref-format 10 条规则）、refs/* 拆解与 SHA-1/SHA-256 OID 校验（5/5 PASS，自动动态挂载 `cap.git_ref_check`）
新增 `cap:git_ref_check`（`lib/git_ref_check/{git_ref,index}.cp`、`doc/lib/git_ref_check.md`、`cap/test/test_git_ref_check_production_deep.cp`），完整覆盖 `git-check-ref-format` 全部规则与 `refs/heads|tags|remotes` 分类。

## [2026-11-22] - 新建 `cap:json_escape` RFC 8259 JSON 字符串零分配转义器、加引号器与支持 UTF-16 代理对解码的严格反转义器（5/5 PASS，自动动态挂载 `cap.json_escape`）
新增 `cap:json_escape`（`lib/json_escape/{json_escape,index}.cp`、`doc/lib/json_escape.md`、`cap/test/test_json_escape_production_deep.cp`），完整支持 `\uD83D\uDE00` 代理对转 4 字节 UTF-8 并拒绝孤立代理项与裸控制字符。

## [2026-11-22] - 新建 `cap:http_permissions_policy` W3C Permissions-Policy 响应头零分配解析器与特性权限（disabled/self/*/origin）查询器（5/5 PASS，自动动态挂载 `cap.http_permissions_policy`）
新增 `cap:http_permissions_policy`（`lib/http_permissions_policy/{permissions_policy,index}.cp`、`doc/lib/http_permissions_policy.md`、`cap/test/test_http_permissions_policy_production_deep.cp`），严格区分未引号 `self`/`*` 与引号 origin。

## [2026-11-22] - 新建 `cap:http_content_type` RFC 9110 §8.3.1 Content-Type 零分配解析器、参数提取器（charset/boundary）与媒体类型分类器（5/5 PASS，自动动态挂载 `cap.http_content_type`）
新增 `cap:http_content_type`（`lib/http_content_type/{content_type,index}.cp`、`doc/lib/http_content_type.md`、`cap/test/test_http_content_type_production_deep.cp`），支持 `+json`/`+xml`/multipart/form/SSE 分类与引号参数提取。

## [2026-11-22] - 新建 `cap:codeowners` GitHub / GitLab CODEOWNERS 文件零分配规则解析与后匹配代码归属求值引擎（5/5 PASS，自动动态挂载 `cap.codeowners`）
新增 `cap:codeowners`（`lib/codeowners/{codeowners,index}.cp`、`doc/lib/codeowners.md`、`cap/test/test_codeowners_production_deep.cp`），支持 GitLab 段头跳过、转义空格与 GitHub 后匹配覆盖语义。

## [2026-11-22] - 新建 `cap:conventional_commit` Conventional Commits 1.0.0 提交信息零分配解析器、破坏性变更检测与 SemVer 升级建议器（5/5 PASS，自动动态挂载 `cap.conventional_commit`）
新增 `cap:conventional_commit`（`lib/conventional_commit/{commit,index}.cp`、`doc/lib/conventional_commit.md`、`cap/test/test_conventional_commit_production_deep.cp`），支持 `type(scope)!: desc`、`BREAKING CHANGE:` 页脚检测与自动版本升级建议。

## [2026-11-22] - 新建 `cap:http_security_headers` Web 安全响应头解析与构造器（HSTS RFC 6797、Referrer-Policy 回退链、X-Frame-Options、COOP/COEP/CORP 跨域隔离）（5/5 PASS，自动动态挂载 `cap.http_security_headers`）
新增 `cap:http_security_headers`（`lib/http_security_headers/{security_headers,index}.cp`、`doc/lib/http_security_headers.md`、`cap/test/test_http_security_headers_production_deep.cp`），完整覆盖 6 类核心 Web 安全头。

## [2026-11-22] - 新建 `cap:http_retry_after` RFC 9110 §10.2.3 Retry-After 响应头解析器（delta-seconds 与 IMF-fixdate）与限幅等待时长计算（5/5 PASS，自动动态挂载 `cap.http_retry_after`）
新增 `cap:http_retry_after`（`lib/http_retry_after/{retry_after,index}.cp`、`doc/lib/http_retry_after.md`、`cap/test/test_http_retry_after_production_deep.cp`），支持整数秒与 HTTP 日期解析、相对等待秒数及上限夹紧。

## [2026-11-22] - 新建 `cap:frontmatter` Markdown 文档 YAML (---) / TOML (+++) Frontmatter 零分配提取器与键值查询（5/5 PASS，自动动态挂载 `cap.frontmatter`）
新增 `cap:frontmatter`（`lib/frontmatter/{frontmatter,index}.cp`、`doc/lib/frontmatter.md`、`cap/test/test_frontmatter_production_deep.cp`），支持 UTF-8 BOM、LF/CRLF、YAML/TOML 块分离与引号/注释剥离查询。

## [2026-11-22] - 新建 `cap:cli_progress` 零分配 CLI 进度条渲染器、百分比/纯整数 ETA 估算器与 ASCII/盲文点阵旋转动画帧生成器（5/5 PASS，自动动态挂载 `cap.cli_progress`）
新增 `cap:cli_progress`（`lib/cli_progress/{progress,index}.cp`、`doc/lib/cli_progress.md`、`cap/test/test_cli_progress_production_deep.cp`），提供进度夹紧、`u128` 防溢出百分比与 ETA 计算。

## [2026-11-22] - 新建 `cap:dns_name` RFC 1035 / RFC 1123 DNS 域名与标签合法性校验、线缆格式编解码与严格点边界子域匹配器（5/5 PASS，自动动态挂载 `cap.dns_name`）
新增 `cap:dns_name`（`lib/dns_name/{dns_name,index}.cp`、`doc/lib/dns_name.md`、`cap/test/test_dns_name_production_deep.cp`），支持 RFC 1035 线缆格式往返、压缩指针拒绝与大小写无关子域匹配。

## [2026-11-22] - 新建 `cap:markdown_table` GFM Markdown 管道表格行零分配单元格迭代器、对齐分隔行检测与单元格管道符转义器（5/5 PASS，自动动态挂载 `cap.markdown_table`）
新增 `cap:markdown_table`（`lib/markdown_table/{table,index}.cp`、`doc/lib/markdown_table.md`、`cap/test/test_markdown_table_production_deep.cp`），支持转义 `\|` 切分、`:---`/`:---:`/`---:` 对齐识别与单元格转义。

## [2026-11-22] - 新建 `cap:dockerfile_parse` Dockerfile 指令零分配流式解析器（支持 # escape 续行折叠、注释跳过、FROM 阶段与 EXPOSE 端口拆解）（5/5 PASS，自动动态挂载 `cap.dockerfile_parse`）
新增 `cap:dockerfile_parse`（`lib/dockerfile_parse/{dockerfile,index}.cp`、`doc/lib/dockerfile_parse.md`、`cap/test/test_dockerfile_parse_production_deep.cp`），实现多行续行折叠、行号追踪、`FROM --platform=... AS stage` 与 `EXPOSE` 端口解析。

## [2026-11-22] - 新建 `cap:http_origin` WHATWG URL Origin 元组解析器、默认端口规范化、Same-Origin 比较与安全上下文判定（5/5 PASS，自动动态挂载 `cap.http_origin`）
新增 `cap:http_origin`（`lib/http_origin/{origin,index}.cp`、`doc/lib/http_origin.md`、`cap/test/test_http_origin_production_deep.cp`），支持 IPv6 `[::1]`、userinfo 剥离、默认端口归约与 `*.localhost` 可信上下文判定。

## [2026-11-22] - 新建 `cap:spdx_license` SPDX 2.3 开源许可证表达式词法/递归下降语法校验器、ID 提取器与常见宽松许可证判定（5/5 PASS，自动动态挂载 `cap.spdx_license`）
新增 `cap:spdx_license`（`lib/spdx_license/{spdx,index}.cp`、`doc/lib/spdx_license.md`、`cap/test/test_spdx_license_production_deep.cp`），完整支持 `AND`/`OR`/`WITH`、`LicenseRef-`/`DocumentRef-` 与 64 层递归深度防护。

## [2026-11-22] - 新建 `cap:passwd_parse` POSIX /etc/passwd 与 /etc/group 零分配流式解析器、UID/GID/名称查询与组成员判定（5/5 PASS，自动动态挂载 `cap.passwd_parse`）
新增 `cap:passwd_parse`（`lib/passwd_parse/{passwd,index}.cp`、`doc/lib/passwd_parse.md`、`cap/test/test_passwd_parse_production_deep.cp`），实现 7 字段 passwd 与 4 字段 group 严格解析与 nologin shell 检测。

## [2026-11-22] - 新建 `cap:http_csp` W3C Content-Security-Policy Level 3 响应头零分配解析器、default-src 回退、nonce 校验与策略构造器（5/5 PASS，自动动态挂载 `cap.http_csp`）
新增 `cap:http_csp`（`lib/http_csp/{csp,index}.cp`、`doc/lib/http_csp.md`、`cap/test/test_http_csp_production_deep.cp`），遵循 W3C CSP3 §2.2.1 首现优先、大小写敏感 nonce 校验与 fetch 指令回退。

## [2026-11-22] - 新建 `cap:rfc3339_time` RFC 3339 / ISO 8601 时间戳零分配解析器、闰年/时区校验、Unix 纪元秒互转与规范化格式化器（5/5 PASS，自动动态挂载 `cap.rfc3339_time`）
新增 `cap:rfc3339_time`（`lib/rfc3339_time/{rfc3339,index}.cp`、`doc/lib/rfc3339_time.md`、`cap/test/test_rfc3339_time_production_deep.cp`），基于 Howard Hinnant 算法实现 1970 前负时间戳与纳秒精度往返。

## [2026-11-22] - 新建 `cap:http_server_timing` W3C Server-Timing 响应头零分配解析器与构造器（纯整数微秒精度与引号转义）（5/5 PASS，自动动态挂载 `cap.http_server_timing`）
新增 `cap:http_server_timing`（`lib/http_server_timing/{server_timing,index}.cp`、`doc/lib/http_server_timing.md`、`cap/test/test_http_server_timing_production_deep.cp`），支持引号逗号/分号、亚毫秒微秒整数解析与原子回滚构造。

## [2026-11-22] - 新建 `cap:slugify` 零分配 URL 安全 Slug 生成器（标点折叠、撇号省略、词边界截断与规范校验）（5/5 PASS，自动动态挂载 `cap.slugify`）
新增 `cap:slugify`（`lib/slugify/{slugify,index}.cp`、`doc/lib/slugify.md`、`cap/test/test_slugify_production_deep.cp`），实现 URL 友好 Slug 生成、自定义分隔符、词边界有界截断与校验。

## [2026-11-22] - 新建 `cap:ssh_pubkey` OpenSSH 公钥与 authorized_keys 文件零分配解析器（含选项字段、Base64 校验与 FIDO2 SK 识别）（5/5 PASS，自动动态挂载 `cap.ssh_pubkey`）
新增 `cap:ssh_pubkey`（`lib/ssh_pubkey/{pubkey,index}.cp`、`doc/lib/ssh_pubkey.md`、`cap/test/test_ssh_pubkey_production_deep.cp`），支持 `ssh-ed25519`/`ssh-rsa`/`ecdsa`/`sk-*` 与带引号选项解析。

## [2026-11-22] - 新建 `cap:punycode` RFC 3492 Punycode / Bootstring 零分配编解码器与 xn-- 国际化域名标签转换器（5/5 PASS，自动动态挂载 `cap.punycode`）
新增 `cap:punycode`（`lib/punycode/{punycode,index}.cp`、`doc/lib/punycode.md`、`cap/test/test_punycode_production_deep.cp`），完整通过中/日/韩/德语/Emoji 测试向量与溢出/代理项防护。

## [2026-11-22] - 新建 `cap:quoted_printable` RFC 2045 §6.7 MIME Quoted-Printable 零分配编解码器（76 列软换行与行尾空白转义）（5/5 PASS，自动动态挂载 `cap.quoted_printable`）
新增 `cap:quoted_printable`（`lib/quoted_printable/{qp,index}.cp`、`doc/lib/quoted_printable.md`、`cap/test/test_quoted_printable_production_deep.cp`），支持 76 列软换行、尾空格转义及严格/宽松解码。

## [2026-11-22] - 新建 `cap:semver_bump` SemVer 2.0.0 版本号递增器（major/minor/patch/prerelease/release）与预发布标签剥离器（5/5 PASS，自动动态挂载 `cap.semver_bump`）
新增 `cap:semver_bump`（`lib/semver_bump/{bump,index}.cp`、`doc/lib/semver_bump.md`、`cap/test/test_semver_bump_production_deep.cp`），支持全套 SemVer 递增语义与 `preid` 切换。

## [2026-11-22] - 新建 `cap:rate_sliding_window` 加权滑动窗口计数器与 GCRA（通用信元速率算法）零分配纯整数平滑限流器（5/5 PASS，自动动态挂载 `cap.rate_sliding_window`）
新增 `cap:rate_sliding_window`（`lib/rate_sliding_window/{sliding_window,index}.cp`、`doc/lib/rate_sliding_window.md`、`cap/test/test_rate_sliding_window_production_deep.cp`），提供平滑跨窗口限流与 GCRA 精确 retry-after 毫秒计算。

## [2026-11-22] - 新建 `cap:http_if_match` RFC 9110 §13.1/§13.2.2 HTTP 条件请求求值器（If-Match / If-None-Match / If-Modified-Since / If-Unmodified-Since）（5/5 PASS，自动动态挂载 `cap.http_if_match`）
新增 `cap:http_if_match`（`lib/http_if_match/{if_match,index}.cp`、`doc/lib/http_if_match.md`、`cap/test/test_http_if_match_production_deep.cp`），实现强/弱 ETag 比较与 RFC 9110 §13.2.2 优先级求值（200/304/412）。

## [2026-11-22] - 新建 `cap:structured_headers` RFC 8941 HTTP Structured Field Values 标量项（整数/小数/字符串/Token/字节序列/布尔）与参数列表零分配解析器（5/5 PASS，自动动态挂载 `cap.structured_headers`）
新增 `cap:structured_headers`（`lib/structured_headers/{sfv,index}.cp`、`doc/lib/structured_headers.md`、`cap/test/test_structured_headers_production_deep.cp`），完整覆盖 6 种 SFV 标量项、15 位整数/3 位小数上限校验与参数迭代器。

## [2026-11-22] - 新建 `cap:duration_parse` 纯整数人类可读时长解析器（ns/us/µs/ms/s/m/h/d/w 复合与小数）及规范/紧凑格式化器（5/5 PASS，自动动态挂载 `cap.duration_parse`）
新增 `cap:duration_parse`（`lib/duration_parse/{duration,index}.cp`、`doc/lib/duration_parse.md`、`cap/test/test_duration_parse_production_deep.cp`），纯整数防浮点误差与溢出，支持无损复合格式化往返。

## [2026-11-22] - 新建 `cap:http_form` WHATWG application/x-www-form-urlencoded 零分配编解码器（+ 与空格互转）、迭代器与构造器（5/5 PASS，自动动态挂载 `cap.http_form`）
新增 `cap:http_form`（`lib/http_form/{form,index}.cp`、`doc/lib/http_form.md`、`cap/test/test_http_form_production_deep.cp`），完整实现 WHATWG 表单编解码、畸形 `%` 保留与 `appendFormParam`/`getFormParam` 往返。

## [2026-11-22] - 新建 `cap:case_convert` 零分配缩写感知标识符命名风格转换器（snake_case / SCREAMING_SNAKE_CASE / kebab-case / camelCase / PascalCase）（5/5 PASS，自动动态挂载 `cap.case_convert`）
新增 `cap:case_convert`（`lib/case_convert/{case_convert,index}.cp`、`doc/lib/case_convert.md`、`cap/test/test_case_convert_production_deep.cp`），支持 `HTTPServer` 等缩写边界拆词与 5 种命名风格转换。

## [2026-11-22] - 新建 `cap:byte_size` 纯整数人类可读字节大小解析器（SI/IEC 小数支持与溢出防护）及 SI/IEC 格式化器（5/5 PASS，自动动态挂载 `cap.byte_size`）
新增 `cap:byte_size`（`lib/byte_size/{byte_size,index}.cp`、`doc/lib/byte_size.md`、`cap/test/test_byte_size_production_deep.cp`），纯整数算术解析 `KB`/`KiB`/`MB`/`MiB`/`GB`/`GiB` 等单位并提供双制式格式化。

## [2026-11-22] - 新建 `cap:w3c_traceparent` W3C Trace Context Level 1 traceparent / tracestate 分布式追踪头零分配解析与格式化器（5/5 PASS，自动动态挂载 `cap.w3c_traceparent`）
新增 `cap:w3c_traceparent`（`lib/w3c_traceparent/{trace_context,index}.cp`、`doc/lib/w3c_traceparent.md`、`cap/test/test_w3c_traceparent_production_deep.cp`），严格校验 55 字节小写 hex、拒绝全零 ID 与 `0xff` 版本，支持多租户 `tracestate`。

## [2026-11-22] - 新建 `cap:text_wrap` 零分配 CLI 贪心自动换行、缩进/公共反缩进与 UTF-8 安全省略号截断工具（5/5 PASS，自动动态挂载 `cap.text_wrap`）
新增 `cap:text_wrap`（`lib/text_wrap/{wrap,index}.cp`、`doc/lib/text_wrap.md`、`cap/test/test_text_wrap_production_deep.cp`），支持段落保留、公共缩进消除与 UTF-8 码点边界安全截断。

## [2026-11-22] - 新建 `cap:sql_escape` SQL 标识符引用、字面量转义与 LIKE 通配符安全转义器（覆盖 PostgreSQL/ANSI 与 MySQL 方言）（5/5 PASS，自动动态挂载 `cap.sql_escape`）
新增 `cap:sql_escape`（`lib/sql_escape/{escape,index}.cp`、`doc/lib/sql_escape.md`、`cap/test/test_sql_escape_production_deep.cp`），实现双引号/反引号标识符、限定名、单引号字面量与 LIKE 模式转义。

## [2026-11-22] - 新建 `cap:html_escape` 零分配 HTML 实体转义与命名/十进制/十六进制数字字符引用 UTF-8 反转义器（5/5 PASS，自动动态挂载 `cap.html_escape`）
新增 `cap:html_escape`（`lib/html_escape/{escape,index}.cp`、`doc/lib/html_escape.md`、`cap/test/test_html_escape_production_deep.cp`），支持 4 字节 UTF-8 码点还原与代理项/NUL 安全替换 U+FFFD。

## [2026-11-22] - 新建 `cap:uri_normalize` RFC 3986 URI 路径点段归约（§5.2.4）、百分号编码规范化与协议默认端口剥离工具（5/5 PASS，自动动态挂载 `cap.uri_normalize`）
新增 `cap:uri_normalize`（`lib/uri_normalize/{normalize,index}.cp`、`doc/lib/uri_normalize.md`、`cap/test/test_uri_normalize_production_deep.cp`），完整实现 RFC 3986 §5.2.4 规则 A-E 与 Origin 规范化。

## [2026-11-22] - 新建 `cap:logfmt` 结构化 logfmt（key=value）日志行零分配流式解析器、键值查询与规范化转义编码器（5/5 PASS，自动动态挂载 `cap.logfmt`）
新增 `cap:logfmt`（`lib/logfmt/{logfmt,index}.cp`、`doc/lib/logfmt.md`、`cap/test/test_logfmt_production_deep.cp`），支持引号转义、无值布尔标记与自动引号编码往返。

## [2026-11-22] - 新建 `cap:npm_package_name` npm / Node.js 包名与模块说明符零分配解析器（作用域/子路径拆分与 validate-npm-package-name 校验）（5/5 PASS，自动动态挂载 `cap.npm_package_name`）
新增 `cap:npm_package_name`（`lib/npm_package_name/{pkg_name,index}.cp`、`doc/lib/npm_package_name.md`、`cap/test/test_npm_package_name_production_deep.cp`），实现 `@scope/pkg/subpath` 解析、裸说明符判定与合法性校验。

## [2026-11-22] - 新建 `cap:editorconfig` .editorconfig 零分配解析器、花括号/通配符段匹配与级联属性求值器（5/5 PASS，自动动态挂载 `cap.editorconfig`）
新增 `cap:editorconfig`（`lib/editorconfig/{editorconfig,index}.cp`、`doc/lib/editorconfig.md`、`cap/test/test_editorconfig_production_deep.cp`），支持 `root=true`、`{a,b}` 段匹配与 8 项核心编辑器属性级联。

## [2026-11-22] - 新建 `cap:env_expand` POSIX Shell 风格环境变量模板零分配展开器（支持 $VAR/${VAR:-def}/${VAR:+alt}/嵌套展开与转义）（5/5 PASS，自动动态挂载 `cap.env_expand`）
新增 `cap:env_expand`（`lib/env_expand/{expand,index}.cp`、`doc/lib/env_expand.md`、`cap/test/test_env_expand_production_deep.cp`），实现 4 种 POSIX 参数修饰符、8 层嵌套上限与 `$$`/`\$` 转义。

## [2026-11-22] - 新建 `cap:http_digest` RFC 9530 Content-Digest / Repr-Digest 与 Want-Content-Digest 结构化字段零分配解析与算法协商（5/5 PASS，自动动态挂载 `cap.http_digest`）
新增 `cap:http_digest`（`lib/http_digest/{digest,index}.cp`、`doc/lib/http_digest.md`、`cap/test/test_http_digest_production_deep.cp`），实现 RFC 8941 字节序列字典解析、规范格式化与权重协商。

## [2026-11-22] - 新建 `cap:gitignore_match` Git .gitignore 规则集零分配解析与路径匹配引擎（支持 **/! 反忽略/目录锚定与字符类）（5/5 PASS，自动动态挂载 `cap.gitignore_match`）
新增 `cap:gitignore_match`（`lib/gitignore_match/{gitignore,index}.cp`、`doc/lib/gitignore_match.md`、`cap/test/test_gitignore_match_production_deep.cp`），实现完整 `.gitignore` 规则解析、后置覆盖与 `!` 反忽略。

## [2026-11-22] - 新建 `cap:http_range` RFC 9110 §14.1.2 HTTP Range 请求头（bytes=start-end / start- / -suffix）零分配解析与实体区间求值器（5/5 PASS，自动动态挂载 `cap.http_range`）
新增 `cap:http_range`（`lib/http_range/{range,index}.cp`、`doc/lib/http_range.md`、`cap/test/test_http_range_production_deep.cp`），支持多段区间迭代、末段截断与 HTTP 416 不可满足判定。

## [2026-11-22] - 新建 `cap:semver_sort` SemVer 2.0.0 §11 严格优先级比较、预发布标签无溢出比较与原地升降序排序/极值选择（5/5 PASS，自动动态挂载 `cap.semver_sort`）
新增 `cap:semver_sort`（`lib/semver_sort/{sort,index}.cp`、`doc/lib/semver_sort.md`、`cap/test/test_semver_sort_production_deep.cp`），完整实现 SemVer 2.0.0 §11 比较、原地稳定排序与非法版本原子回退。

## [2026-11-22] - 新建 `cap:cli_color_detect` 终端 ANSI 色彩能力探测（遵循 NO_COLOR / FORCE_COLOR / CLICOLOR / COLORTERM / TERM / CI 规范）（5/5 PASS，自动动态挂载 `cap.cli_color_detect`）
新增 `cap:cli_color_detect`（`lib/cli_color_detect/{color_detect,index}.cp`、`doc/lib/cli_color_detect.md`、`cap/test/test_cli_color_detect_production_deep.cp`），纯函数确定性探测 `none`/`basic16`/`ansi256`/`truecolor`。

## [2026-11-22] - 新建 `cap:glob_escape` Glob 通配符元字符转义/反转义、静态目录前缀提取（遍历剪枝）与否定模式解析（5/5 PASS，自动动态挂载 `cap.glob_escape`）
新增 `cap:glob_escape`（`lib/glob_escape/{glob_escape,index}.cp`、`doc/lib/glob_escape.md`、`cap/test/test_glob_escape_production_deep.cp`），实现 Glob 元字符识别、转义往返与 `staticBaseDir` 静态前缀剪枝。

## [2026-11-22] - 新建 `cap:http_vary` RFC 9110 §12.5.5 Vary 响应头零分配迭代、通配符检测与大小写不敏感去重合并器（5/5 PASS，自动动态挂载 `cap.http_vary`）
新增 `cap:http_vary`（`lib/http_vary/{vary,index}.cp`、`doc/lib/http_vary.md`、`cap/test/test_http_vary_production_deep.cp`），实现 `Vary` 字段合法性校验、`*` 塌缩与规范化小写合并。

## [2026-11-22] - 新建 `cap:hex_dump` 零分配十六进制编解码与规范化 hexdump -C 多行诊断格式化器（5/5 PASS，自动动态挂载 `cap.hex_dump`）
新增 `cap:hex_dump`（`lib/hex_dump/{hex,index}.cp`、`doc/lib/hex_dump.md`、`cap/test/test_hex_dump_production_deep.cp`），提供严格/宽松十六进制编解码与 16 字节 `hexdump -C` 诊断输出。

## [2026-11-22] - 新建 `cap:http_prefer` RFC 7240 Prefer 与 Preference-Applied HTTP 头零分配解析器与格式化器（5/5 PASS，自动动态挂载 `cap.http_prefer`）
新增 `cap:http_prefer`（`lib/http_prefer/{prefer,index}.cp`、`doc/lib/http_prefer.md`、`cap/test/test_http_prefer_production_deep.cp`），实现 `respond-async`/`return`/`handling`/`wait` 首现优先解析与规范化输出。

## [2026-11-22] - 新建 `cap:mime_sniff` WHATWG MIME Sniffing 魔数嗅探引擎（图像/音视频/归档可执行/HTML-XML-JSON/UTF-8 文本与二进制判定）（5/5 PASS，自动动态挂载 `cap.mime_sniff`）
新增 `cap:mime_sniff`（`lib/mime_sniff/{sniff,index}.cp`、`doc/lib/mime_sniff.md`、`cap/test/test_mime_sniff_production_deep.cp`），完整实现 WHATWG 魔数嗅探与前 512B 文本/二进制检测。

## [2026-11-22] - 新建 `cap:ansi_strip` 零分配 ANSI/VT100 转义序列（CSI/OSC8 超链接/DCS/2 字节 ESC）剥离与可见 UTF-8 字符计数（5/5 PASS，自动动态挂载 `cap.ansi_strip`）
新增 `cap:ansi_strip`（`lib/ansi_strip/{strip,index}.cp`、`doc/lib/ansi_strip.md`、`cap/test/test_ansi_strip_production_deep.cp`），完整覆盖 CSI、OSC 8（BEL/ST）、字符集切换与 C1 CSI。

## [2026-11-22] - 新建 `cap:shell_quote` POSIX Shell 安全单引号转义、参数拼接与支持单双引号/反斜杠/相邻拼接的命令行拆分器（5/5 PASS，自动动态挂载 `cap.shell_quote`）
新增 `cap:shell_quote`（`lib/shell_quote/{quote,index}.cp`、`doc/lib/shell_quote.md`、`cap/test/test_shell_quote_production_deep.cp`），实现零分配 POSIX 引号转义与命令行参数迭代解析。

## [2026-11-22] - 新建 `cap:jwt_claims` RFC 7519 JWT 分段器、Base64URL 解码器、Header/Payload 声明提取与 exp/nbf/iss/sub/aud 校验器（5/5 PASS，自动动态挂载 `cap.jwt_claims`）
新增 `cap:jwt_claims`（`lib/jwt_claims/{jwt,index}.cp`、`doc/lib/jwt_claims.md`、`cap/test/test_jwt_claims_production_deep.cp`），实现零分配 JWT 拆段、Base64URL 解码、`alg=none` 检测与时间窗/身份校验。

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
