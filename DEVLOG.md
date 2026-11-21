## 2026-11-22: 新建 `cap:charset_label` WHATWG Encoding Standard 字符集标签规范化器（14 种编码别名映射与 UTF-8/UTF-16 BOM 嗅探）（5/5 PASS，文档直写 `doc/lib/charset_label.md`）
Subagent 2 吸收 `cap:charset_label`，零堆分配提供规范 IANA 名称、ASCII 超集判定与 BOM 嗅探。

## 2026-11-22: 新建 `cap:toml_escape` TOML 1.0 裸键校验、基本字符串字面量转义与严格 Unicode 标量值（\uXXXX/\UXXXXXXXX）反转义器（5/5 PASS，文档直写 `doc/lib/toml_escape.md`）
主智能体吸收 `cap:toml_escape`，与 `cap:json_escape`/`cap:xml_escape`/`cap:html_escape` 形成统一序列化转义工具链。

## 2026-11-22: 新建 `cap:git_url_parse` Git 远程地址零分配解析器与协议互转工具（支持 SCP git@host:org/repo.git、ssh://、https://、git://、file://）（5/5 PASS，文档直写 `doc/lib/git_url_parse.md`）
Subagent 1 吸收 `cap:git_url_parse`，规范化互转时防范 HTTPS token 泄露为 SCP 用户名。

## 2026-11-22: 新建 `cap:mac_addr` IEEE 802 MAC-48 / EUI-48 硬件地址零分配解析器（4 种格式）、位分类器与 RFC 4291 EUI-64 派生器（5/5 PASS，文档直写 `doc/lib/mac_addr.md`）
主智能体吸收 `cap:mac_addr`，拒绝混用分隔符并实现规范化冒号/连字符输出。

## 2026-11-22: 新建 `cap:mime_headers` RFC 5322 / MIME 头块零分配解析器（头体分离、obs-fold 折行展开、大小写不敏感查询与重复头计数）（5/5 PASS，文档直写 `doc/lib/mime_headers.md`）
Subagent 2 吸收 `cap:mime_headers`，不匹配头不消耗缓冲并支持多条 `Received:` 零缓冲计数。

## 2026-11-22: 新建 `cap:brace_expand` 零分配 Bash 风格花括号展开迭代器（逗号列表、步进/补零整数序列与字母序列）（5/5 PASS，文档直写 `doc/lib/brace_expand.md`）
主智能体吸收 `cap:brace_expand`，无有效花括号组时原样产出一次。

## 2026-11-22: 新建 `cap:ssh_config` OpenSSH ~/.ssh/config 零分配解析器与主机参数匹配引擎（首现优先、* 与 ! 否定模式）（5/5 PASS，文档直写 `doc/lib/ssh_config.md`）
Subagent 1 吸收 `cap:ssh_config`，支持 `Key Value`/`Key=Value` 与自定义配置项首次生效查询。

## 2026-11-22: 新建 `cap:docker_image_ref` OCI / Docker 镜像引用零分配解析器（[registry[:port]/]repository[:tag][@digest] 与 sha256/sha512 摘要校验）（5/5 PASS，文档直写 `doc/lib/docker_image_ref.md`）
主智能体吸收 `cap:docker_image_ref`，与 `cap:dockerfile_parse` 协同服务容器工具链。

## 2026-11-22: 新建 `cap:rfc2047_word` RFC 2047 MIME Encoded-Word (=?charset?B|Q?text?=) 国际化头零分配编解码器（含相邻空白折叠）（5/5 PASS，文档直写 `doc/lib/rfc2047_word.md`）
Subagent 2 吸收 `cap:rfc2047_word`，严格遵循 RFC 2047 §6.2 空白折叠规则。

## 2026-11-22: 新建 `cap:mail_address` RFC 5322 电子邮件地址校验、"Display Name" <local@domain> 解析与引号逗号感知收件人列表迭代器（5/5 PASS，文档直写 `doc/lib/mail_address.md`）
主智能体吸收 `cap:mail_address`，零堆分配安全处理引号内逗号与转义字符。

## 2026-11-22: 新建 `cap:systemd_unit` Linux systemd 单元文件（.service/.timer/.socket）零分配解析器（支持重复键、反斜杠续行与空值清空）（5/5 PASS，文档直写 `doc/lib/systemd_unit.md`）
Subagent 1 吸收 `cap:systemd_unit`，支持多条 `Environment`/`ExecStartPre` 迭代与统计。

## 2026-11-22: 新建 `cap:os_release` freedesktop.org /etc/os-release 零分配解析器（Shell 引号反转义与 ID/ID_LIKE 发行版家族匹配）（5/5 PASS，文档直写 `doc/lib/os_release.md`）
主智能体吸收 `cap:os_release`，补齐 Linux `/etc` 与 `/proc` 系统信息解析矩阵。

## 2026-11-22: 新建 `cap:fstab_parse` POSIX /etc/fstab 与 /proc/mounts 零分配 6 字段解析器、八进制路径反转义与最长前缀挂载点查找（5/5 PASS，文档直写 `doc/lib/fstab_parse.md`）
Subagent 2 吸收 `cap:fstab_parse`，防 `/database` 误匹配 `/data` 且支持 `ro`/`rw` 后现优先判定。

