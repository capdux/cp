## 2026-11-22: 新建 `cap:http_allow` RFC 9110 §10.2.1 HTTP `Allow` 方法集位掩码解析、格式化与安全/幂等判定器（5/5 PASS，文档直写 `doc/lib/http_allow.md`）
- **设计要点**：严格区分大小写解析 RFC 9110 方法 token，使用 `u16` 位掩码零分配表示标准方法集合并支持 `PROPFIND`/`MKCOL` 等扩展方法。

## 2026-11-22: 新建 `cap:semver_satisfies` NPM / Bun 风格 SemVer 2.0.0 版本解析、比较与范围表达式零分配匹配器（5/5 PASS，文档直写 `doc/lib/semver_satisfies.md`）
- **设计要点**：全状态零堆分配单遍评估 `||` 与空白逻辑与；严格实现 SemVer 2.0.0 预发布优先级与同 `[major, minor, patch]` 预发布比较器放行规则。

## 2026-11-22: 新建 `cap:sysctl_conf` Linux `/etc/sysctl.conf` / `sysctl.d/*.conf` 内核参数配置零分配解析与键等价查询器（5/5 PASS，文档直写 `doc/lib/sysctl_conf.md`）
- **设计要点**：`SysctlIter` 零分配跳过注释与非法键行；`sysctlKeysEqual` 无缓冲等价比较 `.` 与 `/`；`getSysctlValue` 严格遵循后定义覆盖前定义语义。

## 2026-11-22: 新建 `cap:http_content_range` RFC 9110 §14.4 HTTP `Content-Range` 断点续传响应头零分配解析与格式化器（5/5 PASS，文档直写 `doc/lib/http_content_range.md`）
- **设计要点**：`parseContentRange` 零分配解析并校验 `start <= end < complete_length`；`sliceLength` 防溢出；`formatContentRange` 同步拦截非法语义状态与缓冲不足。

## 2026-11-22: 新建 `cap:cgroup_v2_limits` Linux cgroup v2 容器资源限额（`cpu.max`/`memory.max`/`pids.max`/`io.max`）零分配解析器（5/5 PASS，文档直写 `doc/lib/cgroup_v2_limits.md`）
- **设计要点**：`parseCpuMax` 严格拒绝 `period_us == 0` 并用 `u128` 计算 `effectiveMilliCpus` 消除溢出；`parseCgroupMax` 统一处理 `"max"` 与无符号整数并拦截负数与溢出。

## 2026-11-22: 新建 `cap:http_alt_svc` RFC 7838 HTTP `Alt-Svc` 备选服务 / HTTP/3 QUIC 发现头零分配解析与格式化器（5/5 PASS，文档直写 `doc/lib/http_alt_svc.md`）
- **设计要点**：`AltSvcIter` 零分配解析多条目并严格校验端口 `1..=65535`；`isHttp3()` 识别 `h3` 与 `h3-*` 协议标识；`formatAltSvcEntry` 防止缓冲溢出与非法字符注入。

## 2026-11-22: 新建 `cap:proc_self_status` Linux `/proc/<pid>/status` 进程状态、内存/线程/UID/GID 零分配键值解析器（5/5 PASS，文档直写 `doc/lib/proc_self_status.md`）
- **设计要点**：单遍扫描 `Key:\tValue` 兼容 `kB`/`MB`/`GB` 单位；`rssBytes()` 使用饱和乘法防止 `u64` 溢出；`findProcStatusField` 零拷贝提取任意扩展字段。

## 2026-11-22: 新建 `cap:ansi_link` OSC 8 终端超链接（ST/BEL）零分配编码、解析与剥离器（5/5 PASS，文档直写 `doc/lib/ansi_link.md`）
- **设计要点**：兼容 ST (`\x1b\\`) 与 BEL (`\x07`) 终止符及内嵌样式码，拒绝未闭合、嵌套开启与控制字符注入；提供 `osc8EncodedLen` 便于调用方精准预留缓冲。

