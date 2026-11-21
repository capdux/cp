# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:proc_net_route` Linux /proc/net/route 内核路由表零分配解析器、小端十六进制 IPv4 解码与最小 metric 默认网关查找器（5/5 PASS，自动动态挂载 `cap.proc_net_route`）
新增 `cap:proc_net_route`（`lib/proc_net_route/{route,index}.cp`、`doc/lib/proc_net_route.md`、`cap/test/test_proc_net_route_production_deep.cp`），实现 11 列路由表迭代、前缀长度计算与默认网关查找。

## [2026-11-22] - 新建 `cap:dns_record_type` IANA DNS 记录类型（18 种 RRType）、Class、RCODE 编解码与 SRV/MX 文本记录解析器（5/5 PASS，自动动态挂载 `cap.dns_record_type`）
新增 `cap:dns_record_type`（`lib/dns_record_type/{rr_type,index}.cp`、`doc/lib/dns_record_type.md`、`cap/test/test_dns_record_type_production_deep.cp`），覆盖 `A`/`AAAA`/`CNAME`/`MX`/`TXT`/`SRV`/`HTTPS`/`SVCB`/`CAA` 等 18 种 RRType、RCODE 及 RFC 7505 Null MX。

## [2026-11-22] - 新建 `cap:npmrc_config` .npmrc 配置文件零分配键值解析器、布尔配置查询与 @scope:registry 仓库路由解析器（5/5 PASS，自动动态挂载 `cap.npmrc_config`）
新增 `cap:npmrc_config`（`lib/npmrc_config/{npmrc,index}.cp`、`doc/lib/npmrc_config.md`、`cap/test/test_npmrc_config_production_deep.cp`），支持 BOM、单双引号剥离、作用域仓库回退与仓库级后缀配置匹配。

## [2026-11-22] - 新建 `cap:proxy_protocol_v2` HAProxy PROXY Protocol v2 二进制头零分配解析、IPv4/LOCAL 编码与 TLV 扩展迭代器（5/5 PASS，自动动态挂载 `cap.proxy_protocol_v2`）
新增 `cap:proxy_protocol_v2`（`lib/proxy_protocol_v2/{proxy_v2,index}.cp`、`doc/lib/proxy_protocol_v2.md`、`cap/test/test_proxy_protocol_v2_production_deep.cp`），完整实现 12 字节签名校验、IPv4/IPv6/UNIX 地址解码与 TLV 扩展迭代，正式突破 250 个纯 CP 标准库模块里程碑！

## [2026-11-22] - 新建 `cap:xdg_basedir` freedesktop.org XDG Base Directory Spec 路径零分配推导器（绝对路径校验、默认回退与防穿越 app_name）（5/5 PASS，自动动态挂载 `cap.xdg_basedir`）
新增 `cap:xdg_basedir`（`lib/xdg_basedir/{xdg,index}.cp`、`doc/lib/xdg_basedir.md`、`cap/test/test_xdg_basedir_production_deep.cp`），完整遵循 XDG 规范忽略相对路径并拒绝 `..` 路径穿越。

## [2026-11-22] - 新建 `cap:proxy_protocol_v1` HAProxy PROXY Protocol v1 文本头零分配解析与规范化格式化器（支持 TCP4/TCP6/UNKNOWN）（5/5 PASS，自动动态挂载 `cap.proxy_protocol_v1`）
新增 `cap:proxy_protocol_v1`（`lib/proxy_protocol_v1/{proxy_v1,index}.cp`、`doc/lib/proxy_protocol_v1.md`、`cap/test/test_proxy_protocol_v1_production_deep.cp`），严格校验 107 字节上限、IPv4/IPv6 地址与端口边界。

