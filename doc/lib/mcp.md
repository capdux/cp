# `cap:mcp` — Model Context Protocol (MCP 2025-03-26) 标准库

## 1. 概述

`cap:mcp`（及动态挂载 `cap.mcp`）是纯 CP 实现的工业级 Model Context Protocol (`2025-03-26`) 协议栈与 Stdio 增量分帧引擎，包含三层核心能力：
- **协议编解码 (`protocol.cp`)**：JSON-RPC 2.0 请求/响应/通知模型，支持字符串与整数 `id`、RFC 8259 字符串转义/反转义及标准错误码。
- **增量传输分帧 (`framer.cp`)**：`McpStreamFramer` 支持 `Ndjson` 与 LSP 风格 `Content-Length: N\r\n\r\n` 双模式，支持任意 `1..7` 字节半包/黏包跨 chunk `feed()` 拆帧与 `max_frame_bytes` 上限拦截。
- **能力路由器 (`server.cp`)**：`McpServer` 内置 `initialize`、`notifications/initialized`、`ping`、`tools/list`、`tools/call`、`resources/list`、`resources/read`、`prompts/list`、`prompts/get` 路由分发，并自动将工具执行错误封装为 `isError: true`。
- **零堆分配工具注册表与权限治理 (`registry.cp`)**：`McpToolRegistry` 基于调用方固定槽位缓冲实现工具注册、能力风险分级（`ReadOnly`/`WriteLocal`/`Network`/`Destructive`）、网络 ACL、高危人工审批门槛（`RequireApproval`）、固定 60 秒窗口限流与动态 `tools/list` 可见集过滤。
- **零堆分配会话生命周期与请求追踪器 (`session.cp`)**：`McpSession` 提供协议版本解析/格式化与兼容性降级协商（`V2024_11_05` / `V2025_03_26`）、能力位掩码交集协商（`McpCapabilityFlags`）、严格五态生命周期门控（`Disconnected` -> `Initializing` -> `Ready` -> `Closing` -> `Closed`）及固定槽位 JSON-RPC 2.0 在途请求追踪（注册、完成、取消、批量超时回收与统计）。
- **零堆分配资源 URI 模板路由与订阅分发器 (`resource_router.cp`)**：`McpResourceRouter` 与 `matchUriTemplate` 提供 RFC 6570 简单 `{var}` 路径段零拷贝参数提取、静态路由优先于模板路由的分级解析、幂等去重客户端资源订阅/退订以及变更广播订阅者扇出收集。
- **零堆分配反向采样模型路由、递归守卫与进度追踪器 (`sampling.cp`)**：提供 MCP 2025-03-26 `sampling/createMessage` 三维偏好（`cost` / `speed` / `intelligence`）加权模型路由（`selectSamplingModel`）、嵌套采样递归深度与单树 Token 总预算硬拦截守卫（`SamplingDepthGuard`），以及固定槽位 `notifications/progress` 单调进度令牌追踪器（`McpProgressTracker`）。

## 2. 核心类型

- **常量**：`MCP_PROTOCOL_VERSION` (`"2025-03-26"`)、`JSONRPC_PARSE_ERROR` (`-32700`)、`JSONRPC_INVALID_REQUEST` (`-32600`)、`JSONRPC_METHOD_NOT_FOUND` (`-32601`)、`JSONRPC_INVALID_PARAMS` (`-32602`)、`JSONRPC_INTERNAL_ERROR` (`-32603`)
- **协议模型**：`McpError`、`McpCapabilities`、`McpToolDef`、`McpResourceDef`、`McpPromptDef`、`McpContent`、`McpToolResult`、`McpIdKind`、`McpId`、`McpRequest`、`McpResponse`、`McpNotification`
- **分帧器**：`FrameMode` (`Ndjson` | `ContentLength`)、`McpStreamFramer`
- **服务端**：`ToolHandler`、`ResourceReader`、`PromptBuilder`、`McpServer`
- **工具治理注册表 (`registry.cp`)**：`ToolRiskLevel`、`ToolPolicy`、`PolicyDecision`、`RegisteredTool`、`McpToolRegistry`
- **会话状态机与请求追踪 (`session.cp`)**：`McpProtocolVersion`、`McpCapabilityFlags`、`McpSessionState`、`McpSessionOpError`、`McpInflightRequest`、`McpSessionStats`、`McpSession`
- **资源路由与订阅分发 (`resource_router.cp`)**：`UriParamCapture`、`McpResourceRoute`、`McpResourceMatch`、`McpSubscription`、`McpResourceRouterStats`、`McpResourceRouter`
- **反向采样路由、递归守卫与进度追踪 (`sampling.cp`)**：`ModelPreferences`、`ModelCandidate`、`SamplingDepthGuard`、`ProgressEntry`、`ProgressUpdateStatus`、`McpProgressTracker`

