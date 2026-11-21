# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

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

