## 2026-11-22: 新建 `cap:haproxy` 零分配 HAProxy `.cfg` 负载均衡配置段与指令解析器（5/5 PASS，文档直写 `doc/lib/haproxy.md`）
- **设计要点**：单单词 TS 风格包名 `cap:haproxy`；`HaSectionIter` 零拷贝切分 9 类段，`HaDirectiveIter` 精确剥离引号外行尾注释。

## 2026-11-22: 新建 `cap:nix` 零分配 Nix / `flake.nix` 表达式词法扫描、属性集顶层绑定提取与 64 层定界符校验器（5/5 PASS，文档直写 `doc/lib/nix.md`）
- **设计要点**：单单词 TS 风格包名 `cap:nix`；`NixAttrIter` 兼容 `rec { ... }` 与裸属性集主体并自动跳过嵌套分号与 `inherit`。

## 2026-11-22: 新建 `cap:caddy` 零分配 Caddyfile 反向代理/TLS 站点配置词法扫描、站点块迭代与 64 层结构校验器（5/5 PASS，文档直写 `doc/lib/caddy.md`）
- **设计要点**：单单词 TS 风格包名 `cap:caddy`，与 `cap:nginx` 配套；精准区分行内 `{http.request.uri}` 占位符与块定界符 `{`。

## 2026-11-22: 新建 `cap:nginx` 零分配 Nginx `.conf` 配置词法扫描、层级指令块迭代与 64 层结构校验器（5/5 PASS，文档直写 `doc/lib/nginx.md`）
- **设计要点**：单单词 TS 风格包名 `cap:nginx`；`NginxBlockIter` 支持 `initWithDepth` 零拷贝递归解析嵌套块。

## 2026-11-22: 新建 `cap:rego` 零分配 Open Policy Agent OPA Rego 策略语言词法扫描、包名/规则提取与 64 层定界符校验器（5/5 PASS，文档直写 `doc/lib/rego.md`）
- **设计要点**：单单词 TS 风格包名 `cap:rego`；`RegoRuleIter` 兼容 `default`、`if`、`:=`、`contains`、索引规则与函数规则并跳过 `else`。

## 2026-11-22: 新建 `cap:xpath` 零分配 W3C XPath 表达式词法扫描、位置路径步进迭代与括号平衡校验器（5/5 PASS，文档直写 `doc/lib/xpath.md`）
- **设计要点**：单单词 TS 风格包名 `cap:xpath`；`XPathStepIter` 零分配跳过谓词内部 `/` 与字符串。

## 2026-11-22: 新建 `cap:idl` 零分配 W3C WebIDL 接口/字典/枚举/扩展属性词法扫描与顶层声明提取器（5/5 PASS，文档直写 `doc/lib/idl.md`）
- **设计要点**：单单词 TS 风格包名 `cap:idl`；`WebIdlDeclIter` 零分配支持 `interface mixin`、`partial dictionary`、`A includes B;` 等 WebIDL 规范语法。

## 2026-11-22: 新建 `cap:sparql` 零分配 W3C SPARQL 1.1 知识图谱查询词法扫描、前缀提取与只读安全判定器（5/5 PASS，文档直写 `doc/lib/sparql.md`）
- **设计要点**：单单词 TS 风格包名 `cap:sparql`，与 `cap:cypher` 构成图查询安全分析双件套；精准区分 `<iri>` 与 `<`/`<=` 比较运算符。

## 2026-11-22: 新建 `cap:cypher` 零分配 openCypher / Neo4j 图查询语言词法扫描、参数提取与只读安全判定器（5/5 PASS，文档直写 `doc/lib/cypher.md`）
- **设计要点**：单单词 TS 风格包名 `cap:cypher`；`isReadOnlyCypher` 覆盖顶层与 `CALL { ... }` 子查询写关键字拦截并避免属性/标签/字面量误判。