## 2026-11-22: 新建 `cap:http_early_hints` RFC 8297 HTTP `103 Early Hints` / RFC 8288 `Link` 预加载头零分配解析与报文格式化器（5/5 PASS，文档直写 `doc/lib/http_early_hints.md`）
- **设计要点**：`EarlyHintIter` 零分配尖括号与引号感知解析多条目；`formatEarlyHintLink` 与 `formatEarlyHints103Block` 拦截 CRLF 注入并在缓冲区不足时返回 `null`。

## 2026-11-22: 新建 `cap:proc_uptime` Linux `/proc/uptime` 系统运行时间与全核空闲占比零分配整数解析与人类可读格式化器（5/5 PASS，文档直写 `doc/lib/proc_uptime.md`）
- **设计要点**：全整数毫秒解析小数位自动补零/截断，`idleRatioPermille` 使用 `u128` 消除溢出并钳位 `0..=1000`，严格拦截缺失/多余字段与溢出。

## 2026-11-22: 新建 `cap:http_expect` RFC 9110 §10.1.1 HTTP `Expect` 请求头解析与 `100-continue` / 417 协商判定器（5/5 PASS，文档直写 `doc/lib/http_expect.md`）
- **设计要点**：`ExpectIter` 零分配引号感知遍历；`evaluateExpectHeader` 严格遵循 RFC 9110 §10.1.1 规则处理未知期望、HTTP/1.0 回退与 `has_body` 条件。

## 2026-11-22: 新建 `cap:proc_mounts` Linux `/proc/mounts` 挂载表、八进制路径转义解码与最长边界前缀挂载点匹配器（5/5 PASS，文档直写 `doc/lib/proc_mounts.md`）
- **设计要点**：`unescapeMountPath` 解码 `\040`/`\011`/`\012`/`\134` 等内核八进制转义；`findMountForPath` 严格区分 `/var` 与 `/var2` 边界并支持叠层挂载后覆盖语义。

## 2026-11-22: 新建 `cap:dns_srv_record` RFC 2782 DNS SRV 服务发现记录文本/线缆头零分配解析与优先级权重选择器（5/5 PASS，文档直写 `doc/lib/dns_srv_record.md`）
- **设计要点**：`SrvRecord.parseText` 与 `parseWireHeader` 零分配解码，识别 `target == "."` 服务不可用语义；`selectSrvByRoll` 支持权重累积落点与全零权重均匀轮询。

## 2026-11-22: 新建 `cap:proc_cpuinfo` Linux `/proc/cpuinfo`（x86_64/ARM64）逻辑/物理核心与指令集标志零分配解析器（5/5 PASS，文档直写 `doc/lib/proc_cpuinfo.md`）
- **设计要点**：`ProcCpuInfoIter` 零分配按 `processor` 块迭代并跳过尾部非处理器元数据；`summarizeCpuInfo` 零分配按 `physical_id` 去重汇总物理核心；`hasCpuFlag` 精确 token 匹配防止 `avx`/`avx2` 子串误判。

## 2026-11-22: 新建 `cap:dns_caa_record` RFC 8659 DNS CAA 证书颁发机构授权记录文本/线缆格式零分配解析与签发策略判定器（5/5 PASS，文档直写 `doc/lib/dns_caa_record.md`）
- **设计要点**：实现 `CaaRecord.parseText` 与 `CaaRecord.parseWire` 双路径解析，`isCaAuthorized` 严格遵循 RFC 8659 未知 critical 拒签与通配符优先 `issuewild` 回退 `issue` 规则。

## 2026-11-22: 新建 `cap:proc_net_dev` Linux `/proc/net/dev` 网络接口收发字节/包/错误/丢包零分配解析器（5/5 PASS，文档直写 `doc/lib/proc_net_dev.md`）
- **设计要点**：跳过前两行表头，按 `:` 切分接口名与 16 列无符号整数，零分配切片迭代 `ProcNetDevIter`；提供 `findNetDev` 与 `sumNonLoopbackBytes`（自动排除 `lo`）。

