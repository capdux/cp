# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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