## 2026-11-22: 新建 `cap:thrift` 零分配 Apache Thrift `.thrift` IDL 顶层声明/嵌套泛型字段提取与括号平衡校验器（5/5 PASS，文档直写 `doc/lib/thrift.md`）
- **设计要点**：单单词 TS 风格包名 `cap:thrift`；`ThriftFieldIter` 零分配处理嵌套泛型类型与注解；全部 3 路常驻子智能体已完成新一轮低上下文轮换。

## 2026-11-22: 新建 `cap:glsl` 零分配 OpenGL / Vulkan GLSL 着色器词法扫描、`#version` 与 `uniform`/`in`/`out` 声明提取器（5/5 PASS，文档直写 `doc/lib/glsl.md`）
- **设计要点**：单单词 TS 风格包名 `cap:glsl`，与 `cap:wgsl` 构成双着色器分析套件；`GlslDeclIter` 自动忽略函数体内部声明。

## 2026-11-22: 新建 `cap:cue` 零分配 CUE 配置与数据约束语言词法扫描、包声明提取与 64 层平衡校验器（5/5 PASS，文档直写 `doc/lib/cue.md`）
- **设计要点**：单单词 TS 风格包名 `cap:cue`；`CueIter` 零拷贝切分 22 类 token 并防护未闭合字符串与嵌套深度。

## 2026-11-22: 新建 `cap:wat` 零分配 WebAssembly Text Format `.wat` 词法扫描、导出声明提取与模块结构校验器（5/5 PASS，文档直写 `doc/lib/wat.md`）
- **设计要点**：单单词 TS 风格包名 `cap:wat`；`WatExportIter` 统一支持显式 `(export ...)` 与内联 `(func $id (export ...))` 声明。

## 2026-11-22: 新建 `cap:wgsl` W3C WebGPU Shading Language `.wgsl` 零分配词法扫描、入口点与资源绑定提取器（5/5 PASS，文档直写 `doc/lib/wgsl.md`）
- **设计要点**：单单词 TS 风格包名 `cap:wgsl`；支持任意顺序 `@group`/`@binding` 与带地址空间模板的 `var<...>` 绑定声明。

## 2026-11-22: 新建 `cap:hcl` 零分配 HashiCorp HCL / Terraform `.tf` 词法扫描、顶层块头迭代与结构校验器（5/5 PASS，文档直写 `doc/lib/hcl.md`）
- **设计要点**：单单词 TS 风格包名 `cap:hcl`；`HclBlockIter` 零分配跳过嵌套块与顶层属性赋值。

## 2026-11-22: 新建 `cap:dot` 零分配 Graphviz DOT 有向/无向图描述语言词法扫描、链式边解析与序列化器（5/5 PASS，文档直写 `doc/lib/dot.md`）
- **设计要点**：单单词 TS 风格包名 `cap:dot`；`DotEdgeIter` 支持 `a -> b -> c` 链式展开、`subgraph` 穿透与双引号节点名。

## 2026-11-22: 新建 `cap:hjson` 零分配 Hjson Human JSON 词法扫描、注释剥离与标准 RFC 8259 JSON 转换器（5/5 PASS，文档直写 `doc/lib/hjson.md`）
- **设计要点**：单单词 TS 风格包名 `cap:hjson`；`hjsonToJson` 与 `stripHjsonComments` 均完整保护单/双/三引号内部文本。

## 2026-11-22: 新建 `cap:ron` 零分配 Rusty Object Notation `.ron` 词法扫描、括号平衡校验与顶层字段提取器（5/5 PASS，文档直写 `doc/lib/ron.md`）
- **设计要点**：单单词 TS 风格包名 `cap:ron`；`extractRonField` 零分配跳过嵌套结构与字符串内冒号/逗号。

## 2026-11-22: 新建 `cap:bibtex` 零分配 BibTeX `.bib` 学术文献条目解析、嵌套花括号字段迭代与格式化器（5/5 PASS，文档直写 `doc/lib/bibtex.md`）
- **设计要点**：单单词 TS 风格包名 `cap:bibtex`；`BibEntryIter`/`BibFieldIter` 零拷贝切分并防护未闭合条目。

