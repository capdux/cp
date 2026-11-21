# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:woff2` 零分配 W3C WOFF2 网页字体 48 字节头、UIntBase128 变长整数与 63 项已知表目录解析器（5/5 PASS，自动动态挂载 `cap.woff2`）
- 新增 `lib/woff2/{woff2,index}.cp`、`doc/lib/woff2.md`、`cap/test/test_woff2_production_deep.cp`：纯 CP 零分配实现 W3C WOFF2 48 字节头校验、UIntBase128 严格解码（最多 5 字节/禁前导 `0x80`/防溢出）与 63 已知表 tag + 自定义 tag 目录迭代（5/5 PASS）。

## [2026-11-22] - 新建 `cap:ico` 零分配 Windows ICO / CUR 图标容器二进制目录解析、PNG/BMP 负载提取与最佳尺寸选择器（5/5 PASS，自动动态挂载 `cap.ico`）
- 新增 `lib/ico/{ico,index}.cp`、`doc/lib/ico.md`、`cap/test/test_ico_production_deep.cp`：纯 CP 零分配实现 ICO/CUR 6 字节头与 16 字节目录项解析（`0`->`256`、PNG 魔数识别、越界防护与最佳尺寸/高色深选择）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:macho` 零分配 Apple Mach-O 32/64-bit 及 Fat Universal 二进制头与 Load Command 解析器（5/5 PASS，自动动态挂载 `cap.macho`）
- 新增 `lib/macho/{macho,index}.cp`、`doc/lib/macho.md`、`cap/test/test_macho_production_deep.cp`：纯 CP 零分配实现 Apple Mach-O 32/64-bit 与 Fat Universal 二进制头、8 种魔数识别、Load Command 与架构切片边界安全迭代（5/5 PASS）。

## [2026-11-22] - 新建 `cap:haproxy` 零分配 HAProxy `.cfg` 负载均衡配置段与指令解析器（5/5 PASS，自动动态挂载 `cap.haproxy`）
- 新增 `lib/haproxy/{haproxy,index}.cp`、`doc/lib/haproxy.md`、`cap/test/test_haproxy_production_deep.cp`：纯 CP 零分配实现 HAProxy `.cfg` 段切分与指令迭代（兼容 CRLF、保护引号内 `#` 与 `\#` 转义），与 `cap:nginx`/`cap:caddy` 集齐反向代理配置三件套（5/5 PASS）。

