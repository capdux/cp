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

