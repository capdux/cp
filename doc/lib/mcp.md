# `cap:mcp` — Model Context Protocol (MCP 2025-03-26) 标准库

## 1. 概述

`cap:mcp`（及动态挂载 `cap.mcp`）是纯 CP 实现的工业级 Model Context Protocol (`2025-03-26`) 协议栈与 Stdio 增量分帧引擎，包含三层核心能力：
- **协议编解码 (`protocol.cp`)**：JSON-RPC 2.0 请求/响应/通知模型，支持字符串与整数 `id`、RFC 8259 字符串转义/反转义及标准错误码。
- **增量传输分帧 (`framer.cp`)**：`McpStreamFramer` 支持 `Ndjson` 与 LSP 风格 `Content-Length: N\r\n\r\n` 双模式，支持任意 `1..7` 字节半包/黏包跨 chunk `feed()` 拆帧与 `max_frame_bytes` 上限拦截。
- **能力路由器 (`server.cp`)**：`McpServer` 内置 `initialize`、`notifications/initialized`、`ping`、`tools/list`、`tools/call`、`resources/list`、`resources/read`、`prompts/list`、`prompts/get` 路由分发，并自动将工具执行错误封装为 `isError: true`。
- **零堆分配工具注册表与权限治理 (`registry.cp`)**：`McpToolRegistry` 基于调用方固定槽位缓冲实现工具注册、能力风险分级（`ReadOnly`/`WriteLocal`/`Network`/`Destructive`）、网络 ACL、高危人工审批门槛（`RequireApproval`）、固定 60 秒窗口限流与动态 `tools/list` 可见集过滤。
- **零堆分配会话生命周期与请求追踪器 (`session.cp`)**：`McpSession` 提供协议版本解析/格式化与兼容性降级协商（`V2024_11_05` / `V2025_03_26`）、能力位掩码交集协商（`McpCapabilityFlags`）、严格五态生命周期门控（`Disconnected` -> `Initializing` -> `Ready` -> `Closing` -> `Closed`）及固定槽位 JSON-RPC 2.0 在途请求追踪（注册、完成、取消、批量超时回收与统计）。
- **零堆分配资源 URI 模板路由与订阅分发器 (`resRouter.cp`)**：`McpResourceRouter` 与 `matchUriTemplate` 提供 RFC 6570 简单 `{var}` 路径段零拷贝参数提取、静态路由优先于模板路由的分级解析、幂等去重客户端资源订阅/退订以及变更广播订阅者扇出收集。
- **零堆分配反向采样模型路由、递归守卫与进度追踪器 (`sampling.cp`)**：提供 MCP 2025-03-26 `sampling/createMessage` 三维偏好（`cost` / `speed` / `intelligence`）加权模型路由（`selectSamplingModel`）、嵌套采样递归深度与单树 Token 总预算硬拦截守卫（`SamplingDepthGuard`），以及固定槽位 `notifications/progress` 单调进度令牌追踪器（`McpProgressTracker`）。

## 2. 核心类型

- **常量**：`MCP_PROTOCOL_VERSION` (`"2025-03-26"`)、`JSONRPC_PARSE_ERROR` (`-32700`)、`JSONRPC_INVALID_REQUEST` (`-32600`)、`JSONRPC_METHOD_NOT_FOUND` (`-32601`)、`JSONRPC_INVALID_PARAMS` (`-32602`)、`JSONRPC_INTERNAL_ERROR` (`-32603`)
- **协议模型**：`McpError`、`McpCapabilities`、`McpToolDef`、`McpResourceDef`、`McpPromptDef`、`McpContent`、`McpToolResult`、`McpIdKind`、`McpId`、`McpRequest`、`McpResponse`、`McpNotification`
- **分帧器**：`FrameMode` (`Ndjson` | `ContentLength`)、`McpStreamFramer`
- **服务端**：`ToolHandler`、`ResourceReader`、`PromptBuilder`、`McpServer`
- **工具治理注册表 (`registry.cp`)**：`ToolRiskLevel`、`ToolPolicy`、`PolicyDecision`、`RegisteredTool`、`McpToolRegistry`
- **会话状态机与请求追踪 (`session.cp`)**：`McpProtocolVersion`、`McpCapabilityFlags`、`McpSessionState`、`McpSessionOpError`、`McpInflightRequest`、`McpSessionStats`、`McpSession`
- **资源路由与订阅分发 (`resRouter.cp`)**：`UriParamCapture`、`McpResourceRoute`、`McpResourceMatch`、`McpSubscription`、`McpResourceRouterStats`、`McpResourceRouter`
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

### MCP 资源 URI 模板路由与变更订阅广播示例 (`resRouter.cp`)

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

---

## 九、 生产级 MCP 工具目录、策略门控分发与 `CallToolResult` 构造器 (`dispatch.cp`)

面向 AI Agent MCP Tool Server 的 100% 零堆分配工具注册、策略路由与标准结果响应构造引擎（30 行内构建类型安全、可观测 MCP 工具服务）：