## [2026-11-22] - 新建 `cap:properties_parse` Java .properties 配置文件零分配流式解析器（支持 =/:/空白分隔、奇偶反斜杠续行与 \uXXXX 代理对反转义）（5/5 PASS，自动动态挂载 `cap.properties_parse`）
新增 `cap:properties_parse`（`lib/properties_parse/{properties,index}.cp`、`doc/lib/properties_parse.md`、`cap/test/test_properties_parse_production_deep.cp`），完整支持 `#`/`!` 注释、逻辑续行与 UTF-16 代理对转 UTF-8；同步将经验证的主/子智能体轻量协调与定期轮换铁律写入 `GEMINI.md` 第九章固化。

## [2026-11-22] - 新建 `cap:http_etag_gen` 零分配 HTTP 强/弱 ETag 生成器（FNV-1a 64-bit 内容哈希与 size/mtime 静态文件弱 ETag）及 RFC 9110 语法校验（5/5 PASS，自动动态挂载 `cap.http_etag_gen`）
新增 `cap:http_etag_gen`（`lib/http_etag_gen/{etag_gen,index}.cp`、`doc/lib/http_etag_gen.md`、`cap/test/test_http_etag_gen_production_deep.cp`），实现强/弱 ETag 生成与 RFC 9110 §8.8.3 严格语法校验。

## [2026-11-22] - 新建 `cap:ini_writer` INI / gitconfig 配置文件零分配安全序列化写入器（自动引号转义、子段输出与原子回滚）（5/5 PASS，自动动态挂载 `cap.ini_writer`）
新增 `cap:ini_writer`（`lib/ini_writer/{ini_writer,index}.cp`、`doc/lib/ini_writer.md`、`cap/test/test_ini_writer_production_deep.cp`），支持 `[section "subsection"]`、多行注释与缓冲不足原子回滚 `pos`。

## [2026-11-22] - 新建 `cap:http_cache_key` HTTP 缓存键零分配规范化构造器（大小写/默认端口归一化、utm_*/fbclid/gclid 追踪参数剥离与 Vary 维度追加）（5/5 PASS，自动动态挂载 `cap.http_cache_key`）
新增 `cap:http_cache_key`（`lib/http_cache_key/{cache_key,index}.cp`、`doc/lib/http_cache_key.md`、`cap/test/test_http_cache_key_production_deep.cp`），实现确定性缓存键生成、IPv6 默认端口剥离与防换行注入 Vary 维度追加。

## [2026-11-22] - 新建 `cap:shell_env_parse` POSIX Shell export KEY=VAL / .profile 风格环境变量赋值行零分配解析器（支持单双引号与 unset 清空）（5/5 PASS，自动动态挂载 `cap.shell_env_parse`）
新增 `cap:shell_env_parse`（`lib/shell_env_parse/{shell_env,index}.cp`、`doc/lib/shell_env_parse.md`、`cap/test/test_shell_env_parse_production_deep.cp`），支持 `export` 前缀、单双引号跨行/转义与 `unset KEY` 清空语义。

## [2026-11-22] - 新建 `cap:http_signature_input` RFC 9421 HTTP Message Signatures (Signature-Input / Signature) 零分配解析器与时间窗校验器（5/5 PASS，自动动态挂载 `cap.http_signature_input`）
新增 `cap:http_signature_input`（`lib/http_signature_input/{sig_input,index}.cp`、`doc/lib/http_signature_input.md`、`cap/test/test_http_signature_input_production_deep.cp`），实现多签名迭代、组件提取、时间窗 `leeway` 校验与 Base64 签名值查找。

## [2026-11-22] - 新建 `cap:http_digest_auth` RFC 7616 HTTP Digest Authentication (WWW-Authenticate / Authorization) 零分配解析与构造器（5/5 PASS，自动动态挂载 `cap.http_digest_auth`）
新增 `cap:http_digest_auth`（`lib/http_digest_auth/{digest_auth,index}.cp`、`doc/lib/http_digest_auth.md`、`cap/test/test_http_digest_auth_production_deep.cp`），支持 `realm`/`nonce`/`qop`/`nc`/`cnonce`/`userhash` 解析与规范化往返。