## [2026-11-22] - 新建 `cap:nix` 零分配 Nix / `flake.nix` 表达式词法扫描、属性集顶层绑定提取与 64 层定界符校验器（5/5 PASS，自动动态挂载 `cap.nix`）
- 新增 `lib/nix/{nix,index}.cp`、`doc/lib/nix.md`、`cap/test/test_nix_production_deep.cp`：纯 CP 零分配实现 Nix 词法扫描（`${...}` 嵌套插值、`...` 缩进串、路径/URI）、属性集顶层 `key = value;` 提取与 64 层平衡校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:caddy` 零分配 Caddyfile 反向代理/TLS 站点配置词法扫描、站点块迭代与 64 层结构校验器（5/5 PASS，自动动态挂载 `cap.caddy`）
- 新增 `lib/caddy/{caddy,index}.cp`、`doc/lib/caddy.md`、`cap/test/test_caddy_production_deep.cp`：纯 CP 零分配实现 Caddyfile 词法扫描（`@matcher`、反引号原始串、`{placeholder}` 与块大括号区分）、全局块/Snippet/站点块迭代与 64 层校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:nginx` 零分配 Nginx `.conf` 配置词法扫描、层级指令块迭代与 64 层结构校验器（5/5 PASS，自动动态挂载 `cap.nginx`）
- 新增 `lib/nginx/{nginx,index}.cp`、`doc/lib/nginx.md`、`cap/test/test_nginx_production_deep.cp`：纯 CP 零分配实现 Nginx `.conf` 词法扫描、`http`/`upstream`/`server`/`location` 递归块迭代与 `${var}` 感知括号校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:rego` 零分配 Open Policy Agent OPA Rego 策略语言词法扫描、包名/规则提取与 64 层定界符校验器（5/5 PASS，自动动态挂载 `cap.rego`）
- 新增 `lib/rego/{rego,index}.cp`、`doc/lib/rego.md`、`cap/test/test_rego_production_deep.cp`：纯 CP 零分配实现 OPA Rego 词法扫描、`package`/`import` 与全形态顶层规则名提取及 64 层定界符校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:xpath` 零分配 W3C XPath 表达式词法扫描、位置路径步进迭代与括号平衡校验器（5/5 PASS，自动动态挂载 `cap.xpath`）
- 新增 `lib/xpath/{xpath,index}.cp`、`doc/lib/xpath.md`、`cap/test/test_xpath_production_deep.cp`：纯 CP 零分配实现 XPath 词法扫描、位置路径步进迭代（支持 `//`、`axis::`、`@attr`、`text()`、命名空间 QName 与嵌套谓词）与 64 层括号校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:idl` 零分配 W3C WebIDL 接口/字典/枚举/扩展属性词法扫描与顶层声明提取器（5/5 PASS，自动动态挂载 `cap.idl`）
- 新增 `lib/idl/{idl,index}.cp`、`doc/lib/idl.md`、`cap/test/test_idl_production_deep.cp`：纯 CP 零分配实现 W3C WebIDL 词法扫描、8 类顶层声明（含 `[ExtAttr]` 与 `partial` 修饰）提取与 64 层括号平衡校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:sparql` 零分配 W3C SPARQL 1.1 知识图谱查询词法扫描、前缀提取与只读安全判定器（5/5 PASS，自动动态挂载 `cap.sparql`）
- 新增 `lib/sparql/{sparql,index}.cp`、`doc/lib/sparql.md`、`cap/test/test_sparql_production_deep.cp`：纯 CP 零分配实现 SPARQL 1.1 词法扫描、`PREFIX` 提取、查询形态识别与严格只读判定（拦截多语句写拼接与非法词法）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:cypher` 零分配 openCypher / Neo4j 图查询语言词法扫描、参数提取与只读安全判定器（5/5 PASS，自动动态挂载 `cap.cypher`）
- 新增 `lib/cypher/{cypher,index}.cp`、`doc/lib/cypher.md`、`cap/test/test_cypher_production_deep.cp`：纯 CP 零分配实现 openCypher 词法扫描、`$param` 提取、64 层括号校验与只读查询判定（精准区分属性 `n.delete`/标签 `:CREATE`/Map 键 `{ set: 1 }` 与真实写子句）（6/6 PASS）。

## [2026-11-22] - 新建 `cap:thrift` 零分配 Apache Thrift `.thrift` IDL 顶层声明/嵌套泛型字段提取与括号平衡校验器（5/5 PASS，自动动态挂载 `cap.thrift`）
- 新增 `lib/thrift/{thrift,index}.cp`、`doc/lib/thrift.md`、`cap/test/test_thrift_production_deep.cp`：纯 CP 零分配实现 Apache Thrift IDL 声明与字段解析（支持 `#`/`//`/`/* */` 注释、`service extends`、嵌套泛型 `map<string, list<i64>>` 与 64 层括号平衡校验）（5/5 PASS）；同步轻量化轮换 `CSS & Web Absorber`。

## [2026-11-22] - 新建 `cap:glsl` 零分配 OpenGL / Vulkan GLSL 着色器词法扫描、`#version` 与 `uniform`/`in`/`out` 声明提取器（5/5 PASS，自动动态挂载 `cap.glsl`）
- 新增 `lib/glsl/{glsl,index}.cp`、`doc/lib/glsl.md`、`cap/test/test_glsl_production_deep.cp`：纯 CP 零分配实现 GLSL 词法扫描、`#version` 解析、顶层 `layout(location,binding) uniform/in/out` 声明提取与括号平衡校验（5/5 PASS）；同步轻量化轮换 `PG & Data Absorber`。

## [2026-11-22] - 新建 `cap:cue` 零分配 CUE 配置与数据约束语言词法扫描、包声明提取与 64 层平衡校验器（5/5 PASS，自动动态挂载 `cap.cue`）
- 新增 `lib/cue/{cue,index}.cp`、`doc/lib/cue.md`、`cap/test/test_cue_production_deep.cp`：纯 CP 零分配实现 CUE 词法扫描（`#Def`/`_hidden`/`_|_`/`...`/`@attr(...)`/SI-IEC 后缀数字/多行字面量）、`package` 提取与 64 层平衡校验（5/5 PASS）；同步轻量化轮换 `DB & Protocol Absorber`。