- **工具目录与 `tools/list` 发射（`McpToolAccess` / `McpToolSpec` / `McpToolCatalog`）**：
  - `McpToolAccess`：`ReadOnly | Mutating | ApprovalRequired`
  - `McpToolCatalog.init(slots)` / `register(*_, tool_id, name, description, required_params_csv, access, max_concurrency)`（自动防重名、防重复 ID 与满槽保护）/ `unregister` / `findByName` / `count` / `capacity`
  - `renderToolsListJson(out_buf: *[]u8): usize?`：零堆分配输出标准 MCP `{"tools":[...]}` JSON 列表（完整 RFC 8259 转义，缓冲不足返回 `null`）。
- **策略门控分发器（`McpCallRequest` / `McpDispatchStatus` / `McpDispatchDecision` / `McpToolDispatcher`）**：
  - `dispatch(*_, catalog: *McpToolCatalog, req: McpCallRequest): McpDispatchDecision`：依次校验工具存在性（`ToolNotFound`）、只读模式拦截（`ReadOnlyBlocked`）、人工审批门控（`ApprovalRequired`）、JSON 对象参数格式（`InvalidArguments`）、顶层必填参数齐全性（`MissingRequiredParam`，精确返回缺失字段名 `missing_param`）与单工具并发上限（`ConcurrencyLimitReached`），放行后原子递增 `inflight` 与 `call_count`。
  - `finishCall(catalog, slot_index, is_error)`：释放并发槽位并累计 `error_count`。
- **零堆分配 `CallToolResult` 构造器（`McpToolResultBuilder`）**：
  - `init(out_buf, is_error)` / `addText(*_, text)` / `addResourceText(*_, uri, mime_type, text)` / `finish(*_): usize?`
  - 便捷静态构造：`buildTextResult(text, is_error, out_buf)` / `buildErrorResult(message, out_buf)`。

```cp
import {
    McpToolAccess,
    McpToolSpec,
    McpToolCatalog,
    McpCallRequest,
    McpDispatchStatus,
    McpToolDispatcher,
    McpToolResultBuilder,
} from "cap:mcp";

pub fn demoMcpToolDispatch() {
    let slots_buf: [4]McpToolSpec = [McpToolSpec{}; 4];
    let slots_s: []McpToolSpec = slots_buf[0..];
    let catalog = McpToolCatalog.init(slots_s);
    * = catalog.register(1, "read_file", "Read workspace file", "path", McpToolAccess.ReadOnly, 4);
    * = catalog.register(2, "deploy_prod", "Deploy release", "version,env", McpToolAccess.ApprovalRequired, 1);

    let list_buf: [512]u8 = [0; 512];
    let list_s: []u8 = list_buf[0..];
    if (n_list := catalog.renderToolsListJson(list_s)) {
        * = list_buf[0..n_list];
    }

    let dispatcher = McpToolDispatcher.init(false);
    const dec = dispatcher.dispatch(&catalog, McpCallRequest{
        tool_name: "read_file",
        arguments_json: "{\"path\":\"README.md\"}",
        read_only_mode: true,
        approved: false,
    });

    let res_buf: [256]u8 = [0; 256];
    let res_s: []u8 = res_buf[0..];
    if (dec.status == McpDispatchStatus.Allowed) {
        if (n := McpToolResultBuilder.buildTextResult("file content\nline 2", false, res_s)) {
            * = res_buf[0..n];
        }
        * = dispatcher.finishCall(&catalog, dec.slot_index, false);
    }
}
```

---

## 十、 零堆分配 MCP 工具参数严格约束校验与 AI 友好错误提示器 (`argValid.cp`)

面向生产级 MCP Tool Server 的零拷贝顶层 JSON 参数校验器与 LLM 自愈提示渲染器：

- **声明式约束与违规报告**：
  - `McpArgType`：`Str | Int | Float | Bool | Array | Object`
  - `McpArgRule`：`name`、`kind`、`required`、`min_len` / `max_len`（字符串解码长度上下界）、`min_int` / `max_int` / `has_int_bounds`（整数闭合区间）、`enum_csv`（逗号分隔白名单，如 `"asc,desc"`）
  - `McpArgViolationKind`：`MalformedJson | MissingRequired | TypeMismatch | StringTooShort | StringTooLong | IntOutOfRange | EnumMismatch | DuplicateKey`
  - `McpArgViolation`、`McpArgValidationReport`
- **核心函数**：
  - `validateMcpToolArgs(args_json, rules, out_violations): McpArgValidationReport`：零堆分配单遍扫描 RFC 8259 顶层 JSON 对象键值对，自动跳过嵌套 `{...}` / `[...]` 防止内层同名键误判，完整校验必填性、类型匹配、字符串长度、整数区间、枚举白名单与重复键。
  - `formatMcpArgErrorHint(report, violations, out_buf): usize?`：零堆输出精准可读的 LLM 自愈修正提示（明确告知违规字段名与合法约束区间/白名单，缓冲不足返回 `null`）。