## [2026-11-22] - 新建 `cap:http_warning` RFC 7234 §5.5 HTTP Warning 头零分配解析器（引号逗号/转义/日期感知）与格式化器（5/5 PASS，自动动态挂载 `cap.http_warning`）
新增 `cap:http_warning`（`lib/http_warning/{warning,index}.cp`、`doc/lib/http_warning.md`、`cap/test/test_http_warning_production_deep.cp`），实现多条 Warning 迭代、过期警告码识别（110–113）与往返格式化。

## [2026-11-22] - 新建 `cap:dotenv_escape` .env 环境变量文件键名校验、防 $VAR 误展开安全引号序列化与反转义器（5/5 PASS，自动动态挂载 `cap.dotenv_escape`）
新增 `cap:dotenv_escape`（`lib/dotenv_escape/{dotenv_escape,index}.cp`、`doc/lib/dotenv_escape.md`、`cap/test/test_dotenv_escape_production_deep.cp`），支持双引号转义往返与 `$` 变量展开防护。

## [2026-11-22] - 新建 `cap:http_content_encoding` RFC 9110 §8.4 Content-Encoding 编码栈零分配解析、别名归一化与最大压缩层数安全防护（5/5 PASS，自动动态挂载 `cap.http_content_encoding`）
新增 `cap:http_content_encoding`（`lib/http_content_encoding/{content_encoding,index}.cp`、`doc/lib/http_content_encoding.md`、`cap/test/test_http_content_encoding_production_deep.cp`），兼容 `x-gzip`/`x-compress` 并限制异常多层压缩链。

## [2026-11-22] - 新建 `cap:number_format` 零分配人类可读数字格式化器（千分位分组、英文序数词、2..36 进制与定点小数）（5/5 PASS，自动动态挂载 `cap.number_format`）
新增 `cap:number_format`（`lib/number_format/{number_format,index}.cp`、`doc/lib/number_format.md`、`cap/test/test_number_format_production_deep.cp`），安全处理 `i64::MIN` 并支持千分位与定点小数格式化。

## [2026-11-22] - 新建 `cap:unicode_width` Unicode 东亚宽度 / 终端显示列宽（wcwidth）零分配计算器（组合符=0、ASCII=1、CJK/Emoji=2）（5/5 PASS，自动动态挂载 `cap.unicode_width`）
新增 `cap:unicode_width`（`lib/unicode_width/{width,index}.cp`、`doc/lib/unicode_width.md`、`cap/test/test_unicode_width_production_deep.cp`），实现终端列宽统计与不切裂 CJK 的列宽截断。

## [2026-11-22] - 新建 `cap:http_te` RFC 9110 / RFC 9112 Transfer-Encoding 与 TE 头零分配解析及异常报文边界校验器（5/5 PASS，自动动态挂载 `cap.http_te`）
新增 `cap:http_te`（`lib/http_te/{te,index}.cp`、`doc/lib/http_te.md`、`cap/test/test_http_te_production_deep.cp`），拦截重复 `chunked`、非末项 `chunked` 及 `Content-Length` 与 `Transfer-Encoding` 冲突报文。

## [2026-11-22] - 新建 `cap:http_freshness` RFC 9111 §4.2 HTTP 缓存新鲜度生存期、Age 修正与 RFC 5861 SWR/SIE 窗口求值器（5/5 PASS，自动动态挂载 `cap.http_freshness`）
新增 `cap:http_freshness`（`lib/http_freshness/{freshness,index}.cp`、`doc/lib/http_freshness.md`、`cap/test/test_http_freshness_production_deep.cp`），完整实现 `s-maxage`/`max-age`/`Expires`/10% `Last-Modified` 启发式及 SWR/SIE 窗口判定。