## [2026-11-22] - 新建 `cap:wat` 零分配 WebAssembly Text Format `.wat` 词法扫描、导出声明提取与模块结构校验器（5/5 PASS，自动动态挂载 `cap.wat`）
- 新增 `lib/wat/{wat,index}.cp`、`doc/lib/wat.md`、`cap/test/test_wat_production_deep.cp`：纯 CP 零分配实现 WebAssembly `.wat` 词法扫描（嵌套 `(; ... ;)` 注释、`$id`、`nan:0x...`）、顶层/内联导出提取与 64 层 `(module ...)` 结构校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:wgsl` W3C WebGPU Shading Language `.wgsl` 零分配词法扫描、入口点与资源绑定提取器（5/5 PASS，自动动态挂载 `cap.wgsl`）
- 新增 `lib/wgsl/{wgsl,index}.cp`、`doc/lib/wgsl.md`、`cap/test/test_wgsl_production_deep.cp`：纯 CP 零分配实现 WGSL 词法扫描（嵌套 `/* */` 注释、`@attr`、后缀数字）、`@vertex`/`@fragment`/`@compute` 入口点与 `@group`/`@binding` 绑定提取（5/5 PASS）。

## [2026-11-22] - 新建 `cap:hcl` 零分配 HashiCorp HCL / Terraform `.tf` 词法扫描、顶层块头迭代与结构校验器（5/5 PASS，自动动态挂载 `cap.hcl`）
- 新增 `lib/hcl/{hcl,index}.cp`、`doc/lib/hcl.md`、`cap/test/test_hcl_production_deep.cp`：纯 CP 零分配实现 HCL/Terraform 词法扫描（`#`/`//`/`/* */`、`${...}`/`%{...}` 插值、`<<EOF`/`<<-EOF` Heredoc）、顶层块头迭代与 64 层深度平衡校验（5/5 PASS）。

## [2026-11-22] - 新建 `cap:dot` 零分配 Graphviz DOT 有向/无向图描述语言词法扫描、链式边解析与序列化器（5/5 PASS，自动动态挂载 `cap.dot`）
- 新增 `lib/dot/{dot,index}.cp`、`doc/lib/dot.md`、`cap/test/test_dot_production_deep.cp`：纯 CP 零分配实现 Graphviz DOT 图头解析、有向/无向/链式边迭代、字符串注释感知括号平衡校验与自动引号转义序列化（5/5 PASS）。

## [2026-11-22] - 新建 `cap:hjson` 零分配 Hjson Human JSON 词法扫描、注释剥离与标准 RFC 8259 JSON 转换器（5/5 PASS，自动动态挂载 `cap.hjson`）
- 新增 `lib/hjson/{hjson,index}.cp`、`doc/lib/hjson.md`、`cap/test/test_hjson_production_deep.cp`：纯 CP 零分配实现 Hjson 注释剥离与标准 JSON 转换（`#`/`//`/`/* */`、可选逗号、无引号键值、`...` 自动去缩进多行字面量与 64 层深度防护）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:ron` 零分配 Rusty Object Notation `.ron` 词法扫描、括号平衡校验与顶层字段提取器（5/5 PASS，自动动态挂载 `cap.ron`）
- 新增 `lib/ron/{ron,index}.cp`、`doc/lib/ron.md`、`cap/test/test_ron_production_deep.cp`：纯 CP 零分配实现 RON 词法扫描（嵌套 `/* */`、原始字符串 `r#"..."#`、字符字面量、多进制下划线数字）、64 层平衡校验与顶层字段切片提取（5/5 PASS）。