```cp
import {
    McpArgType,
    McpArgRule,
    McpArgViolation,
    validateMcpToolArgs,
    formatMcpArgErrorHint,
} from "cap:mcp";

pub fn demoMcpArgValidator() {
    const rules: [3]McpArgRule = [
        McpArgRule{ name: "query", kind: McpArgType.Str, required: true, min_len: 1, max_len: 64 },
        McpArgRule{ name: "limit", kind: McpArgType.Int, required: false, min_int: 1, max_int: 100, has_int_bounds: true },
        McpArgRule{ name: "order", kind: McpArgType.Str, required: false, enum_csv: "asc,desc" },
    ];
    let violations: [4]McpArgViolation = [McpArgViolation{}; 4];
    const report = validateMcpToolArgs(
        "{\"query\":\"cp stdlib\",\"limit\":250,\"order\":\"random\"}",
        rules[0..],
        violations[0..],
    );

    if (!report.valid) {
        let hint_buf: [256]u8 = [0; 256];
        if (n := formatMcpArgErrorHint(report, violations[0..], hint_buf[0..])) {
            // "Invalid tool arguments: field 'limit' out of range (allowed [1, 100]); field 'order' invalid value (allowed: asc,desc)"
            * = hint_buf[0..n];
        }
    }
}
```

---

## 十一、 零堆分配 MCP 协议版本/能力协商与资源订阅路由引擎 (`capNeg.cp`)

面向多版本 MCP（`2024-11-05` / `2025-03-26` / `2025-06-18`）服务端与多 Agent 客户端的 100% 纯 CP 零动态堆分配协议握手、能力位图交集协商、会话五态门禁与通配符资源订阅广播引擎：

- **能力位图常量 (`u32`)**：
  - `MCP_CAP_TOOLS` (`1<<0`)、`MCP_CAP_TOOLS_LIST_CHANGED` (`1<<1`)、`MCP_CAP_RESOURCES` (`1<<2`)、`MCP_CAP_RESOURCES_SUBSCRIBE` (`1<<3`)、`MCP_CAP_PROMPTS` (`1<<4`)、`MCP_CAP_LOGGING` (`1<<5`)、`MCP_CAP_SAMPLING` (`1<<6`)、`MCP_CAP_ROOTS` (`1<<7`)、`MCP_CAP_ELICITATION` (`1<<8`)、`MCP_CAP_ALL_KNOWN` (`0x1FF`)
- **协议版本与状态枚举**：
  - `McpNegotiatedVersion`：`unknown | v2024_11_05 | v2025_03_26 | v2025_06_18`（规避与 `session.cp` 中 `McpProtocolVersion` 冲突），内置 `parse(s)`、`toString()`、`rank()`、`isKnown()`、`compare(other)`、`isAtLeast(other)`、`isAtMost(other)`。
  - `McpSessionPhase`：`uninitialized | initializing | ready | closing | closed`。
- **会话能力协商器 (`McpSessionNegotiator`)**：
  - `McpSessionNegotiator.init(server_min_ver, server_max_ver, server_caps, required_client_caps)`
  - `negotiate(client_ver_str, client_caps): McpHandshakeResult`（与 `negotiate_request(req)`）：校验版本区间（高版本兼容降级至 `server_max_ver`，过旧/未知版本拒绝）与 `required_client_caps`（缺失位在 `missing_required_caps` 中精确报告），按位与计算交集 `effective_caps`，仅成功时推进 `uninitialized -> initializing`。
  - `mark_initialized(): bool`：仅 `initializing -> ready` 合法。
  - `can_invoke(required_cap_bit: u32): bool`：仅 `ready` 态且目标能力位已协商通过返回 `true`。
  - `begin_close(): bool` / `finish_close(): bool`：执行优雅关闭并在 `closed` 态清零 `effective_caps`。
- **零堆分配资源订阅路由器 (`McpResourceSubscriptionRegistry`)**：
  - 固定 `MCP_SUB_MAX_SLOTS = 64` 槽位与每槽 `MCP_SUB_MAX_URI_LEN = 128` 字节内联缓冲，零动态堆分配。
  - `subscribe(client_id, uri_pattern): bool`：支持精确 URI 与尾部 `*` 前缀通配（如 `file:///workspace/*`），幂等去重，拦截 `client_id == 0`、空/超长 URI 及非法中置 `*`。
  - `unsubscribe(client_id, uri_pattern): bool` / `unsubscribe_all(client_id): usize`：即时释放槽位供后续订阅复用。
  - `match_subscribers(changed_uri, out_client_ids): usize`：零堆匹配精确与通配符规则，并按 `client_id` 去重输出待推送通知列表。
  - `active_count(): usize`：返回当前活跃订阅槽位数。