## 3. API 速查

| 符号 | 签名/说明 |
| :--- | :--- |
| `decodeRequest(alloc, json_bytes)` | 解析并校验 JSON-RPC 2.0 请求/通知，返回 `!McpRequest` |
| `encodeResponse(alloc, resp)` | 编码 JSON-RPC 2.0 成功或错误响应，返回 `![]u8` |
| `encodeErrorResponse(alloc, id, code, message, data)` | 编码标准 JSON-RPC 错误响应，返回 `![]u8` |
| `encodeNotification(alloc, notif)` | 编码 JSON-RPC 2.0 通知报文，返回 `![]u8` |
| `McpStreamFramer.init(alloc, mode, max_frame_bytes)` | 初始化增量分帧器 |
| `framer.feed(chunk)` | 喂入任意长度字节切片并提取完整载荷帧，超限返回 `error.FrameTooLarge` |
| `McpStreamFramer.encodeFrame(alloc, mode, payload)` | 按 `Ndjson` 或 `ContentLength` 模式封装输出帧 |
| `McpServer.init(alloc, name, version)` | 创建 MCP 服务端实例 |
| `server.registerTool(def, handler)` | 注册工具定义与处理函数 |
| `server.registerResource(def, reader)` | 注册资源定义与读取函数 |
| `server.registerPrompt(def, builder)` | 注册提示词模板与构造函数 |
| `server.handleMessage(raw_json)` | 路由单条 JSON-RPC 报文，返回 `!([ ]u8)?`（通知返回 `null`） |
| `McpToolRegistry.init(slots)` | 使用调用方固定 `RegisteredTool` 槽位缓冲初始化零堆分配注册表 |
| `reg.register(name, desc, risk, requires_network, rate_limit_per_min)` | 注册工具（拒绝空名、重名与池满），`rate_limit_per_min == 0` 表示不限流 |
| `reg.findIndex(name)` / `reg.get(index)` | 按名称查找槽位索引 / 读取只读副本 |
| `reg.setEnabled(name, enabled)` | 动态启用或禁用指定工具 |
| `reg.authorizeAndConsume(name, policy, now_ms)` | 综合校验存在性、启用状态、网络 ACL、风险分级、高危人工审批与 60s 速率限制，仅 `Allow` 时累加调用计数 |
| `reg.recordError(name)` | 累加指定工具的错误计数 |
| `reg.listAllowedTools(policy, out_indices)` | 按 `ToolPolicy` 过滤出可见工具索引，用于动态构造 `tools/list` |
| `parseMcpVersion(raw)` / `formatMcpVersion(ver)` | 在字符串（`"2024-11-05"` / `"2025-03-26"`）与 `McpProtocolVersion` 间互转 |
| `negotiateMcpVersion(client_ver, server_max_ver)` | 协商双方兼容的最高协议版本，任一方 `Unknown` 返回 `null` |
| `negotiateCapabilities(client_caps, server_caps)` | 计算客户端与服务端 `McpCapabilityFlags` 位掩码交集并屏蔽未知高位 |
| `hashMethodName(method)` | 计算 MCP JSON-RPC 方法名的确定性 64 位 FNV-1a 哈希 |
| `McpSession.init(slots)` | 使用调用方固定 `McpInflightRequest` 槽位缓冲初始化 `Disconnected` 会话 |
| `sess.beginInitialize(cli_ver, srv_ver, cli_caps, srv_caps)` | `Disconnected -> Initializing`，校验版本与能力协商，返回 `McpSessionOpError?` |
| `sess.markReady()` / `sess.beginClose()` / `sess.finishClose()` / `sess.forceClose()` / `sess.reset()` | 严格执行会话状态机转换与在途槽位清理 |
| `sess.checkMethodAllowed(method)` / `sess.canDispatchMethod(method)` | 按当前生命周期状态与协商能力门控 JSON-RPC 方法（如未 `Ready` 拒绝 `tools/call`） |
| `sess.beginRequest(id, method_hash, sent_ms, timeout_ms)` / `sess.beginMethodRequest(...)` | 注册在途请求（拒绝非 `Ready`、0 ID、重复 ID 与满槽） |
| `sess.completeRequest(id)` / `sess.cancelRequest(id)` / `sess.purgeCancelled(out_ids)` | 完成并释放请求（返回副本供识别迟到已取消响应）、标记取消与批量清理已取消槽位 |
| `sess.collectTimedOut(now_ms, out_ids)` | 批量回收超时在途请求（支持 `timeout_ms == 0` 豁免、时钟回拨防护与输出截断安全） |
| `hashResourceUri(uri)` | 计算 MCP 资源 URI 的确定性 64 位 FNV-1a 哈希 |
| `isValidUriTemplate(template)` / `isStaticUriTemplate(template)` | 校验 URI 模板合法性 / 判定是否为无变量静态 URI |
| `matchUriTemplate(template, uri, out_params)` | RFC 6570 简单 `{var}` 路径段零拷贝匹配与参数提取（不匹配返回 `null`，输出截断安全） |
| `McpResourceRouter.init(routes, subscriptions)` | 使用调用方固定路由表与订阅表槽位初始化零堆分配资源路由器 |
| `router.registerTemplate(route_id, uri_template, mime_type, read_only)` | 注册静态 URI 或模板路由（拒绝 `route_id == 0`、非法模板、重复 ID/模板与满槽） |
| `router.unregisterRoute(route_id)` / `router.getRoute(slot_index)` | 按 `route_id` 注销路由释放槽位 / 按槽位索引获取只读副本 |
| `router.resolveUri(uri, out_params)` | 解析资源 URI（严格优先精确静态路由，未命中时再按槽位顺序匹配模板路由） |
| `router.subscribe(client_id, uri)` / `router.unsubscribe(client_id, uri)` | 客户端资源变更订阅（幂等去重）与退订 |
| `router.unsubscribeClient(client_id)` | 批量清理指定客户端的全部订阅（客户端断连回收） |
| `router.collectSubscribers(uri, out_client_ids)` | 收集指定 URI 的全部订阅客户端 ID 用于变更广播扇出（输出截断安全） |
| `router.stats()` | 返回路由/订阅槽位占用与解析命中/未命中/广播累计统计快照 `McpResourceRouterStats` |
| `isValidUnitWeight(v)` | 校验权重/分值是否为 `[0.0, 1.0]` 内的有限浮点数（拦截 `NaN`、`±Inf` 与越界值） |
| `scoreSamplingCandidate(prefs, candidate)` | 按三维偏好计算单个候选模型的归一化加权得分（`f32?`），全 0 权重回退三维均权 |
| `selectSamplingModel(prefs, min_tokens, candidates)` | 按三维偏好选出满足 `available && max_tokens >= min_tokens` 的最优 `model_id`（`u16?`），拒绝非法权重 |
| `SamplingDepthGuard.init(max_depth, max_tokens_per_tree)` | 初始化嵌套反向采样深度与单树 Token 总预算硬拦截守卫 |
| `guard.tryEnter(requested_tokens)` / `guard.exit()` | 原子校验并进入一层采样（防止无限递归与 Token 爆炸，防 `u32` 溢出）/ 退出一层（下溢安全） |
| `guard.remainingTokens()` / `guard.remainingDepth()` / `guard.refundTokens(n)` / `guard.reset()` | 查询剩余 Token 预算与深度、退回未用预估 Token 与重置守卫状态 |
| `McpProgressTracker.init(slots)` | 使用调用方固定 `ProgressEntry` 槽位缓冲初始化进度追踪器（安全支持零长度缓冲） |
| `tracker.register(token_id, total)` / `tracker.get(token_id)` | 注册进度令牌（拒绝 `token_id == 0`、重复注册与满槽，`total == 0` 表示不定长）/ 读取副本 |
| `tracker.update(token_id, progress, total)` | 严格单调推进进度（拒绝回退/重复值与超出 `total`，达到 `total` 自动标记 `Completed`） |
| `tracker.complete(token_id)` / `tracker.percent(token_id)` | 显式标记完成（支持不定长任务）/ 计算 `[0.0, 100.0]` 完成百分比 |
| `tracker.remove(token_id)` / `tracker.purgeCompleted()` | 释放指定槽位 / 批量回收全部已完成槽位供后续注册复用 |