## [2026-11-22] - 新建 `cap:http_via` RFC 9110 §7.6.3 Via 代理链头与 Max-Forwards 零分配解析、环路检测与追加构造器（5/5 PASS，自动动态挂载 `cap.http_via`）
新增 `cap:http_via`（`lib/http_via/{via,index}.cp`、`doc/lib/http_via.md`、`cap/test/test_http_via_production_deep.cp`），支持括号内逗号注释、跳数统计、大小写无关环路检测与 `Max-Forwards` 递减。

## [2026-11-22] - 新建 `cap:http_keep_alive` HTTP Keep-Alive 参数（timeout/max）零分配解析格式化器与 RFC 9112 §9.3 持久连接判定器（5/5 PASS，自动动态挂载 `cap.http_keep_alive`）
新增 `cap:http_keep_alive`（`lib/http_keep_alive/{keep_alive,index}.cp`、`doc/lib/http_keep_alive.md`、`cap/test/test_http_keep_alive_production_deep.cp`），实现 `Keep-Alive` 编解码与 HTTP/1.0 vs HTTP/1.1 持久连接决策。

## [2026-11-22] - 新建 `cap:http_clear_site_data` W3C Clear-Site-Data 响应头零分配解析与规范化构造器（cache/cookies/storage/executionContexts/clientHints/*）（5/5 PASS，自动动态挂载 `cap.http_clear_site_data`）
新增 `cap:http_clear_site_data`（`lib/http_clear_site_data/{clear_site_data,index}.cp`、`doc/lib/http_clear_site_data.md`、`cap/test/test_http_clear_site_data_production_deep.cp`），遵循 W3C 大小写敏感引号指令规则与 `"*"` 折叠。

## [2026-11-22] - 新建 `cap:csv_escape` RFC 4180 CSV / TSV 字段转义器、单行构造器、严格引号反转义器与电子表格公式前缀风险检测（5/5 PASS，自动动态挂载 `cap.csv_escape`）
新增 `cap:csv_escape`（`lib/csv_escape/{csv_escape,index}.cp`、`doc/lib/csv_escape.md`、`cap/test/test_csv_escape_production_deep.cp`），支持自定义分隔符、CRLF/LF 行拼接与严格引号校验。

## [2026-11-22] - 新建 `cap:makefile_parse` GNU Makefile 目标、依赖、.PHONY 伪目标与变量赋值（=/ :=/ ?=/ +=）零分配解析器（5/5 PASS，自动动态挂载 `cap.makefile_parse`）
新增 `cap:makefile_parse`（`lib/makefile_parse/{makefile,index}.cp`、`doc/lib/makefile_parse.md`、`cap/test/test_makefile_parse_production_deep.cp`），支持双冒号规则、内联 `;` 配方剥离与 `export`/`override` 变量。

## [2026-11-22] - 新建 `cap:socket_addr` 零分配 host:port / [ipv6]:port 套接字地址解析器（支持严格远端模式与 :8080/:0 服务端绑定模式）（5/5 PASS，自动动态挂载 `cap.socket_addr`）
新增 `cap:socket_addr`（`lib/socket_addr/{socket_addr,index}.cp`、`doc/lib/socket_addr.md`、`cap/test/test_socket_addr_production_deep.cp`），实现 IPv4/IPv6/主机名分类、端口校验与规范化输出。

## [2026-11-22] - 新建 `cap:http_client_hints` W3C Sec-CH-UA / Sec-CH-UA-Mobile / Sec-CH-UA-Platform 客户端提示零分配解析器与 GREASE 识别（5/5 PASS，自动动态挂载 `cap.http_client_hints`）
新增 `cap:http_client_hints`（`lib/http_client_hints/{client_hints,index}.cp`、`doc/lib/http_client_hints.md`、`cap/test/test_http_client_hints_production_deep.cp`），支持品牌版本迭代、GREASE 占位过滤与移动/平台提示解析。