## 2026-11-22: 新建 `cap:dns_txt_parse` DNS TXT 分号键值标签、DMARC (v=DMARC1) 与 SPF (v=spf1) 策略记录零分配解析器（5/5 PASS，文档直写 `doc/lib/dns_txt_parse.md`）
轮换后的 `HTTP RFC Absorber` 仅用 1 分 24 秒交付 `cap:dns_txt_parse`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:cron_preset` Vixie Cron @macro 预设（@hourly/@daily/@weekly/@monthly/@yearly/@reboot）零分配展开与 5 段表达式校验器（5/5 PASS，文档直写 `doc/lib/cron_preset.md`）
主智能体按“单回合 1 个短任务”平衡节奏完成 `cap:cron_preset` 并与子智能体交付的 `cap:dns_txt_parse` 合批提交。

## 2026-11-22: 新建 `cap:proc_net_route` Linux /proc/net/route 内核路由表零分配解析器、小端十六进制 IPv4 解码与最小 metric 默认网关查找器（5/5 PASS，文档直写 `doc/lib/proc_net_route.md`）
轮换后的 `Sys Config Absorber` 仅用 1 分 59 秒交付 `cap:proc_net_route`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:dns_record_type` IANA DNS 记录类型（18 种 RRType）、Class、RCODE 编解码与 SRV/MX 文本记录解析器（5/5 PASS，文档直写 `doc/lib/dns_record_type.md`）
轮换后的 `HTTP RFC Absorber` 仅用 2 分 39 秒交付 `cap:dns_record_type`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:npmrc_config` .npmrc 配置文件零分配键值解析器、布尔配置查询与 @scope:registry 仓库路由解析器（5/5 PASS，文档直写 `doc/lib/npmrc_config.md`）
轮换后的 `Sys Config Absorber` 仅用 1 分 19 秒交付 `cap:npmrc_config`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:proxy_protocol_v2` HAProxy PROXY Protocol v2 二进制头零分配解析、IPv4/LOCAL 编码与 TLV 扩展迭代器（5/5 PASS，文档直写 `doc/lib/proxy_protocol_v2.md`）
轮换后的 `HTTP RFC Absorber` 仅用 1 分 49 秒交付 `cap:proxy_protocol_v2`（5 组深度用例 100% PASS），与 `cap:proxy_protocol_v1` 组成完整代理协议双雄。

## 2026-11-22: 新建 `cap:xdg_basedir` freedesktop.org XDG Base Directory Spec 路径零分配推导器（绝对路径校验、默认回退与防穿越 app_name）（5/5 PASS，文档直写 `doc/lib/xdg_basedir.md`）
轮换后的 `Sys Config Absorber` 仅用 1 分 24 秒交付 `cap:xdg_basedir`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:proxy_protocol_v1` HAProxy PROXY Protocol v1 文本头零分配解析与规范化格式化器（支持 TCP4/TCP6/UNKNOWN）（5/5 PASS，文档直写 `doc/lib/proxy_protocol_v1.md`）
轮换后的 `HTTP RFC Absorber` 仅用 1 分 35 秒交付 `cap:proxy_protocol_v1`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:properties_parse` Java .properties 配置文件零分配流式解析器（支持 =/:/空白分隔、奇偶反斜杠续行与 \uXXXX 代理对反转义）（5/5 PASS，文档直写 `doc/lib/properties_parse.md`）
轮换后的 `Sys Config Absorber` 仅用 1 分 49 秒交付 `cap:properties_parse`（5 组深度用例 100% PASS）；连续 6 个模块验证新协作与轮换策略显著提速，已正式固化至 `GEMINI.md` 第九章实现自我进化。

## 2026-11-22: 新建 `cap:http_etag_gen` 零分配 HTTP 强/弱 ETag 生成器（FNV-1a 64-bit 内容哈希与 size/mtime 静态文件弱 ETag）及 RFC 9110 语法校验（5/5 PASS，文档直写 `doc/lib/http_etag_gen.md`）
轮换后的 `HTTP RFC Absorber` 仅用 1 分 16 秒交付 `cap:http_etag_gen`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:ini_writer` INI / gitconfig 配置文件零分配安全序列化写入器（自动引号转义、子段输出与原子回滚）（5/5 PASS，文档直写 `doc/lib/ini_writer.md`）
轮换后的 `Sys Config Absorber` 仅用 1 分 17 秒交付 `cap:ini_writer`（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:http_cache_key` HTTP 缓存键零分配规范化构造器（大小写/默认端口归一化、utm_*/fbclid/gclid 追踪参数剥离与 Vary 维度追加）（5/5 PASS，文档直写 `doc/lib/http_cache_key.md`）
轮换后的 `HTTP RFC Absorber` 仅用 1 分 39 秒完成 `cap:http_cache_key` 交付与全量验收（5 组深度用例 100% PASS）。