## 4. 示例

```cp
const std = @import("std");
import { McpServer, McpToolDef, McpContent, McpToolResult } from "cap:mcp";

fn pingTool(alloc: std.mem.Allocator, _: []const u8): anyerror!McpToolResult {
    const items = try alloc.alloc(McpContent, 1);
    items[0] = McpContent.makeText(try alloc.dupe(u8, "pong"));
    return McpToolResult.ok(items);
}

pub fn example(alloc: std.mem.Allocator): !void {
    let srv = McpServer.init(alloc, "demo-agent", "1.0.0");
    defer srv.deinit();
    try srv.registerTool(McpToolDef{ .name = "ping", .description = "Ping tool" }, pingTool);
    if (resp := try srv.handleMessage("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}")) {
        alloc.free(resp);
    }
}
```

### AI Agent 工具权限治理与限流示例 (`registry.cp`)

```cp
import {
    ToolRiskLevel,
    ToolPolicy,
    PolicyDecision,
    RegisteredTool,
    McpToolRegistry,
} from "cap:mcp";

pub fn demoAgentToolGovernance() {
    let slots_buf: [8]RegisteredTool = [RegisteredTool{}; 8];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    * = reg.register("read_file", "Read workspace file", ToolRiskLevel.ReadOnly, false, 60);
    * = reg.register("web_fetch", "Fetch external URL", ToolRiskLevel.Network, true, 10);
    * = reg.register("drop_table", "Drop SQL table", ToolRiskLevel.Destructive, false, 2);

    // 沙箱 Agent 策略：仅允许本地只读，禁止网络
    const sandbox_policy = ToolPolicy{
        max_risk: ToolRiskLevel.ReadOnly,
        require_approval_for_destructive: true,
        allow_network: false,
    };

    const d1 = reg.authorizeAndConsume("read_file", sandbox_policy, 1000); // PolicyDecision.Allow
    const d2 = reg.authorizeAndConsume("web_fetch", sandbox_policy, 1000); // PolicyDecision.DenyNetwork
    const d3 = reg.authorizeAndConsume("drop_table", sandbox_policy, 1000); // PolicyDecision.DenyRisk

    // 动态过滤当前 Agent 可见的 tools/list
    let visible_buf: [8]usize = [0; 8];
    let visible: []usize = visible_buf[0..];
    const visible_count = reg.listAllowedTools(sandbox_policy, visible);
    * = d1;
    * = d2;
    * = d3;
    * = visible_count;
}
```