## [2026-11-22] - 新建 `cap:bibtex` 零分配 BibTeX `.bib` 学术文献条目解析、嵌套花括号字段迭代与格式化器（5/5 PASS，自动动态挂载 `cap.bibtex`）
- 新增 `lib/bibtex/{bibtex,index}.cp`、`doc/lib/bibtex.md`、`cap/test/test_bibtex_production_deep.cp`：纯 CP 零分配实现 BibTeX `.bib` 解析与格式化（跳过 `%` 注释与 `@comment`/`@preamble`/`@string`，支持 `{}`/`()` 两种条目定界、嵌套花括号平衡与大小写不敏感字段检索）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:plist` 零分配 Apple XML Property List `.plist` 顶层字典解析与 XML 转义序列化器（5/5 PASS，自动动态挂载 `cap.plist`）
- 新增 `lib/plist/{plist,index}.cp`、`doc/lib/plist.md`、`cap/test/test_plist_production_deep.cp`：纯 CP 零分配实现 Apple XML Plist 顶层字典迭代与类型查询（深度平衡跳过嵌套 `<dict>`/`<array>` 子树，支持负数/溢出拦截与 XML 实体转义）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:kdl` 零分配 KDL 配置文档语言词法扫描、节点头迭代与嵌套结构校验器（5/5 PASS，自动动态挂载 `cap.kdl`）
- 新增 `lib/kdl/{kdl,index}.cp`、`doc/lib/kdl.md`、`cap/test/test_kdl_production_deep.cp`：纯 CP 零分配实现 KDL 词法扫描与节点校验（嵌套 `/* */` 注释、`/-` slashdash、原始字符串 `r#"..."#`、多进制下划线数字与 64 层深度防护）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:srt` 零分配 SubRip `.srt` 字幕解析、时间戳编解码、饱和平移与格式化器（5/5 PASS，自动动态挂载 `cap.srt`）
- 新增 `lib/srt/{srt,index}.cp`、`doc/lib/srt.md`、`cap/test/test_srt_production_deep.cp`：纯 CP 零分配实现 SubRip `.srt` 字幕解析与时间轴饱和平移（BOM、CRLF/LF、逗号/点毫秒兼容、逆序/畸形块跳过）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:vtt` W3C WebVTT 字幕/时间轴零分配解析、时间戳编解码与格式化器（5/5 PASS，自动动态挂载 `cap.vtt`）
- 新增 `lib/vtt/{vtt,index}.cp`、`doc/lib/vtt.md`、`cap/test/test_vtt_production_deep.cp`：纯 CP 零分配实现 W3C WebVTT 字幕解析与格式化（BOM、`NOTE`/`STYLE`/`REGION` 跳过、`MM:SS.mmm`/`HH:MM:SS.mmm` 严格校验与往返格式化）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:json5` 零分配 JSON5 1.0 词法扫描、语法校验与标准 RFC 8259 JSON 转换器（5/5 PASS，自动动态挂载 `cap.json5`）
- 新增 `lib/json5/{json5,index}.cp`、`doc/lib/json5.md`、`cap/test/test_json5_production_deep.cp`：纯 CP 零分配实现 JSON5 1.0 词法器与标准 JSON 转换（支持单引号、未加引号键、十六进制、前导/尾随小数点、`+` 号、`Infinity`/`NaN`、续行字符串、注释与尾随逗号及 64 层深度防护）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:opml` 零分配 OPML 2.0 订阅大纲 XML 解析、层级迭代与属性转义序列化器（5/5 PASS，自动动态挂载 `cap.opml`）
- 新增 `lib/opml/{opml,index}.cp`、`doc/lib/opml.md`、`cap/test/test_opml_production_deep.cp`：纯 CP 零分配实现 OPML 2.0 解析与序列化（BOM/注释跳过、嵌套深度追踪、单双引号属性与防下溢），与 `cap:rss`/`cap:atom` 组成完整聚合订阅三件套（5/5 PASS）。

## [2026-11-22] - 新建 `cap:atom` RFC 4287 Atom 1.0 XML Feed 零分配解析、条目迭代与 XML 序列化器（5/5 PASS，自动动态挂载 `cap.atom`）
- 新增 `lib/atom/{atom,index}.cp`、`doc/lib/atom.md`、`cap/test/test_atom_production_deep.cp`：纯 CP 零分配实现 RFC 4287 Atom 1.0 解析与序列化（跳过 `<entry>` 子树与 XML 注释、CDATA 剥离、`<link rel="alternate">` 属性解析与 XML 实体转义）（5/5 PASS）。