## 2026-11-22: 新建 `cap:shell_env_parse` POSIX Shell export KEY=VAL / .profile 风格环境变量赋值行零分配解析器（支持单双引号与 unset 清空）（5/5 PASS，文档直写 `doc/lib/shell_env_parse.md`）
轮换后的 `Sys Config Absorber` 仅用 2 分 52 秒交付 `cap:shell_env_parse`（5 组深度用例 100% PASS），双路轮换提速均已验证有效。

## 2026-11-22: 新建 `cap:http_signature_input` RFC 9421 HTTP Message Signatures (Signature-Input / Signature) 零分配解析器与时间窗校验器（5/5 PASS，文档直写 `doc/lib/http_signature_input.md`）
轮换后的轻上下文子智能体 `HTTP RFC Absorber` 仅用 2 分 19 秒交付 `cap:http_signature_input`（6 组深度用例 100% PASS），验证了定期轮换策略显著提速。

## 2026-11-22: 新建 `cap:http_digest_auth` RFC 7616 HTTP Digest Authentication (WWW-Authenticate / Authorization) 零分配解析与构造器（5/5 PASS，文档直写 `doc/lib/http_digest_auth.md`）
Subagent 1 吸收 `cap:http_digest_auth`，完工后已按轮换策略重建轻上下文子智能体 `Sys Config Absorber`。

## 2026-11-22: 新建 `cap:http_warning` RFC 7234 §5.5 HTTP Warning 头零分配解析器（引号逗号/转义/日期感知）与格式化器（5/5 PASS，文档直写 `doc/lib/http_warning.md`）
Subagent 2 吸收 `cap:http_warning`，完工后已按轮换策略重建轻上下文子智能体 `HTTP RFC Absorber`。

## 2026-11-22: 新建 `cap:dotenv_escape` .env 环境变量文件键名校验、防 $VAR 误展开安全引号序列化与反转义器（5/5 PASS，文档直写 `doc/lib/dotenv_escape.md`）
Subagent 1 吸收 `cap:dotenv_escape`，与 `cap:dotenv_parse`/`cap:env_expand` 构成完整 `.env` 工具链。

## 2026-11-22: 新建 `cap:http_content_encoding` RFC 9110 §8.4 Content-Encoding 编码栈零分配解析、别名归一化与最大压缩层数安全防护（5/5 PASS，文档直写 `doc/lib/http_content_encoding.md`）
Subagent 2 吸收 `cap:http_content_encoding`，零堆分配校验多层内容编码。

## 2026-11-22: 新建 `cap:number_format` 零分配人类可读数字格式化器（千分位分组、英文序数词、2..36 进制与定点小数）（5/5 PASS，文档直写 `doc/lib/number_format.md`）
主智能体吸收 `cap:number_format`，纯整数实现零浮点误差数字展示。

## 2026-11-22: 新建 `cap:unicode_width` Unicode 东亚宽度 / 终端显示列宽（wcwidth）零分配计算器（组合符=0、ASCII=1、CJK/Emoji=2）（5/5 PASS，文档直写 `doc/lib/unicode_width.md`）
主智能体吸收 `cap:unicode_width`，服务 CLI 表格与进度条精确排版。

## 2026-11-22: 新建 `cap:http_te` RFC 9110 / RFC 9112 Transfer-Encoding 与 TE 头零分配解析及异常报文边界校验器（5/5 PASS，文档直写 `doc/lib/http_te.md`）
Subagent 1 吸收 `cap:http_te`，补齐 HTTP/1.1 传输编码边界防护。

## 2026-11-22: 新建 `cap:http_freshness` RFC 9111 §4.2 HTTP 缓存新鲜度生存期、Age 修正与 RFC 5861 SWR/SIE 窗口求值器（5/5 PASS，文档直写 `doc/lib/http_freshness.md`）
主智能体吸收 `cap:http_freshness`，与 `cap:http_cache_control`/`cap:http_date` 形成完整 HTTP 缓存引擎。