```cp
import {
    MCP_CAP_TOOLS,
    MCP_CAP_RESOURCES,
    MCP_CAP_RESOURCES_SUBSCRIBE,
    MCP_CAP_ELICITATION,
    McpNegotiatedVersion,
    McpSessionNegotiator,
    McpResourceSubscriptionRegistry,
} from "cap:mcp";

pub fn demoMcpCapabilityNegotiator() {
    let neg = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2024_11_05,
        McpNegotiatedVersion.v2025_03_26,
        MCP_CAP_TOOLS | MCP_CAP_RESOURCES | MCP_CAP_RESOURCES_SUBSCRIBE,
        MCP_CAP_TOOLS,
    );
    // 客户端请求 2025-06-18 -> 自动兼容降级至 2025-03-26
    const res = neg.negotiate("2025-06-18", MCP_CAP_TOOLS | MCP_CAP_RESOURCES_SUBSCRIBE | MCP_CAP_ELICITATION);
    if (res.accepted && neg.mark_initialized() && neg.can_invoke(MCP_CAP_RESOURCES_SUBSCRIBE)) {
        let reg = McpResourceSubscriptionRegistry.init();
        * = reg.subscribe(1001, "file:///workspace/*");
        * = reg.subscribe(1001, "file:///workspace/main.cp");
        * = reg.subscribe(1002, "file:///workspace/main.cp");

        let targets: [8]u64 = [0; 8];
        let targets_s: []u64 = targets[0..];
        const n = reg.match_subscribers("file:///workspace/main.cp", targets_s);
        // n == 2 (client 1001 去重仅出现一次, client 1002 出现一次)
        * = n;
    }
}
```

---

## 十二、 零堆分配 MCP 在途请求并发门控、取消令牌注册表与超时清扫器 (`inflight.cp`)

面向高并发 MCP 服务端/客户端的 100% 纯 CP 零动态堆分配在途 JSON-RPC 请求生命周期、分方法并发限流、`notifications/cancelled` 取消令牌与超时清扫内核（固定 64 槽位内联池）：

- **常量与枚举**：
  - `MCP_GATE_MAX_INFLIGHT` (`64`)
  - `McpMethodClass`（`tools_call | resources_read | prompts_get | sampling_create | other`）
  - `McpInflightState`（`running | cancel_requested | completed | timed_out`）
- **数据模型**：`McpGateLimits`（`defaults()`, `isValid()`）、`McpAdmitOutcome`、`McpInflightSlot`、`McpGateStats`
- **核心引擎 `McpInflightGate`**：
  - `init(limits: McpGateLimits): McpInflightGate?`：校验总上限 `1..=64` 与分方法上限不超过总上限。
  - `admit_request(*_, req_id, batch_id, method_class, timeout_ms, now_ms): McpAdmitOutcome`：校验 `req_id != 0 && timeout_ms > 0`、活跃 ID 去重、总并发上限与分方法并发上限。
  - `request_cancel(*_, req_id): bool` / `is_canceled(req_id): bool` / `cancel_batch(*_, batch_id): usize`：幂等单请求取消与按 `batch_id` 批量取消。
  - `complete_request(*_, req_id): bool`：完成请求并立即释放总/分方法并发配额。
  - `sweep_expired(*_, now_ms, out_expired_ids: *[]u64): usize`：清扫 `now_ms >= deadline_ms` 的超时请求，输出缓冲满时自动暂停以支持分批清扫不丢数据。
  - `stats(): McpGateStats`：实时返回活跃总数、`tools_call` 活跃数、待取消数以及累计准入/完成/取消/超时/拒绝统计。

```cp
import {
    McpMethodClass,
    McpGateLimits,
    McpInflightGate,
} from "cap:mcp";

pub fn demoMcpInflightGate() {
    let limits = McpGateLimits.defaults();
    limits.max_total_inflight = 16;
    limits.max_tools_call = 4;
    let gate = McpInflightGate.init(limits) ?? return;

    * = gate.admit_request(101, 1, McpMethodClass.tools_call, 5_000, 1_000);
    * = gate.admit_request(102, 1, McpMethodClass.resources_read, 1_000, 1_000);

    // 响应 notifications/cancelled
    * = gate.request_cancel(101);
    * = gate.complete_request(101);

    // 清扫 t=2500 时已超时的请求 102
    let expired: [8]u64 = [0; 8];
    let exp_s: []u64 = expired[0..];
    const n_exp = gate.sweep_expired(2_500, exp_s);
    * = n_exp; // 1 (req_id == 102)
}
```

---

## 十三、 零堆分配 MCP 资源订阅注册表、RFC 6570 URI 模板匹配与增量变更扇出通知器 (`resSub.cp`)

面向 MCP 2025-03-26 `resources/subscribe` 与 `notifications/resources/updated` 协议的 100% 纯 CP 零动态堆分配资源订阅与变更扇出引擎：

