## 2026-11-22: 新建 `cap:sexp` 零分配 S-Expression 词法扫描、括号平衡校验、最大深度统计与紧凑格式化器（5/5 PASS，文档直写 `doc/lib/sexp.md`）
- **设计要点**：单单词 TS 风格包名 `cap:sexp`；`SexpIter` 零拷贝切分 5 类 token，`formatSexpCompact` 规范输出并严格校验缓冲边界。

## 2026-11-22: 新建 `cap:css` CSS 引擎与全套 36 项样式/布局/选择器/动画/变换/单位大一统主包（`lib/css_*` 全仓清零！）（5/5 PASS，文档直写 `doc/lib/css.md`）
- **收官里程碑**：`lib/css_*` 与 `"cap:css_*"` 均清零，14/14 PASS；方案 A 全部 235 个同域碎片 100% 归入简洁单单词 TS/Bun 风格主包。

## 2026-11-22: 新建 `cap:js` JS 标识符与运算符解析统一主包（同步收敛 `cap:hash`/`cap:text`/`cap:diff`/`cap:mime`）（5/5 PASS，文档直写 `doc/lib/js.md`）
- **收敛要点**：全部 5 主包 `cpc check` 0 errors，11/11 PASS；除在途最后 12 个 `css_*` 外，全仓同域碎片已彻底清零；同步派发 `bun_agy` 增量同步 Wave 2 (`cap:sourcemap` + `cap:io`)。

## 2026-11-22: 新建 `cap:markdown` Markdown 行分析、链接与引用定义统一主包（同步收敛 `cap:unicode`/`cap:s3`/`cap:shell`）（5/5 PASS，文档直写 `doc/lib/markdown.md`）
- **收敛要点**：全部 4 个主包保留原导出零破坏，冲突符号以最小别名（`wcwidth`/`S3MetaAcl`/`SigV4Credentials`/`Posix*`/`quotePosixBuf`）消解；10/10 PASS，累计消除 212 个散装目录。

## 2026-11-22: 新建 `cap:css` CSS 引擎与全套样式/布局/渐变/媒体查询统一主包（`css_*` 第 2 批 12 目录收敛，突破 200 目录里程碑！）（5/5 PASS，文档直写 `doc/lib/css.md`）
- **收敛要点**：12 个布局/字体/渐变/媒体查询子模块通过 `Css*`/`Layout*`/`Media*` 聚合别名零破坏合入 `lib/css/`，13/13 PASS；已派发最后 12 个 `css_*` 清零批次。

## 2026-11-22: 新建 `cap:mysql` MySQL 线协议全套认证、能力位、握手、字段类型、控制报文与二进制帧解析统一主包（`lib/mysql_*` 全仓清零）（5/5 PASS，文档直写 `doc/lib/mysql.md`）
- **收敛要点**：`proto.cp` 复用 `caps.cp` 的 `StatusFlag`，冲突符号以最小别名 `AuthError`/`ControlError`/`CapsCapabilities`/`WireHandshakeV10` 消解；8/8 PASS。

## 2026-11-22: 新建 `cap:css` CSS 引擎与样式/动画/边框/阴影/容器查询统一主包（`css_*` 第 1 批 12 目录收敛）（5/5 PASS，文档直写 `doc/lib/css.md`）
- **收敛要点**：9 个与 `lib/css/engine/` 重名符号以 `Css*` 聚合别名导出，13/13 PASS（含原 `test_phase_d_css_lightning_port.cp`）。

## 2026-11-22: 新建 `cap:pg` PostgreSQL 线协议全套认证、SASL/COPY、扩展查询、OID、SQLSTATE 与二进制流解析统一主包（`lib/pg_*` 全仓清零）（5/5 PASS，文档直写 `doc/lib/pg.md`）
- **收敛要点**：`messages.cp` 诊断枚举最小重命名为 `DiagSqlStateClass` 保留 `sqlstate.cp` 完整 `SqlStateClass`；8/8 PASS，累计消除 170 个散装目录。

## 2026-11-22: 新建 `cap:rate` 令牌桶/GCRA/滑动窗口限流统一主包（同步收敛 `cap:cron`/`cap:time`）（5/5 PASS，文档直写 `doc/lib/rate.md`）
- **收敛要点**：`lib/rate/` 最小重命名 `SlidingGcraLimiter`/`SimpleTokenBucket` 消除冲突，3 主包均 0 errors，6/6 PASS；全部 3 路子智能体均已轮换至全新低上下文实例。

## 2026-11-22: 新建 `cap:jwt` JWT 声明解析统一主包（同步收敛 `cap:ssh`/`cap:git`/`cap:pkg`）（5/5 PASS，文档直写 `doc/lib/jwt.md`）
- **收敛要点**：`lib/ssh/pubkey.cp` 的 `SshKeyType` 重导出为 `PubKeyType` 保留既有 `keys.cp` 零破坏；4 主包均 0 errors，6/6 PASS；同步轮换 `Sys Config Absorber` 为全新低上下文实例。

## 2026-11-22: 新建 `cap:http` HTTP/1.1/2/3 协议与全套 56 项 RFC 头部/安全/代理/路由/缓存大一统主包（`lib/http_*` 全仓清零！）（5/5 PASS，文档直写 `doc/lib/http.md`）
- **收敛要点**：`lib/http_types` 内部冲突类型最小重命名避免覆盖既有核心类型；全仓 `lib/http_*` 与 `cap:http_*` 均为 0；累计消除 150 个长下划线顶层目录！