### MCP 会话状态机、能力协商与在途请求追踪示例 (`session.cp`)

```cp
import {
    McpProtocolVersion,
    McpCapabilityFlags,
    McpInflightRequest,
    McpSession,
} from "cap:mcp";

pub fn demoMcpSessionLifecycle() {
    let slots_buf: [8]McpInflightRequest = [McpInflightRequest{}; 8];
    let slots: []McpInflightRequest = slots_buf[0..];
    let sess = McpSession.init(slots);

    const cli_caps = McpCapabilityFlags.fromBits(
        McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES,
    );
    const srv_caps = McpCapabilityFlags.fromBits(
        McpCapabilityFlags.TOOLS | McpCapabilityFlags.PROMPTS,
    );

    // 1. 握手协商：Disconnected -> Initializing -> Ready
    * = sess.beginInitialize(
        McpProtocolVersion.V2025_03_26,
        McpProtocolVersion.V2025_03_26,
        cli_caps,
        srv_caps,
    );
    * = sess.markReady();

    // 2. 注册并追踪在途请求（自动校验 Ready 状态与 TOOLS 能力）
    * = sess.beginMethodRequest(1, "tools/call", 1000, 5000);
    * = sess.beginMethodRequest(2, "tools/list", 1200, 1000);

    // 3. 完成正常响应，并批量回收超时请求
    * = sess.completeRequest(1);
    let timed_out_buf: [4]u64 = [0; 4];
    let timed_out: []u64 = timed_out_buf[0..];
    const expired_count = sess.collectTimedOut(2500, timed_out); // 回收请求 2
    * = expired_count;
}
```