- **常量与 RFC 6570 模板函数**：
  - `MCP_RSUB_MAX_SUBSCRIPTIONS = 128`、`MCP_RSUB_MAX_URI_LEN = 192`、`MCP_RSUB_MAX_TEMPLATE_PARAMS = 8`、`MCP_RSUB_MAX_PENDING_NOTIFS = 128`、`MCP_RSUB_DEFAULT_MAX_PER_SESSION = 32`
  - `validateRfc6570Template(template)`：校验非空、长度、控制符、花括号成对、禁止嵌套 `{` / 空 `{}` / 连续 `}{` 及合法变量字符（支持 `{var}` 单段与 `{+path}` 多段路径展开）。
  - `matchRfc6570Template(template, uri): McpTemplateMatchResult`：精确匹配静态段与 `{var}` / `{+var}` 占位符，返回零拷贝参数偏移数组 `McpTemplateParam`。
- **数据模型**：
  - `McpSubKind`（`exact_uri | uri_template`）、`McpSubStatus`（`ok | already_subscribed | invalid_session | invalid_uri | capacity_full | session_limit_reached | not_found`）
  - `McpSubEntry`、`McpPendingNotification`（含 `uriSlice()`、`etag_version`、`coalesced_count`）、`McpResourceNotifyReport`、`McpResourceSubStats`
- **核心引擎 `McpResourceSubHub`**：
  - `init(max_per_session): McpResourceSubHub?`：配置单会话订阅配额（`1..=128`）。
  - `subscribe(*_, subscriber_id, uri_or_template)` / `unsubscribe(*_, subscriber_id, uri_or_template)` / `unsubscribeAllForSession(*_, subscriber_id)`：支持精确 URI 与 RFC 6570 模板订阅、自动去重、单会话/全局配额限制与会话断开全量清理（含待发通知）。
  - `notifyResourceUpdated(*_, resource_uri, out_subscriber_ids: *[]u64): McpResourceNotifyReport`：单调递增 `etag_version`，扇出收集去重后的受影响订阅者 ID，并将同 `(subscriber_id, resource_uri)` 的重复脏标记原地去抖合并（更新最新 `etag_version` 与 `coalesced_count`）。
  - `drainPendingNotifications(*_, subscriber_id, out_notifs: *[]McpPendingNotification): usize`：支持全局（`0`）或按会话分批排空待发通知不丢数据。

```cp
import {
    McpPendingNotification,
    McpResourceSubHub,
} from "cap:mcp";

pub fn demoMcpResourceSubHub() {
    let hub = McpResourceSubHub.init(16) ?? return;
    * = hub.subscribe(101, "db://{tenant}/tables/{table}");
    * = hub.subscribe(102, "db://acme/tables/orders");

    let targets: [8]u64 = [0; 8];
    let targets_s: []u64 = targets[0..];
    const r1 = hub.notifyResourceUpdated("db://acme/tables/orders", *targets_s);
    const r2 = hub.notifyResourceUpdated("db://acme/tables/orders", *targets_s);
    * = r1;
    * = r2; // coalesced_notifications == 2, etag_version == 2

    let out: [4]McpPendingNotification = [McpPendingNotification{}; 4];
    let out_s: []McpPendingNotification = out[0..];
    const drained = hub.drainPendingNotifications(101, *out_s);
    * = drained; // 1 (etag_version == 2, coalesced_count == 2)
}
```


---

## 十四、 零堆分配 JSON-RPC 2.0 / MCP 批处理请求关联器与乱序响应重排引擎 (`batch.cp`)

面向 JSON-RPC 2.0 §6 与 MCP 2025-03-26 批处理请求的 100% 纯 CP 零动态堆分配关联器、乱序响应重排器与超时清扫聚合引擎：

- **常量与枚举**：
  - `MCP_BATCH_MAX_ITEMS = 64`、`MCP_BATCH_ERR_TIMEOUT = -32001`、`MCP_BATCH_ERR_INVALID_REQUEST = -32600`
  - `McpBatchEntryKind`（`rpc_request | notification`）、`McpBatchItemState`（`pending | succeeded | failed | timed_out | notification_done`）
  - `McpBatchSubmitStatus`（`ok | invalid_empty_batch | duplicate_request_id | capacity_full | batch_sealed`）
  - `McpBatchRecordStatus`（`ok | unknown_request_id | already_completed | batch_finalized`）
  - `McpBatchOrderMode`（`submission_order | completion_order`）
- **数据模型**：
  - `McpBatchItem`、`McpBatchResponseItem`、`McpBatchSummary`（含 `avg_latency_us`、`min/max_latency_us`、`success_rate`、`requires_wire_response` 与拒绝遥测计数）
- **核心引擎 `McpBatchDispatcher`**：
  - `init(batch_id)` / `addRequest(*_, request_id, method_hash, submitted_at_ms)` / `addNotification(*_, method_hash, submitted_at_ms)` / `sealBatch(*_)`（空批按规范返回 `invalid_empty_batch`，全通知批自动封口完结且 `requiresWireResponse() == false`）。
  - `recordResult(*_, request_id, payload_hash, is_error, error_code, latency_us)`：支持异步乱序回传，自动拦截未知 ID、重复写入及已完结批次。
  - `expirePending(*_, now_ms, deadline_ms)`：批次截止清扫未决 RPC 并自动注入 `-32001` 超时错误。
  - `collectResponses(order, out_buf: *[]McpBatchResponseItem)`：按原始提交顺序或异步完成顺序重排输出（自动剔除通知条目）。
  - `requiresWireResponse()` / `isComplete()` / `getItemByRequestId(request_id)` / `summary()`