## 2026-11-22: 新建 `cap:plist` 零分配 Apple XML Property List `.plist` 顶层字典解析与 XML 转义序列化器（5/5 PASS，文档直写 `doc/lib/plist.md`）
- **设计要点**：单单词 TS 风格包名 `cap:plist`；`PlistDictIter` 零分配追踪嵌套深度防止子树键泄露至顶层。

## 2026-11-22: 新建 `cap:kdl` 零分配 KDL 配置文档语言词法扫描、节点头迭代与嵌套结构校验器（5/5 PASS，文档直写 `doc/lib/kdl.md`）
- **设计要点**：单单词 TS 风格包名 `cap:kdl`；`KdlNodeIter` 零分配追踪 `{}` 嵌套深度并自动跳过 `/-` 注释节点/属性。

## 2026-11-22: 新建 `cap:srt` 零分配 SubRip `.srt` 字幕解析、时间戳编解码、饱和平移与格式化器（5/5 PASS，文档直写 `doc/lib/srt.md`）
- **设计要点**：单单词 TS 风格包名 `cap:srt`，与 `cap:vtt` 构成完整字幕生态；`shiftSrtCue` 负偏移饱和钳位至 0、正偏移防 `u64` 上溢。

## 2026-11-22: 新建 `cap:vtt` W3C WebVTT 字幕/时间轴零分配解析、时间戳编解码与格式化器（5/5 PASS，文档直写 `doc/lib/vtt.md`）
- **设计要点**：单单词 TS 风格包名 `cap:vtt`；`VttCue.durationMs()` 饱和减法防下溢；支持 CRLF 与多行字幕正文切片。

## 2026-11-22: 新建 `cap:json5` 零分配 JSON5 1.0 词法扫描、语法校验与标准 RFC 8259 JSON 转换器（5/5 PASS，文档直写 `doc/lib/json5.md`）
- **设计要点**：单单词 TS 风格包名 `cap:json5`，首行带精确上游 `Synced:` 时间戳；`toStandardJson` 零分配转换并防护嵌套深度与缓冲溢出。

## 2026-11-22: 新建 `cap:opml` 零分配 OPML 2.0 订阅大纲 XML 解析、层级迭代与属性转义序列化器（5/5 PASS，文档直写 `doc/lib/opml.md`）
- **设计要点**：单单词 TS 风格包名 `cap:opml`；`OpmlOutlineIter` 零分配维护嵌套与自闭合 `<outline>` 层级并防护多余闭合标签下溢。

## 2026-11-22: 新建 `cap:atom` RFC 4287 Atom 1.0 XML Feed 零分配解析、条目迭代与 XML 序列化器（5/5 PASS，文档直写 `doc/lib/atom.md`）
- **设计要点**：单单词 TS 风格包名 `cap:atom`；与 `cap:rss` 配套补齐聚合订阅生态，`AtomEntryIter` 零分配处理嵌套 `<author><name>` 与自闭合 `<link/>`。

## 2026-11-22: 新建 `cap:rss` 零分配 RSS 2.0 XML Feed 解析、频道元数据提取、条目迭代与 XML 序列化器（5/5 PASS，文档直写 `doc/lib/rss.md`）
- **设计要点**：单单词 TS 风格包名 `cap:rss`；`parseRssChannel` 自动跳过 `<item>`/`<image>` 子树同名标签；`formatRssItem` 零分配实体转义并校验缓冲容量。

## 2026-11-22: 新建 `cap:cuid` 零分配 CUID / CUID2 碰撞安全 ID 校验、分类、Base36 与确定性编码器（5/5 PASS，文档直写 `doc/lib/cuid.md`）
- **设计要点**：单单词 TS 风格包名 `cap:cuid`；`formatCuid1` 支持 base36 指纹直写及任意主机名 FNV-1a 确定性映射；`decodeBase36` 拦截溢出与非法字符。

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