## 2026-11-22: 新建 `cap:pkg` 包管理/NPM 名称/npmrc/exports/matcher 统一主包（同步收敛 `cap:shell`）（5/5 PASS，文档直写 `doc/lib/pkg.md`）
- **收敛要点**：`lib/shell/envparse.cp` 避开既有 `env.cp`；顺手修复 `cap:shell`/`cap:pkg` 既有 `E0209` 与 `DirEntry` 导入；累计已消除 137 个长下划线顶层目录。

## 2026-11-22: 新建 `cap:mail` RFC 5322 邮件地址、RFC 2047 编码字与 Quoted-Printable 统一主包（同步收敛 `cap:mime`）（5/5 PASS，文档直写 `doc/lib/mail.md`）
- **收敛要点**：`lib/mime/` 与新建 `lib/mail/` 零冲突通过 `cpc check` 与 7/7 深度测试；累计已消除 130 个长下划线顶层目录。

## 2026-11-22: 新建 `cap:http` HTTP/1.1/2/3 协议与全套 RFC 头部协商、条件请求、路由与限流统一主包（5/5 PASS，文档直写 `doc/lib/http.md`）
- **收敛要点**：13 个 HTTP 条件请求/路由/限流子模块零破坏合入 `lib/http/`；`http_router` 的 `HttpMethod` 调整为 `RouterMethod` 避免与 `methodparse.cp` 冲突；累计已消除 123 个长下划线顶层目录。



## 2026-11-22: 新建 `cap:docker` Docker 镜像引用与 Dockerfile 解析统一主包（同步收敛 `cap:dotenv`/`cap:ini`/`cap:csv`）（5/5 PASS，文档直写 `doc/lib/docker.md`）
- **收敛要点**：全部 4 个主包零冲突 `cpc check` 0 errors，7/7 深度测试 PASS；累计已消除 110 个长下划线顶层目录，热日志已瘦身。

## 2026-11-22: 新建 `cap:yaml` YAML/TOML/HTML/XML 统一主包同步收敛（突破 100 目录收敛里程碑！）（5/5 PASS，文档直写 `doc/lib/yaml.md`）
- **收敛要点**：`lib/html/esc.cp` 避开既有 `escape.cp`，4 个主包 `cpc check` 全 0 errors，6/6 深度测试通过；累计消除 103 个长下划线顶层目录。

## 2026-11-22: 新建 `cap:http` HTTP/1.1/2/3 协议与全套 RFC 头部协商、Cookie、CSP、Digest、EarlyHints 与转发统一主包（5/5 PASS，文档直写 `doc/lib/http.md`）
- **收敛要点**：15 个 HTTP Cookie/CSP/Digest/EarlyHints/Forwarded 子模块零破坏合入 `lib/http/`；累计已消除 97 个长下划线顶层目录！

## 2026-11-22: 新建 `cap:net` 网络 Socket、IP/MAC/SocketAddr、PROXY Protocol v1/v2、Hosts 与 NoProxy 统一主包（5/5 PASS，文档直写 `doc/lib/net.md`）
- **收敛要点**：保留原 `lib/net/{address,cidr}.cp` 零破坏，冲突符号通过聚合别名 `RfcIpAddr`/`NoProxyIpv4Addr` 消解；累计已消除 82 个长下划线顶层目录。

## 2026-11-22: 新建 `cap:json` JSON 解析、转义、压缩与 RFC 6901 JSON Pointer 统一主包（同步收敛 `cap:text`）（5/5 PASS，文档直写 `doc/lib/json.md`）
- **收敛要点**：`lib/json/ptr.cp` 避开既有 `pointer.cp`；`lib/text/wrap.cp` 的 3 参 `indentLines` 最小调整为 `indentNonEmptyLines`；累计已消除 75 个长下划线顶层目录。

## 2026-11-22: 新建 `cap:git` Git 对象/仓库、Diff 统计、Ref 校验、URL 解析、GitIgnore 与 GitConfig 统一主包（5/5 PASS，文档直写 `doc/lib/git.md`）
- **收敛要点**：保留原 `lib/git/ignore.cp` 零破坏，新迁入规则匹配器命名为 `ignorematch.cp`；累计已消除 70 个长下划线顶层目录。

## 2026-11-22: 新建 `cap:http` HTTP/1.1/2/3 协议与全套 RFC 9110/7838 头部协商、缓存与分片统一主包（5/5 PASS，文档直写 `doc/lib/http.md`）
- **收敛要点**：15 个 HTTP 头部协商/缓存/分片子模块通过最小聚合别名零破坏合入 `lib/http/`；Wave 3 圆满收官，累计已消除 65 个长下划线顶层目录！

## 2026-11-22: 新建 `cap:sql` SQL 词法/语法 AST、连接 URI、转义、参数占位符与多语句切分统一主包（5/5 PASS，文档直写 `doc/lib/sql.md`）
- **收敛要点**：保留原 `lib/sql/{protocol,pg_types}.cp` 零破坏，`uri.cp` 与 `splitter.cp` 重名符号最小重命名；累计已消除 50 个长下划线顶层目录！