```cp
import {
    McpBatchOrderMode,
    McpBatchResponseItem,
    McpBatchDispatcher,
} from "cap:mcp";

pub fn demoMcpBatchDispatcher() {
    let batch = McpBatchDispatcher.init(9001);
    * = batch.addRequest(10, 0xA1, 1000);
    * = batch.addNotification(0xB1, 1000);
    * = batch.addRequest(20, 0xA2, 1000);
    * = batch.sealBatch();

    // 乱序回传：20 先完成，10 后完成
    * = batch.recordResult(20, 0x2222, false, 0, 150);
    * = batch.recordResult(10, 0x1111, false, 0, 350);

    // 按原始提交顺序重排输出：[10, 20]（自动跳过通知）
    let out: [4]McpBatchResponseItem = [McpBatchResponseItem{}; 4];
    let out_s: []McpBatchResponseItem = out[0..];
    const n = batch.collectResponses(McpBatchOrderMode.submission_order, *out_s);
    * = n; // 2
}
```


---

## 十五、 零堆分配 MCP Prompt 目录注册表、参数契约校验与前缀自动补全引擎 (`promptCatalog.cp`)

面向 MCP 2025-03-26 `prompts/list`、`prompts/get`、`completion/complete` 的 100% 纯 CP 零动态堆分配核心逻辑引擎：

- **常量与数据模型**：
  - `MCP_PROMPT_MAX_ENTRIES = 32`、`MCP_PROMPT_MAX_ARGS = 8`、`MCP_PROMPT_MAX_ENUM_CHOICES = 8`、`MCP_PROMPT_NAME_MAX_LEN = 64`、`MCP_PROMPT_DESC_MAX_LEN = 128`、`MCP_PROMPT_ARG_NAME_MAX_LEN = 48`、`MCP_PROMPT_CHOICE_MAX_LEN = 32`、`MCP_PROMPT_DEFAULT_MAX_VALUE_LEN = 1024`。
  - `McpPromptRegisterStatus`（`ok | empty_name | name_too_long | desc_too_long | duplicate_name | capacity_full | too_many_args | invalid_arg_spec`）。
  - `McpPromptArgSpec`（`plain(name, required)`、`withChoices(name, required, enforce_enum, choice_list)`）、`McpStoredPromptArg`、`McpPromptEntry`。
  - `McpPromptListPage`（`valid`, `count`, `total`, `has_more`, `next_cursor`）。
  - `McpPromptArgValue`、`McpPromptViolationKind`（`none | prompt_not_found | missing_required_arg | unknown_arg | duplicate_arg | value_too_long | invalid_enum_value`）、`McpPromptGetValidation`。
  - `McpCompletionCandidate`、`McpCompletionResult`（`found`, `count`, `total_matches`, `has_more`）。
- **核心引擎 `McpPromptCatalog`**：
  - `registerPrompt(*_, name, description, args)`：原子校验名称/描述长度、重复 Prompt、满容、>8 参数及参数契约合法性，内联零堆拷贝存储。
  - `listPrompts(cursor, limit, out_indices: *[]usize): McpPromptListPage`：游标分页，非法游标或 `limit == 0` 返回 `valid = false`。
  - `validatePromptGet(prompt_name, provided_args): McpPromptGetValidation`：一站式校验必填参数缺失、未知参数、重复参数、超长值与枚举越界，返回首个违规参数名与总违规数。
  - `completePromptArg(prompt_name, arg_name, prefix, out_candidates: *[]McpCompletionCandidate): McpCompletionResult`：大小写无关 ASCII 前缀匹配，自动设置截断 `has_more` 标志。

```cp
import {
    McpPromptArgSpec,
    McpPromptArgValue,
    McpCompletionCandidate,
    McpPromptCatalog,
} from "cap:mcp";

pub fn demoMcpPromptCatalog() {
    let cat = McpPromptCatalog.init();
    const langs: [3][]const u8 = ["rust", "ruby", "python"];
    const args: [2]McpPromptArgSpec = [
        McpPromptArgSpec.withChoices("lang", true, true, langs[0..]),
        McpPromptArgSpec.plain("topic", false),
    ];
    * = cat.registerPrompt("code_review", "Review code snippet", args[0..]);

    const vals: [1]McpPromptArgValue = [McpPromptArgValue{ name: "lang", value: "rust" }];
    const vr = cat.validatePromptGet("code_review", vals[0..]);
    * = vr.ok; // true

    let cands: [4]McpCompletionCandidate = [McpCompletionCandidate{}; 4];
    let cands_s: []McpCompletionCandidate = cands[0..];
    const cr = cat.completePromptArg("code_review", "lang", "RU", *cands_s);
    * = cr.count; // 2 ("rust", "ruby")
}
```


---