## [2026-11-22] - 新建 `cap:yaml_escape` YAML 1.2 标量引号安全判定、单/双引号转义与 Unicode（\xHH/\uXXXX/\UXXXXXXXX）反转义器（5/5 PASS，自动动态挂载 `cap.yaml_escape`）
新增 `cap:yaml_escape`（`lib/yaml_escape/{yaml_escape,index}.cp`、`doc/lib/yaml_escape.md`、`cap/test/test_yaml_escape_production_deep.cp`），补全 JSON/TOML/XML/HTML/YAML 五大格式转义矩阵。

## [2026-11-22] - 新建 `cap:git_diff_stat` Git Unified Diff (diff --git) 零分配文件变更统计提取器（增删行数、重命名、新增/删除与二进制识别）（5/5 PASS，自动动态挂载 `cap.git_diff_stat`）
新增 `cap:git_diff_stat`（`lib/git_diff_stat/{diff_stat,index}.cp`、`doc/lib/git_diff_stat.md`、`cap/test/test_git_diff_stat_production_deep.cp`），仅在 hunk 内统计 `+`/`-` 行并排除 `+++`/`---` 文件头。

## [2026-11-22] - 新建 `cap:http_sec_fetch` W3C Fetch Metadata（Sec-Fetch-Site/Mode/Dest/User）零分配解析与资源隔离防护策略求值器（5/5 PASS，自动动态挂载 `cap.http_sec_fetch`）
新增 `cap:http_sec_fetch`（`lib/http_sec_fetch/{sec_fetch,index}.cp`、`doc/lib/http_sec_fetch.md`、`cap/test/test_http_sec_fetch_production_deep.cp`），实现 W3C Resource Isolation Policy 与同源 API 守卫。

## [2026-11-22] - 新建 `cap:charset_label` WHATWG Encoding Standard 字符集标签规范化器（14 种编码别名映射与 UTF-8/UTF-16 BOM 嗅探）（5/5 PASS，自动动态挂载 `cap.charset_label`）
新增 `cap:charset_label`（`lib/charset_label/{charset,index}.cp`、`doc/lib/charset_label.md`、`cap/test/test_charset_label_production_deep.cp`），严格遵循 WHATWG Encoding §4.2 映射全部主流别名。

## [2026-11-22] - 新建 `cap:toml_escape` TOML 1.0 裸键校验、基本字符串字面量转义与严格 Unicode 标量值（\uXXXX/\UXXXXXXXX）反转义器（5/5 PASS，自动动态挂载 `cap.toml_escape`）
新增 `cap:toml_escape`（`lib/toml_escape/{toml_escape,index}.cp`、`doc/lib/toml_escape.md`、`cap/test/test_toml_escape_production_deep.cp`），完整支持 TOML 1.0 键与基本字符串转义/还原并拒绝代理项标量。

## [2026-11-22] - 新建 `cap:git_url_parse` Git 远程地址零分配解析器与协议互转工具（支持 SCP git@host:org/repo.git、ssh://、https://、git://、file://）（5/5 PASS，自动动态挂载 `cap.git_url_parse`）
新增 `cap:git_url_parse`（`lib/git_url_parse/{git_url,index}.cp`、`doc/lib/git_url_parse.md`、`cap/test/test_git_url_parse_production_deep.cp`），实现多协议解析、`owner`/`repo` 提取及 `formatHttps`/`formatScp` 互转。

## [2026-11-22] - 新建 `cap:mac_addr` IEEE 802 MAC-48 / EUI-48 硬件地址零分配解析器（4 种格式）、位分类器与 RFC 4291 EUI-64 派生器（5/5 PASS，自动动态挂载 `cap.mac_addr`）
新增 `cap:mac_addr`（`lib/mac_addr/{mac,index}.cp`、`doc/lib/mac_addr.md`、`cap/test/test_mac_addr_production_deep.cp`），支持冒号/连字符/Cisco 点分/裸十六进制、单播/组播/LAA 分类与 EUI-64 生成。

