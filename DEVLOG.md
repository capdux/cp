# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:bencode` 纯 CP BEP-3 BitTorrent Bencode 严格编解码、规范化字典排序与零拷贝 InfoHash SHA-1 引擎（支持 `cap.bencode` 自动动态挂载，详见 `doc/lib/bencode.md`）（5/5 PASS，文档直写 `doc/lib/bencode.md`）
- **核心设计**：解码端在分配容器前先校验整数规范性与字典严格字典序，`extractRawSection` 直接在原始字节流跳过标量与嵌套结构定位 `"info"` 原始区间，确保 `info_hash` 计算零 AST 重编码偏差。

## 2026-11-21: 新建 `cap:websocket_frame` 纯 CP RFC 6455 WebSocket 帧协议、XOR 掩码、分片重组与控制帧交织引擎（支持 `cap.websocket_frame` 自动动态挂载，详见 `doc/lib/websocket_frame.md`）（5/5 PASS，文档直写 `doc/lib/websocket_frame.md`）
- **核心设计**：零堆分配计算 `Sec-WebSocket-Accept` 28 字节指纹；`WsStreamAssembler` 严格遵循 RFC 6455 §5.4 允许分片消息中途插入 Ping/Pong/Close 控制帧，并在组装完毕后执行完整 UTF-8 与 Close 状态码校验，全路径零泄漏。

## 2026-11-21: 新建 `cap:oauth2` 纯 CP RFC 6749/7636 OAuth 2.0、PKCE S256 与 OIDC 协议引擎（支持 `cap.oauth2` 自动动态挂载，详见 `doc/lib/oauth2.md`）（5/5 PASS，文档直写 `doc/lib/oauth2.md`）
- **核心设计**：使用 `getrandom` 拒绝采样消除 PKCE 生成模偏差，以常量时间比对防范 `state`/Verifier 时序侧信道；`TokenResponse` 兼容标准令牌与 OAuth2 错误 JSON 解析并在全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:histogram` 纯 CP HDR 延迟直方图与分位统计引擎（5/5 PASS，文档直写 `doc/lib/histogram.md`）
- **核心设计**：采用对数-线性子分桶在微秒至小时跨度内保持 O(log N) 空间复杂度，并支持 Coordinated Omission 补偿与跨线程直方图无损 `merge`；`SlidingWindowCounter` 仅在初始化时单次分配环形槽位，热路径零堆分配。

## 2026-11-21: 新建 `cap:unicode` 纯 CP UTF-8/UTF-16、UAX #29 Grapheme 与终端显示宽度引擎（5/5 PASS，文档直写 `doc/lib/unicode.md`）
- **核心设计**：零外部 `uucode` C/Zig 构表依赖，纯 CP 实现 UAX #29 簇边界状态机与 East Asian Width/Emoji 宽度表；`displayWidth` 支持跳过 ANSI CSI/OSC 控制序列，`truncateByWidth` 确保不切碎多字节与组合 Emoji 簇。

## 2026-11-21: 新建 `cap:picohttp` 纯 CP 零堆分配 HTTP/1.1 & Chunked 流式解析引擎（5/5 PASS，文档直写 `doc/lib/picohttp.md`）
- **核心设计**：零 C 绑定、零堆分配完成 HTTP/1.1 Pipelining 偏移解析与严格 `tchar`/CRLF 注入防护；`ChunkedDecoder` 原地紧缩与非破坏性流式双模式兼顾高性能与低内存。

## 2026-11-21: 新建 `cap:interchange` 纯 CP JSON/JSONC/JSON5/TOML/YAML 统一数据交换引擎（5/5 PASS，文档直写 `doc/lib/interchange.md`）
- **核心设计**：以独立拥有的 `InterchangeValue` 树为中间表示，JSON/JSONC/JSON5 由原生纯 CP 状态机解析（支持注释、单引号、未引号键、`Infinity`/`NaN` 与代理对转义），TOML/YAML 复用现有 `cap:toml`/`cap:yaml` 后深拷贝并立即释放原文档，实现零泄漏格式互转。

## 2026-11-21: 新建 `cap:valkey` 纯 CP RESP2/RESP3 Valkey/Redis 协议编解码与增量流式解析引擎（5/5 PASS，文档直写 `doc/lib/valkey.md`）
- **核心设计**：剔除所有 Bun/JSValue 耦合，基于 `std.mem.Allocator` 实现可自释放 `RespValue` 树；`RespStreamParser` 在预分配前先校验长度与嵌套深度上限，支持 `1..7` 字节切片连续投喂并精准区分 `Incomplete` 与协议异常。