## 十六、 零堆分配 MCP RFC 5424 日志级别路由器与通知环形缓冲区 (`logRouter.cp`)

面向 MCP `logging/setLevel` 与 `notifications/message` 规范的 100% 纯 CP 零动态堆分配 8 级严重度过滤、Logger 前缀路由、日志风暴限流与环形通知缓冲引擎：

- **常量与 RFC 5424 级别编解码**：
  - `MCP_LOG_MAX_RULES = 32`、`MCP_LOG_MAX_RATE_TRACKERS = 32`、`MCP_LOG_RING_CAPACITY = 64`、`MCP_LOG_LOGGER_MAX_LEN = 64`、`MCP_LOG_MSG_MAX_LEN = 256`
  - `McpLogLevel`（`Debug < Info < Notice < Warning < Error < Critical < Alert < Emergency`）
  - `mcpLogLevelRank(level): u8`、`parseMcpLogLevel(name): McpLogLevel?`（大小写不敏感解析）、`formatMcpLogLevel(level): []const u8`（标准小写线缆名）
- **数据模型**：
  - `McpLogEmitStatus`（`enqueued | filtered_level | rate_limited | invalid_input`）
  - `McpLogRateLimitConfig`（`max_per_window`, `window_ms`；`unlimited()` / `isValid()`）
  - `McpLogPrefixRule`、`McpLogRateTracker`、`McpLogNotification`（`loggerSlice()` / `messageSlice()`）、`McpLogRouterStats`
- **核心引擎 `McpLogRouter`**：
  - `init(global_level, rate_cfg)` / `setGlobalLevel(*_, level)` / `setGlobalLevelByName(*_, level_name)`
  - `setLoggerPrefixLevel(*_, prefix, min_level)` / `removeLoggerPrefixLevel(*_, prefix)` / `effectiveLevel(logger)`：最长前缀匹配优先，严格按 `.` / `/` / `:` 分隔符边界匹配（`"db"` 不误匹配 `"debugger"`）。
  - `emitLog(*_, level, logger, message, now_ms)`：依次执行参数校验、级别过滤、每 Logger 窗口防风暴限流（含单调时钟回退防护）与 64 槽位环形缓冲入队（满环覆盖最旧并累计 `ring_overwrites`）。
  - `drainNotifications(*_, *out_notifs)` / `pendingCount()` / `stats()`：按 FIFO 分批排空待下发通知。

```cp
import {
    McpLogLevel,
    McpLogRateLimitConfig,
    McpLogNotification,
    McpLogRouter,
} from "cap:mcp";

pub fn demoMcpLogRouter() {
    const rl = McpLogRateLimitConfig{ max_per_window: 50, window_ms: 1_000 };
    let router = McpLogRouter.init(McpLogLevel.Warning, rl) ?? return;
    * = router.setLoggerPrefixLevel("db.pool", McpLogLevel.Debug);
    * = router.emitLog(McpLogLevel.Debug, "db.pool.conn", "checked out", 1_000);

    let out: [8]McpLogNotification = [McpLogNotification{}; 8];
    let out_s: []McpLogNotification = out[0..];
    const n = router.drainNotifications(*out_s);
    * = n; // 1
}
```


---

## 十七、 零堆分配 MCP 2025-03-26 进度令牌与竞态安全取消通知状态机 (`progress.cp`)

面向 MCP 2025-03-26 `notifications/progress` 与 `notifications/cancelled` 规范的 100% 纯 CP 零动态堆分配进度令牌表、节流防刷屏、竞态安全取消状态机与 JSON-RPC 2.0 载荷编解码器：

- **常量与枚举**：
  - `MCP_PROGRESS_MAX_SLOTS = 64`、`MCP_PROGRESS_MAX_TOMBSTONES = 64`、`MCP_PROGRESS_TOKEN_MAX_LEN = 64`、`MCP_PROGRESS_MSG_MAX_LEN = 128`、`MCP_CANCEL_REASON_MAX_LEN = 128`
  - `McpProgressTokenKind`（`int_token | str_token`）、`McpProgressState`（`active | completed | cancelled | timed_out`）
  - `McpProgressReportStatus`（`emitted | throttled | non_monotonic | not_found | already_terminal | invalid_value`）
  - `McpCancelOutcome`（`cancelled_active | recorded_early_cancel | already_cancelled | already_completed | invalid_or_full`）
- **数据模型与零分配 JSON-RPC 2.0 编解码**：
  - `McpProgressToken`（`fromInt`, `fromStr`, `strSlice`, `equals`）、`McpProgressSlot`（`messageSlice`, `cancelReasonSlice`, `percent`）、`McpProgressHubStats`
  - `McpParsedProgressNotif` / `McpParsedCancelNotif`
  - `encodeProgressNotification` / `parseProgressNotification` / `encodeCancelledNotification` / `parseCancelledNotification`