## 2026-11-22: 新建 `cap:http_via` RFC 9110 §7.6.3 Via 代理链头与 Max-Forwards 零分配解析、环路检测与追加构造器（5/5 PASS，文档直写 `doc/lib/http_via.md`）
Subagent 2 吸收 `cap:http_via`，零堆分配支撑反向代理链与环路防护。

## 2026-11-22: 新建 `cap:http_keep_alive` HTTP Keep-Alive 参数（timeout/max）零分配解析格式化器与 RFC 9112 §9.3 持久连接判定器（5/5 PASS，文档直写 `doc/lib/http_keep_alive.md`）
主智能体吸收 `cap:http_keep_alive`，严格遵循 RFC 9112 §9.3 `Connection: close` 优先级。

## 2026-11-22: 新建 `cap:http_clear_site_data` W3C Clear-Site-Data 响应头零分配解析与规范化构造器（cache/cookies/storage/executionContexts/clientHints/*）（5/5 PASS，文档直写 `doc/lib/http_clear_site_data.md`）
主智能体吸收 `cap:http_clear_site_data`，补全 Web 安全响应头工具链。

## 2026-11-22: 新建 `cap:csv_escape` RFC 4180 CSV / TSV 字段转义器、单行构造器、严格引号反转义器与电子表格公式前缀风险检测（5/5 PASS，文档直写 `doc/lib/csv_escape.md`）
Subagent 2 吸收 `cap:csv_escape`，零堆分配提供 CSV/TSV 字段安全序列化与公式注入风险预警。

## 2026-11-22: 新建 `cap:makefile_parse` GNU Makefile 目标、依赖、.PHONY 伪目标与变量赋值（=/ :=/ ?=/ +=）零分配解析器（5/5 PASS，文档直写 `doc/lib/makefile_parse.md`）
Subagent 1 吸收 `cap:makefile_parse`，精准区分规则 `:` 与赋值 `:=`/`?=`/`+=`。

## 2026-11-22: 新建 `cap:socket_addr` 零分配 host:port / [ipv6]:port 套接字地址解析器（支持严格远端模式与 :8080/:0 服务端绑定模式）（5/5 PASS，文档直写 `doc/lib/socket_addr.md`）
主智能体吸收 `cap:socket_addr`，区分 `parse()` 严格远端端点与 `parseBind()` 服务端监听地址语义。

## 2026-11-22: 新建 `cap:http_client_hints` W3C Sec-CH-UA / Sec-CH-UA-Mobile / Sec-CH-UA-Platform 客户端提示零分配解析器与 GREASE 识别（5/5 PASS，文档直写 `doc/lib/http_client_hints.md`）
主智能体吸收 `cap:http_client_hints`，与 `cap:http_user_agent` 互补覆盖现代浏览器客户端提示。

## 2026-11-22: 新建 `cap:yaml_escape` YAML 1.2 标量引号安全判定、单/双引号转义与 Unicode（\xHH/\uXXXX/\UXXXXXXXX）反转义器（5/5 PASS，文档直写 `doc/lib/yaml_escape.md`）
Subagent 2 吸收 `cap:yaml_escape`，覆盖全部 YAML 保留字、数字外观与控制字符转义。

## 2026-11-22: 新建 `cap:git_diff_stat` Git Unified Diff (diff --git) 零分配文件变更统计提取器（增删行数、重命名、新增/删除与二进制识别）（5/5 PASS，文档直写 `doc/lib/git_diff_stat.md`）
Subagent 1 吸收 `cap:git_diff_stat`，零堆分配汇总多文件 diff 统计指标。

## 2026-11-22: 新建 `cap:http_sec_fetch` W3C Fetch Metadata（Sec-Fetch-Site/Mode/Dest/User）零分配解析与资源隔离防护策略求值器（5/5 PASS，文档直写 `doc/lib/http_sec_fetch.md`）
主智能体吸收 `cap:http_sec_fetch`，阻断跨站 POST/XHR/`<object>`/`<embed>` 并兼容遗留客户端。

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