## [2026-11-22] - 新建 `cap:mime_headers` RFC 5322 / MIME 头块零分配解析器（头体分离、obs-fold 折行展开、大小写不敏感查询与重复头计数）（5/5 PASS，自动动态挂载 `cap.mime_headers`）
新增 `cap:mime_headers`（`lib/mime_headers/{headers,index}.cp`、`doc/lib/mime_headers.md`、`cap/test/test_mime_headers_production_deep.cp`），遵循 RFC 5322 §2.2.3 将续行折叠为单空格并兼容 CRLF/LF。

## [2026-11-22] - 新建 `cap:brace_expand` 零分配 Bash 风格花括号展开迭代器（逗号列表、步进/补零整数序列与字母序列）（5/5 PASS，自动动态挂载 `cap.brace_expand`）
新增 `cap:brace_expand`（`lib/brace_expand/{brace_expand,index}.cp`、`doc/lib/brace_expand.md`、`cap/test/test_brace_expand_production_deep.cp`），支持 `{a,b,c}`、`{01..05}`、`{6..2..2}` 与 `{a..z}` 零堆分配流式展开。

## [2026-11-22] - 新建 `cap:ssh_config` OpenSSH ~/.ssh/config 零分配解析器与主机参数匹配引擎（首现优先、* 与 ! 否定模式）（5/5 PASS，自动动态挂载 `cap.ssh_config`）
新增 `cap:ssh_config`（`lib/ssh_config/{ssh_config,index}.cp`、`doc/lib/ssh_config.md`、`cap/test/test_ssh_config_production_deep.cp`），严格遵循 `ssh_config(5)` 首现优先生效、`!pattern` 一票否决与 `Host *` 全局回退。

## [2026-11-22] - 新建 `cap:docker_image_ref` OCI / Docker 镜像引用零分配解析器（[registry[:port]/]repository[:tag][@digest] 与 sha256/sha512 摘要校验）（5/5 PASS，自动动态挂载 `cap.docker_image_ref`）
新增 `cap:docker_image_ref`（`lib/docker_image_ref/{image_ref,index}.cp`、`doc/lib/docker_image_ref.md`、`cap/test/test_docker_image_ref_production_deep.cp`），精准区分 `registry:port` 与 `:tag` 并推导默认 `docker.io`/`latest`。

## [2026-11-22] - 新建 `cap:rfc2047_word` RFC 2047 MIME Encoded-Word (=?charset?B|Q?text?=) 国际化头零分配编解码器（含相邻空白折叠）（5/5 PASS，自动动态挂载 `cap.rfc2047_word`）
新增 `cap:rfc2047_word`（`lib/rfc2047_word/{encoded_word,index}.cp`、`doc/lib/rfc2047_word.md`、`cap/test/test_rfc2047_word_production_deep.cp`），支持 B/Q 编码解码、相邻词空白折叠与 UTF-8 Base64 编码往返。

## [2026-11-22] - 新建 `cap:mail_address` RFC 5322 电子邮件地址校验、"Display Name" <local@domain> 解析与引号逗号感知收件人列表迭代器（5/5 PASS，自动动态挂载 `cap.mail_address`）
新增 `cap:mail_address`（`lib/mail_address/{mailbox,index}.cp`、`doc/lib/mail_address.md`、`cap/test/test_mail_address_production_deep.cp`），实现 dot-atom/DNS 校验与多收件人解析。

## [2026-11-22] - 新建 `cap:systemd_unit` Linux systemd 单元文件（.service/.timer/.socket）零分配解析器（支持重复键、反斜杠续行与空值清空）（5/5 PASS，自动动态挂载 `cap.systemd_unit`）
新增 `cap:systemd_unit`（`lib/systemd_unit/{unit,index}.cp`、`doc/lib/systemd_unit.md`、`cap/test/test_systemd_unit_production_deep.cp`），遵循 `systemd.syntax(7)` 续行、覆盖与空赋值清空语义。