- **核心状态机 `McpProgressHub`**：
  - `init(default_min_interval_ms, early_cancel_ttl_ms)`
  - `register(*_, token, request_id, total, min_interval_ms, timeout_ms, now_ms)`：绑定令牌与 `request_id`；若存在提前到达的取消墓碑，立即消费墓碑并置为 `cancelled` 防止启动无效任务。
  - `reportProgress(*_, token, progress, total, message, now_ms)`：严格校验单调递增，按 `min_interval_ms` 节流防刷屏（到达 `total` 时允许穿透节流发射终态进度）。
  - `complete(*_, request_id)` / `cancelByRequestId(*_, request_id, reason, now_ms)` / `isCancelled(request_id)` / `reapExpired(*_, now_ms)` / `purgeTerminal(*_)` / `getByToken` / `getByRequestId` / `stats()`。

```cp
import {
    McpProgressToken,
    McpProgressReportStatus,
    encodeProgressNotification,
    McpProgressHub,
} from "cap:mcp";

pub fn demoMcpProgressHub() {
    let hub = McpProgressHub.init(100, 30_000);
    const tok = McpProgressToken.fromStr("job-42") ?? return;
    * = hub.register(tok, 1001, 100.0, null, 10_000, 1_000);

    if (hub.reportProgress(tok, 25.0, null, "indexing", 1_000) == McpProgressReportStatus.emitted) {
        let buf: [256]u8 = [0; 256];
        let buf_s: []u8 = buf[0..];
        * = encodeProgressNotification(tok, 25.0, 100.0, "indexing", *buf_s);
    }
}
```


---

## 十八、 零堆分配 MCP 2025-06-18 人机协同 Elicitation 交互审批与扁平 Schema 校验网关 (`elicitation.cp`)

面向 MCP 2025-06-18 `elicitation/create` 人机交互确认与表单采集规范的 100% 纯 CP 零动态堆分配会话网关：

- **常量与枚举**：
  - `MCP_ELICIT_MAX_SESSIONS = 32`、`MCP_ELICIT_MAX_FIELDS = 8`、`MCP_ELICIT_MAX_ENUM_VALS = 8`、`MCP_ELICIT_NAME_MAX_LEN = 48`、`MCP_ELICIT_MSG_MAX_LEN = 160`
  - `ElicitFieldType`（`string_val | number_val | integer_val | boolean_val` — 严格扁平原始类型，禁止嵌套对象）
  - `ElicitStringFormat`（`none | email | uri | date`）
  - `ElicitAction`（`accept | decline | cancel`）、`ElicitSessionState`（`pending | accepted | declined | cancelled | timed_out`）
  - `ElicitStatus`（`ok | not_found | already_resolved | session_expired | capacity_full | duplicate_request_id | invalid_schema | missing_required_field | unknown_field | duplicate_field | type_mismatch | length_out_of_bounds | format_invalid | number_out_of_bounds | enum_mismatch`）
- **格式校验与 JSON-RPC 2.0 编解码**：
  - `elicitValidateEmail` / `elicitValidateUri` / `elicitValidateDate`（含闰年日期边界校验）
  - `encodeElicitationRequest(request_id, message, fields, *out_buf): usize?`：零分配生成标准 MCP 2025-06-18 `elicitation/create` JSON-RPC 2.0 请求载荷（含 `requestedSchema` 与 `required` 数组）。
  - `formatElicitationResponseSummary(action, field_count, *out_buf): usize?`。
- **核心会话状态机 `ElicitationHub`**：
  - `init(default_timeout_ms)` / `createRequest(*_, request_id, message, fields, timeout_ms, now_ms)`
  - `respondAccept(*_, request_id, values, now_ms): ElicitResponseOutcome`：逐字段校验类型/必填/长度/格式/数值上下界/枚举白名单，校验失败保持 `pending` 并返回精确 `failed_field_index`；防重放拦截已终态响应。
  - `respondDecline` / `respondCancel` / `reapExpired` / `purgeTerminal` / `getSessionState` / `stats`。

```cp
import {
    ElicitStringFormat,
    ElicitFieldSpec,
    ElicitFieldValue,
    ElicitationHub,
    encodeElicitationRequest,
} from "cap:mcp";

pub fn demoMcpElicitation() {
    let hub = ElicitationHub.init(30_000);
    const envs: [2][]const u8 = ["staging", "prod"];
    const schema: [2]ElicitFieldSpec = [
        ElicitFieldSpec.enumField("env", true, envs[0..]),
        ElicitFieldSpec.stringField("approver", true, 5, 64, ElicitStringFormat.email),
    ];
    * = hub.createRequest(101, "Approve deployment?", schema[0..], 0, 1_000);

    const vals: [2]ElicitFieldValue = [
        ElicitFieldValue{ name: "env", raw_value: "prod" },
        ElicitFieldValue{ name: "approver", raw_value: "ops@cap.dev" },
    ];
    const out = hub.respondAccept(101, vals[0..], 2_000);
    * = out.status; // ElicitStatus.ok

    let json_buf: [512]u8 = [0; 512];
    let json_s: []u8 = json_buf[0..];
    * = encodeElicitationRequest(101, "Approve deployment?", schema[0..], *json_s);
}
```