## 2026-11-22: 新建 `cap:resolv_conf` POSIX /etc/resolv.conf 零分配解析器（nameserver/search/domain 末现优先与 glibc 上限夹紧 options）（5/5 PASS，文档直写 `doc/lib/resolv_conf.md`）
主智能体吸收 `cap:resolv_conf`，完整遵循 `resolv.conf(5)` 与 glibc 解析语义。

## 2026-11-22: 新建 `cap:sql_splitter` 多语句 SQL 脚本零分配分号切分器（跳过引号/注释/$$ 块）与 CTE 穿透语句类型识别器（5/5 PASS，文档直写 `doc/lib/sql_splitter.md`）
Subagent 1 吸收 `cap:sql_splitter`，与 `cap:sql_params`/`cap:sql_escape` 组成完整 SQL 预处理工具链。

## 2026-11-22: 新建 `cap:proc_loadavg` Linux /proc/loadavg 与 /proc/uptime 纯整数零分配解析器（百分位负载与毫秒运行时间）（5/5 PASS，文档直写 `doc/lib/proc_loadavg.md`）
主智能体吸收 `cap:proc_loadavg`，避免浮点解析误差并提供 `isOverloaded(num_cpus)` 过载判定。

## 2026-11-22: 新建 `cap:http_ratelimit_header` IETF Draft RateLimit / RateLimit-Policy 限流响应头零分配解析与规范化构造器（5/5 PASS，文档直写 `doc/lib/http_ratelimit_header.md`）
Subagent 2 吸收 `cap:http_ratelimit_header`，强制校验窗口 `w > 0` 并支持命名策略格式化。

## 2026-11-22: 新建 `cap:proc_meminfo` Linux /proc/meminfo 零分配流式解析器与内存/Swap 使用率计算器（kB 自动转精确字节）（5/5 PASS，文档直写 `doc/lib/proc_meminfo.md`）
主智能体吸收 `cap:proc_meminfo`，纯整数 `u128` 计算使用百分比并防 `kB * 1024` 溢出。

## 2026-11-22: 新建 `cap:sql_params` SQL 占位符词法安全扫描器与方言重写器（跳过字符串/注释/$$ 块，支持 ?→$N 重写与 :name 提取）（5/5 PASS，文档直写 `doc/lib/sql_params.md`）
Subagent 1 吸收 `cap:sql_params`，自动忽略 PostgreSQL `::type` 双冒号类型转换。

## 2026-11-22: 新建 `cap:proc_stat` Linux /proc/<pid>/stat 零分配解析器（末右括号定位，安全兼容含空格/括号的 comm 进程名）（5/5 PASS，文档直写 `doc/lib/proc_stat.md`）
主智能体吸收 `cap:proc_stat`，按内核规范以最后一个 `)` 切分 `comm`，彻底避免含空格或括号进程名解析错位。

## 2026-11-22: 新建 `cap:http_upgrade` RFC 9110 §7.8 Upgrade / Connection 协议升级与逐跳头零分配解析器及 WebSocket 握手判定器（5/5 PASS，文档直写 `doc/lib/http_upgrade.md`）
Subagent 2 吸收 `cap:http_upgrade`，零堆分配解析多协议升级列表与 `Connection` 令牌。

## 2026-11-22: 新建 `cap:json_minify` 零分配 JSON / JSONC（去 // 与 /* */ 注释、去尾随逗号、去 BOM 与空白）压缩器（5/5 PASS，文档直写 `doc/lib/json_minify.md`）
主智能体吸收 `cap:json_minify`，适用于 `tsconfig.json`/`cap.json` 等 JSONC 配置预处理。

## 2026-11-22: 新建 `cap:http_host_header` RFC 9110 §7.2 HTTP Host / :authority 请求头安全解析、校验、回环识别与规范化格式化器（5/5 PASS，文档直写 `doc/lib/http_host_header.md`）
Subagent 1 吸收 `cap:http_host_header`，支持 `unbracketedHost`/`effectivePort`/`formatCanonical`。

## 2026-11-22: 新建 `cap:rfc7807_problem` RFC 9457 / RFC 7807 HTTP Problem Details (application/problem+json) 零分配构造与转义序列化器（5/5 PASS，文档直写 `doc/lib/rfc7807_problem.md`）
主智能体吸收 `cap:rfc7807_problem`，零堆分配生成符合 RFC 9457 的标准 API 错误响应体。

## 2026-11-22: 新建 `cap:http_cookie_match` RFC 6265 §5.1.3/§5.1.4 Cookie 域名匹配、路径匹配、默认路径计算与 __Secure-/__Host- 发送策略判定器（5/5 PASS，文档直写 `doc/lib/http_cookie_match.md`）
Subagent 2 吸收 `cap:http_cookie_match`，对 IPv4/IPv6 字面量强制仅精确匹配，与 `cap:http_cookie` 形成完整客户端 Cookie 引擎。