## [2026-11-22] - 新建 `cap:os_release` freedesktop.org /etc/os-release 零分配解析器（Shell 引号反转义与 ID/ID_LIKE 发行版家族匹配）（5/5 PASS，自动动态挂载 `cap.os_release`）
新增 `cap:os_release`（`lib/os_release/{os_release,index}.cp`、`doc/lib/os_release.md`、`cap/test/test_os_release_production_deep.cp`），支持单/双引号转义、同名后置覆盖与 `isOsLike` 家族判定。

## [2026-11-22] - 新建 `cap:fstab_parse` POSIX /etc/fstab 与 /proc/mounts 零分配 6 字段解析器、八进制路径反转义与最长前缀挂载点查找（5/5 PASS，自动动态挂载 `cap.fstab_parse`）
新增 `cap:fstab_parse`（`lib/fstab_parse/{fstab,index}.cp`、`doc/lib/fstab_parse.md`、`cap/test/test_fstab_parse_production_deep.cp`），支持 `\040` 等八进制转义还原、选项键值查询与 `/` 边界最长前缀挂载点匹配。

## [2026-11-22] - 新建 `cap:resolv_conf` POSIX /etc/resolv.conf 零分配解析器（nameserver/search/domain 末现优先与 glibc 上限夹紧 options）（5/5 PASS，自动动态挂载 `cap.resolv_conf`）
新增 `cap:resolv_conf`（`lib/resolv_conf/{resolv,index}.cp`、`doc/lib/resolv_conf.md`、`cap/test/test_resolv_conf_production_deep.cp`），支持 `#`/`;` 注释、IPv4/IPv6 nameserver 迭代与 `ndots`/`timeout`/`attempts` 夹紧。

## [2026-11-22] - 新建 `cap:sql_splitter` 多语句 SQL 脚本零分配分号切分器（跳过引号/注释/$$ 块）与 CTE 穿透语句类型识别器（5/5 PASS，自动动态挂载 `cap.sql_splitter`）
新增 `cap:sql_splitter`（`lib/sql_splitter/{splitter,index}.cp`、`doc/lib/sql_splitter.md`、`cap/test/test_sql_splitter_production_deep.cp`），实现顶层 `;` 切分、`WITH` CTE 穿透分类与 `isReadOnlySql` 判定。

## [2026-11-22] - 新建 `cap:proc_loadavg` Linux /proc/loadavg 与 /proc/uptime 纯整数零分配解析器（百分位负载与毫秒运行时间）（5/5 PASS，自动动态挂载 `cap.proc_loadavg`）
新增 `cap:proc_loadavg`（`lib/proc_loadavg/{loadavg,index}.cp`、`doc/lib/proc_loadavg.md`、`cap/test/test_proc_loadavg_production_deep.cp`），纯整数解析 1/5/15 分钟负载、进程数与系统运行毫秒。

## [2026-11-22] - 新建 `cap:http_ratelimit_header` IETF Draft RateLimit / RateLimit-Policy 限流响应头零分配解析与规范化构造器（5/5 PASS，自动动态挂载 `cap.http_ratelimit_header`）
新增 `cap:http_ratelimit_header`（`lib/http_ratelimit_header/{ratelimit_header,index}.cp`、`doc/lib/http_ratelimit_header.md`、`cap/test/test_http_ratelimit_header_production_deep.cp`），兼容结构化 `"default";r=50;t=30` 与字典 `limit=100, remaining=50, reset=30` 两种格式。

## [2026-11-22] - 新建 `cap:proc_meminfo` Linux /proc/meminfo 零分配流式解析器与内存/Swap 使用率计算器（kB 自动转精确字节）（5/5 PASS，自动动态挂载 `cap.proc_meminfo`）
新增 `cap:proc_meminfo`（`lib/proc_meminfo/{meminfo,index}.cp`、`doc/lib/proc_meminfo.md`、`cap/test/test_proc_meminfo_production_deep.cp`），支持 `MemAvailable` 与旧内核 `MemFree+Buffers+Cached` 回退。