### MCP 资源 URI 模板路由与变更订阅广播示例 (`resource_router.cp`)

```cp
import {
    UriParamCapture,
    McpResourceRoute,
    McpSubscription,
    McpResourceRouter,
} from "cap:mcp";

pub fn demoMcpResourceRouter() {
    let route_buf: [8]McpResourceRoute = [McpResourceRoute{}; 8];
    let sub_buf: [16]McpSubscription = [McpSubscription{}; 16];
    let routes: []McpResourceRoute = route_buf[0..];
    let subs: []McpSubscription = sub_buf[0..];
    let router = McpResourceRouter.init(routes, subs);

    // 注册模板路由与精确静态路由（静态路由自动优先于模板路由）
    * = router.registerTemplate(10, "db://{schema}/{table}", "application/json", true);
    * = router.registerTemplate(20, "db://public/users", "application/vnd.mcp.users+json", false);

    // 解析 URI 并零拷贝提取模板变量
    let params_buf: [4]UriParamCapture = [UriParamCapture{}; 4];
    let params: []UriParamCapture = params_buf[0..];
    if (hit := router.resolveUri("db://analytics/events", params)) {
        // hit.route_id == 10, hit.param_count == 2 (schema="analytics", table="events")
        * = hit;
    }

    // 多客户端幂等订阅与变更广播扇出
    * = router.subscribe(101, "db://public/users");
    * = router.subscribe(102, "db://public/users");
    let targets_buf: [8]u32 = [0; 8];
    let targets: []u32 = targets_buf[0..];
    const fanout_count = router.collectSubscribers("db://public/users", targets); // 2
    * = fanout_count;
}
```

### MCP 反向采样模型路由、递归深度守卫与进度令牌追踪示例 (`sampling.cp`)

```cp
import {
    ModelPreferences,
    ModelCandidate,
    selectSamplingModel,
    SamplingDepthGuard,
    ProgressEntry,
    McpProgressTracker,
} from "cap:mcp";

pub fn demoMcpSamplingAndProgress() {
    // 1. 三维偏好模型打分路由
    const candidates: [2]ModelCandidate = [
        ModelCandidate{
            model_id: 1,
            cost_score: 0.95,
            speed_score: 0.90,
            intelligence_score: 0.60,
            max_tokens: 8192,
            available: true,
        },
        ModelCandidate{
            model_id: 2,
            cost_score: 0.35,
            speed_score: 0.50,
            intelligence_score: 0.98,
            max_tokens: 32768,
            available: true,
        },
    ];
    const smart_prefs = ModelPreferences{
        cost_priority: 0.1,
        speed_priority: 0.1,
        intelligence_priority: 0.9,
    };
    const chosen = selectSamplingModel(smart_prefs, 4096, candidates[0..]); // 2

    // 2. 嵌套采样递归深度与单树 Token 总预算守卫
    let guard = SamplingDepthGuard.init(3, 16000);
    if (guard.tryEnter(4000)) {
        // 剩余预算 12000，当前深度 1
        guard.exit();
    }

    // 3. 固定槽位单调进度追踪与槽位回收
    let slots_buf: [4]ProgressEntry = [ProgressEntry{}; 4];
    let slots: []ProgressEntry = slots_buf[0..];
    let tracker = McpProgressTracker.init(slots);
    * = tracker.register(1001, 100);
    * = tracker.update(1001, 40, null);
    const pct = tracker.percent(1001); // 40.0
    * = tracker.update(1001, 100, null); // 自动 Completed
    const reclaimed = tracker.purgeCompleted(); // 回收 1 个槽位

    * = chosen;
    * = pct;
    * = reclaimed;
}
```