## [2026-11-22] - 新建 `cap:rss` 零分配 RSS 2.0 XML Feed 解析、频道元数据提取、条目迭代与 XML 序列化器（5/5 PASS，自动动态挂载 `cap.rss`）
- 新增 `lib/rss/{rss,index}.cp`、`doc/lib/rss.md`、`cap/test/test_rss_production_deep.cp`：纯 CP 零分配实现 RSS 2.0 解析与序列化（UTF-8 BOM、CDATA 剥离与伪闭合防护、属性引号内 `>` 保护、XML 实体转义）（6/6 PASS）。

## [2026-11-22] - 新建 `cap:cuid` 零分配 CUID / CUID2 碰撞安全 ID 校验、分类、Base36 与确定性编码器（5/5 PASS，自动动态挂载 `cap.cuid`）
- 新增 `lib/cuid/{cuid,index}.cp`、`doc/lib/cuid.md`、`cap/test/test_cuid_production_deep.cp`：纯 CP 零分配实现 CUID v1/v2 校验、版本检测、64 位 Base36 编解码与 25 字节定长 CUID v1 确定性编码及时间戳提取（5/5 PASS）。

## [2026-11-22] - 新建 `cap:sexp` 零分配 S-Expression 词法扫描、括号平衡校验、最大深度统计与紧凑格式化器（5/5 PASS，自动动态挂载 `cap.sexp`）
- 新增 `lib/sexp/{sexp,index}.cp`、`doc/lib/sexp.md`、`cap/test/test_sexp_production_deep.cp`：纯 CP 零分配实现 S-Expression 词法扫描（跳过 `;` 注释、双引号转义与未闭合检测）、平衡校验与紧凑格式化（5/5 PASS）。

## [2026-11-22] - 新建 `cap:css` CSS 引擎与全套 36 项样式/布局/选择器/动画/变换/单位大一统主包（`lib/css_*` 全仓清零！）（5/5 PASS，自动动态挂载 `cap.css`）
- **方案 A 领域主包收敛全面收官 (`css_*` 最终第 3 批 12 目录 -> `cap:css`)**：将最后 12 个 `css_selector_builder` 至 `css_units` 合入 `lib/css/`（14/14 PASS），**全仓 36 个 `lib/css_*` 彻底清零**！至此全部 **235 个**同域长下划线散装目录 100% 收敛完毕，全面转入 3 路 `bun_agy` 上游增量同步！

## [2026-11-22] - 新建 `cap:js` JS 标识符与运算符解析统一主包（同步收敛 `cap:hash`/`cap:text`/`cap:diff`/`cap:mime`）（5/5 PASS，自动动态挂载 `cap.js`）
- **方案 A 收敛终章 (11 小簇目录 -> `cap:hash`/`cap:text`/`cap:diff`/`cap:js`/`cap:mime`)**：合入 `consistent_*`(3) 至 `cap:hash`、`case_*`(2) 至 `cap:text`、`diff_*`(2) 至 `cap:diff`、`mime_*`(2) 至 `cap:mime`，新建 `cap:js` 合入 `js_*`(2)（11/11 PASS），非 CSS 同域碎片 100% 清零，累计消除 223 个长下划线顶层目录！

## [2026-11-22] - 新建 `cap:markdown` Markdown 行分析、链接与引用定义统一主包（同步收敛 `cap:unicode`/`cap:s3`/`cap:shell`）（5/5 PASS，自动动态挂载 `cap.markdown`）
- **方案 A 收敛 (10 小簇目录 -> `cap:markdown`/`cap:unicode`/`cap:s3`/`cap:shell`)**：新建 `cap:markdown` 并合入 `md_*`(3)，同时合入 `unicode_*`(3) 至 `cap:unicode`、`s3_*`(2) 至 `cap:s3`、`shell_*`(2) 至 `cap:shell`（10/10 PASS），删除 10 个旧目录与文档，累计消除 212 个长下划线顶层目录。