## [2026-11-22] - 新建 `cap:sql_params` SQL 占位符词法安全扫描器与方言重写器（跳过字符串/注释/$$ 块，支持 ?→$N 重写与 :name 提取）（5/5 PASS，自动动态挂载 `cap.sql_params`）
新增 `cap:sql_params`（`lib/sql_params/{params,index}.cp`、`doc/lib/sql_params.md`、`cap/test/test_sql_params_production_deep.cp`），安全跳过引号/注释/Dollar-quoted 块实现 `?` 到 `$1` 重写与 `:name` 迭代。

## [2026-11-22] - 新建 `cap:proc_stat` Linux /proc/<pid>/stat 零分配解析器（末右括号定位，安全兼容含空格/括号的 comm 进程名）（5/5 PASS，自动动态挂载 `cap.proc_stat`）
新增 `cap:proc_stat`（`lib/proc_stat/{proc_stat,index}.cp`、`doc/lib/proc_stat.md`、`cap/test/test_proc_stat_production_deep.cp`），提取 PID/PPID/状态/CPU 滴答/线程数/VSize/RSS 并防进程名注入。

## [2026-11-22] - 新建 `cap:http_upgrade` RFC 9110 §7.8 Upgrade / Connection 协议升级与逐跳头零分配解析器及 WebSocket 握手判定器（5/5 PASS，自动动态挂载 `cap.http_upgrade`）
新增 `cap:http_upgrade`（`lib/http_upgrade/{upgrade,index}.cp`、`doc/lib/http_upgrade.md`、`cap/test/test_http_upgrade_production_deep.cp`），覆盖 8 类标准及动态逐跳头与 WebSocket 13 握手校验。

## [2026-11-22] - 新建 `cap:json_minify` 零分配 JSON / JSONC（去 // 与 /* */ 注释、去尾随逗号、去 BOM 与空白）压缩器（5/5 PASS，自动动态挂载 `cap.json_minify`）
新增 `cap:json_minify`（`lib/json_minify/{minify,index}.cp`、`doc/lib/json_minify.md`、`cap/test/test_json_minify_production_deep.cp`），完整保留字符串字面量与转义并拦截未闭合字符串/块注释。

## [2026-11-22] - 新建 `cap:http_host_header` RFC 9110 §7.2 HTTP Host / :authority 请求头安全解析、校验、回环识别与规范化格式化器（5/5 PASS，自动动态挂载 `cap.http_host_header`）
新增 `cap:http_host_header`（`lib/http_host_header/{host_header,index}.cp`、`doc/lib/http_host_header.md`、`cap/test/test_http_host_header_production_deep.cp`），严格拦截 userinfo/路径/非法端口与畸形 IPv6 方括号。

## [2026-11-22] - 新建 `cap:rfc7807_problem` RFC 9457 / RFC 7807 HTTP Problem Details (application/problem+json) 零分配构造与转义序列化器（5/5 PASS，自动动态挂载 `cap.rfc7807_problem`）
新增 `cap:rfc7807_problem`（`lib/rfc7807_problem/{problem,index}.cp`、`doc/lib/rfc7807_problem.md`、`cap/test/test_rfc7807_problem_production_deep.cp`），支持标准状态码标题推导与 RFC 8259 安全 JSON 输出。

## [2026-11-22] - 新建 `cap:http_cookie_match` RFC 6265 §5.1.3/§5.1.4 Cookie 域名匹配、路径匹配、默认路径计算与 __Secure-/__Host- 发送策略判定器（5/5 PASS，自动动态挂载 `cap.http_cookie_match`）
新增 `cap:http_cookie_match`（`lib/http_cookie_match/{cookie_match,index}.cp`、`doc/lib/http_cookie_match.md`、`cap/test/test_http_cookie_match_production_deep.cp`），实现严格点边界与 `/` 边界匹配及 RFC 6265bis 前缀约束校验。