## [2026-11-22] - 新建 `cap:css` CSS 引擎与全套样式/布局/渐变/媒体查询统一主包（`css_*` 第 2 批 12 目录收敛，突破 200 目录里程碑！）（5/5 PASS，自动动态挂载 `cap.css`）
- **方案 A 收敛里程碑 (`css_*` 第 2 批 12 目录 -> `cap:css`)**：将 `css_flex_grid` 至 `css_primitives` 12 个散装目录零破坏合入 `lib/css/`（13/13 PASS），删除 12 个旧目录与文档，**累计消除 202 个长下划线顶层目录（突破 200 目录里程碑）**！

## [2026-11-22] - 新建 `cap:mysql` MySQL 线协议全套认证、能力位、握手、字段类型、控制报文与二进制帧解析统一主包（`lib/mysql_*` 全仓清零）（5/5 PASS，自动动态挂载 `cap.mysql`）
- **方案 A 收敛 (`mysql_*` 8 目录 -> `cap:mysql`)**：新建 `lib/mysql/{auth,caps,control,fieldtypes,handshake,packet,proto,wire,index}.cp` 与 `doc/lib/mysql.md`，全部 8 套深度测试 100% PASS，全仓 `lib/mysql_*` 与 `cap:mysql_*` 彻底清零（累计消除 190 个长下划线顶层目录）。

## [2026-11-22] - 新建 `cap:css` CSS 引擎与样式/动画/边框/阴影/容器查询统一主包（`css_*` 第 1 批 12 目录收敛）（5/5 PASS，自动动态挂载 `cap.css`）
- **方案 A 收敛 (`css_*` 第 1 批 12 目录 -> `cap:css`)**：将 `css_animation` 至 `css_effects` 12 个散装目录零破坏合入 `lib/css/` 并保留原 `lib/css/engine/` 全部导出（13/13 PASS），删除 12 个旧目录与文档，累计消除 182 个长下划线顶层目录。

## [2026-11-22] - 新建 `cap:pg` PostgreSQL 线协议全套认证、SASL/COPY、扩展查询、OID、SQLSTATE 与二进制流解析统一主包（`lib/pg_*` 全仓清零）（5/5 PASS，自动动态挂载 `cap.pg`）
- **方案 A 收敛 (`pg_*` 8 目录 -> `cap:pg`)**：新建 `lib/pg/{auth,copysasl,extquery,messages,oids,reader,sqlstate,wiretypes,index}.cp` 与 `doc/lib/pg.md`，全部 8 套深度测试 100% PASS，全仓 `lib/pg_*` 与 `cap:pg_*` 彻底清零（累计消除 170 个长下划线顶层目录）。

## [2026-11-22] - 新建 `cap:rate` 令牌桶/GCRA/滑动窗口限流统一主包（同步收敛 `cap:cron`/`cap:time`）（5/5 PASS，自动动态挂载 `cap.rate`）
- **方案 A 收敛 Wave 7 (6 限流/Cron/时间目录 -> `cap:rate`/`cap:cron`/`cap:time`)**：新建 `cap:rate` 合入 `rate_sliding_window`/`rate_token_bucket`/`token_bucket`，合入 `cron_preset` 至 `cap:cron`、`duration_parse`/`rfc3339_time` 至 `cap:time`（6/6 PASS），删除 6 个旧目录与文档，累计消除 162 个长下划线顶层目录；同步轮换 `CLI & Format Absorber`。

## [2026-11-22] - 新建 `cap:jwt` JWT 声明解析统一主包（同步收敛 `cap:ssh`/`cap:git`/`cap:pkg`）（5/5 PASS，自动动态挂载 `cap.jwt`）
- **方案 A 收敛 Wave 8 (6 开发工具/SSH/认证目录 -> `cap:ssh`/`cap:git`/`cap:pkg`/`cap:jwt`)**：合入 `ssh_config`/`ssh_pubkey` 至 `cap:ssh`、`conventional_commit`/`codeowners` 至 `cap:git`、`spdx_license` 至 `cap:pkg`，新建 `cap:jwt` 合入 `jwt_claims`（6/6 PASS），删除 6 个旧目录与文档，累计消除 156 个长下划线顶层目录。

## [2026-11-22] - 新建 `cap:http` HTTP/1.1/2/3 协议与全套 56 项 RFC 头部/安全/代理/路由/缓存大一统主包（`lib/http_*` 全仓清零！）（5/5 PASS，自动动态挂载 `cap.http`）
- **方案 A 收敛里程碑 (`http_*` 最终第 4 批 13 目录 -> `cap:http`)**：将最后 13 个 `http_sec_fetch` 至 `http_warning` 合入 `lib/http/`（13/13 PASS），**全仓 56 个 `lib/http_*` 散装目录彻底清零**！累计已消除 **150 个**长下划线顶层目录。

## [2026-11-22] - 新建 `cap:pkg` 包管理/NPM 名称/npmrc/exports/matcher 统一主包（同步收敛 `cap:shell`）（5/5 PASS，自动动态挂载 `cap.pkg`）
- **方案 A 收敛 Wave 7 (7 Shell/包管理目录 -> `cap:shell`/`cap:pkg`)**：将 `shell_env_parse`/`shell_quote`/`brace_expand` 合入 `lib/shell/`，将 `npm_package_name`/`npmrc_config`/`pkg_exports`/`pkg_matcher` 合入 `lib/pkg/`（7/7 PASS），顺手修复 `lib/glob/walker.cp`、`lib/pkg/types.cp` `E0209` 及缺失 `DirEntry` 导入，删除 7 个旧目录与文档。

## [2026-11-22] - 新建 `cap:mail` RFC 5322 邮件地址、RFC 2047 编码字与 Quoted-Printable 统一主包（同步收敛 `cap:mime`）（5/5 PASS，自动动态挂载 `cap.mail`）
- **方案 A 收敛 Wave 6 (7 MIME/Mail 目录 -> `cap:mime`/`cap:mail`)**：将 `mime_headers`/`mime_sniff`/`multipart_boundary`/`content_disposition` 合入 `lib/mime/`，新建 `lib/mail/` 合入 `mail_address`/`rfc2047_word`/`quoted_printable`（7/7 PASS），删除 7 个旧目录与文档；同步完成热修 `#86` 验收提交与回收。

## [2026-11-22] - 新建 `cap:http` HTTP/1.1/2/3 协议与全套 RFC 头部协商、条件请求、路由与限流统一主包（5/5 PASS，自动动态挂载 `cap.http`）
- **方案 A 收敛 Wave 5 (`http_*` 第 3 批 13 目录 -> `cap:http`)**：将 `http_if_match` 至 `http_router` 共 13 个散装目录合并收敛至 `lib/http/{ifmatch,keepalive,link,maxforwards,methodparse,origin,permissions,prefer,proxyurl,range,ratelimit,retryafter,router}.cp` 并由 `lib/http/index.cp` 零破坏聚合导出（13/13 PASS），删除 13 个旧目录与旧文档。

## [2026-11-22] - 新建 `cap:docker` Docker 镜像引用与 Dockerfile 解析统一主包（同步收敛 `cap:dotenv`/`cap:ini`/`cap:csv`）（5/5 PASS，自动动态挂载 `cap.docker`）
- **方案 A 收敛 Wave 6 (7 配置/容器目录 -> `cap:docker`/`cap:dotenv`/`cap:ini`/`cap:csv`)**：新建 `cap:docker` 并合入 `docker_image_ref`/`dockerfile_parse`，同时合入 `dotenv_escape`/`env_expand` 至 `cap:dotenv`、`ini_writer`/`properties_parse` 至 `cap:ini`、`csv_escape` 至 `cap:csv`（7/7 PASS），删除 7 个旧目录与文档；顺带滚动归档热日志至 `*_H.md`。

## [2026-11-22] - 新建 `cap:yaml` YAML/TOML/HTML/XML 统一主包同步收敛（突破 100 目录收敛里程碑！）（5/5 PASS，自动动态挂载 `cap.yaml`）
- **方案 A 收敛 Wave 5 (6 标记/配置目录 -> `cap:html`/`cap:xml`/`cap:yaml`/`cap:toml`)**：将 `html_escape`、`xml_escape`、`yaml_escape`/`yaml_scalar`、`toml_escape`/`toml_lexer` 6 个散装目录零冲突合并至各自单单词领域主包（6/6 PASS），删除 6 个旧目录与旧文档，累计已消除 103 个长下划线顶层目录（突破百目录里程碑）！

## [2026-11-22] - 新建 `cap:http` HTTP/1.1/2/3 协议与全套 RFC 头部协商、Cookie、CSP、Digest、EarlyHints 与转发统一主包（5/5 PASS，自动动态挂载 `cap.http`）
- **方案 A 收敛 Wave 4 (`http_*` 第 2 批 15 目录 -> `cap:http`)**：将 `http_content_type` 至 `http_host_header` 共 15 个散装目录合并收敛至 `lib/http/{contenttype,cookiejar,cookiematch,csp,date,digest,digestauth,earlyhints,etaggen,expect,fetchmeta,form,forwarded,freshness,host}.cp` 并由 `lib/http/index.cp` 零破坏聚合导出（15/15 PASS），删除 15 个旧目录与旧文档。

## [2026-11-22] - 新建 `cap:net` 网络 Socket、IP/MAC/SocketAddr、PROXY Protocol v1/v2、Hosts 与 NoProxy 统一主包（5/5 PASS，自动动态挂载 `cap.net`）
- **方案 A 收敛 Wave 5 (7 网络目录 -> `cap:net`)**：将 `ip_addr`/`mac_addr`/`socket_addr`/`proxy_protocol_v1`/`proxy_protocol_v2`/`hosts_file`/`no_proxy` 7 个散装目录合并收敛至 `lib/net/{ipaddr,mac,sockaddr,proxyv1,proxyv2,hosts,noproxy}.cp` 并由 `lib/net/index.cp` 零破坏聚合导出（7/7 PASS），删除 7 个旧目录与旧文档。

## [2026-11-22] - 新建 `cap:json` JSON 解析、转义、压缩与 RFC 6901 JSON Pointer 统一主包（同步收敛 `cap:text`）（5/5 PASS，自动动态挂载 `cap.json`）
- **方案 A 收敛 Wave 4 (`json_*` -> `cap:json` & `text_*` -> `cap:text`)**：将 `json_escape`/`json_minify`/`json_pointer` 合入 `lib/json/{escape,minify,ptr}.cp`，将 `text_indent`/`text_wrap` 合入 `lib/text/{indent,wrap}.cp`（5/5 PASS），删除 5 个旧目录与旧文档。

## [2026-11-22] - 新建 `cap:git` Git 对象/仓库、Diff 统计、Ref 校验、URL 解析、GitIgnore 与 GitConfig 统一主包（5/5 PASS，自动动态挂载 `cap.git`）
- **方案 A 收敛 Wave 4 (5 Git 目录 -> `cap:git`)**：将 `git_diff_stat`、`git_ref_check`、`git_url_parse`、`gitignore_match`、`gitconfig_ini` 5 个散装目录合并收敛至 `lib/git/{diffstat,refcheck,url,ignorematch,config}.cp` 并由 `lib/git/index.cp` 零破坏聚合导出（5/5 PASS），删除 5 个旧目录与旧文档。

## [2026-11-22] - 新建 `cap:http` HTTP/1.1/2/3 协议与全套 RFC 9110/7838 头部协商、缓存与分片统一主包（5/5 PASS，自动动态挂载 `cap.http`）
- **方案 A 收敛 Wave 3 (`http_*` 第 1 批 15 目录 -> `cap:http`)**：将 `http_accept` 至 `http_content_range` 共 15 个散装目录合并收敛至 `lib/http/{accept,acceptenc,acceptlang,acceptranges,allow,altsvc,auth,cache,cachectrl,cachekey,clearsite,clienthints,contentenc,contentlang,contentrange}.cp` 并由 `lib/http/index.cp` 零破坏聚合导出（15/15 PASS），删除 15 个旧目录与旧文档。

## [2026-11-22] - 新建 `cap:sql` SQL 词法/语法 AST、连接 URI、转义、参数占位符与多语句切分统一主包（5/5 PASS，自动动态挂载 `cap.sql`）
- **方案 A 收敛 Wave 3 (`sql_*` -> `cap:sql`)**：将全部 5 个 `lib/sql_*` 散装目录合并收敛至 `lib/sql/{ast,lexer,parser,analyzer,uri,escape,params,splitter}.cp` 并由 `lib/sql/index.cp` 零破坏聚合导出（5/5 PASS），删除 5 个旧目录与旧文档。

