# `cap:a2a` — Agent-to-Agent (A2A) 多智能体协作协议标准库

## 1. 概述

`cap:a2a`（亦可通过 `import cap from "cap"` 访问 `cap.a2a`）是 CP 语言纯原生实现的工业级 **Agent-to-Agent (A2A)** 多智能体协作协议标准库，提供：
- **AgentCard 与能力发现**：标准化描述 Agent 名称、端点 URL、版本、技能（`AgentSkill`）与流式/推送能力，支持 RFC 8259 JSON 序列化与摘要解析。
- **Task 生命周期状态机与多模态载荷**：严格校验 `Submitted` / `Working` / `InputRequired` / `Completed` / `Failed` / `Canceled` / `Rejected` 状态转移，终态不可逆；支持 `Text`、`DataJson`、`FileUri` 载荷及 `Artifact` 产物分片。
- **JSON-RPC 2.0 任务路由与 SSE 流式事件**：内置 `A2aServer`（支持 `agent/card`、`tasks/send`、`tasks/get`、`tasks/cancel`，含 `historyLength` 截断与容量上限防护）及 `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` SSE 编解码。

## 2. 核心类型

- `AgentSkill` / `AgentCapabilities` / `AgentCard` / `AgentCardSummary`：Agent 元数据与能力声明。
- `TaskState`：`Submitted`、`Working`、`InputRequired`、`Completed`、`Failed`、`Canceled`、`Rejected`（含 `toString()`、`fromString()`、`isTerminal()`、`canTransitionTo()`）。
- `PartKind` / `Part` / `MessageRole` / `Message` / `Artifact`：任务消息与产物结构。
- `A2aServerConfig` / `A2aTask` / `A2aServer`：有界容量的任务路由器与状态机管理器。
- `SseEventKind` / `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` / `SseTaskEvent` / `ParsedSseTaskEvent`：SSE 流式事件模型。
- `A2aError`：协议错误集（`InvalidJson`、`TaskNotFound`、`TaskNotCancelable`、`InvalidStateTransition`、`CapacityExceeded`、`InvalidSseEvent` 等）。

## 3. API 速查

| 符号 | 说明 |
| :--- | :--- |
| `encodeAgentCardJson(alloc, card)` | 将 `AgentCard` 序列化为符合 RFC 8259 转义的 JSON |
| `parseAgentCardSummary(alloc, json)` | 解析 `AgentCard` JSON 摘要（需调用 `summary.deinit(alloc)` 释放） |
| `A2aServer.init(alloc, card, config)` | 初始化 A2A 任务路由器 |
| `server.sendTask(task_id, ctx_id, msg)` | 创建任务或对 `InputRequired` 任务续传追问 |
| `server.transitionTask(task_id, next, msg)` | 执行受控状态转移并可选追加 Agent 回复 |
| `server.addArtifact(task_id, artifact)` | 为任务追加输出产物 |
| `server.cancelTask(task_id)` | 取消非终态任务；终态返回 `error.TaskNotCancelable` |
| `server.handleJsonRpc(raw_req)` | 路由 `agent/card`、`tasks/send`、`tasks/get`、`tasks/cancel` |
| `encodeTaskEventSse(alloc, event)` | 编码 `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` 为 SSE 帧 |
| `parseSseTaskEvent(alloc, raw_sse)` | 解析 SSE 帧为 `ParsedSseTaskEvent`（需调用 `deinit(alloc)`） |

## 4. 示例

```typescript
import { AgentCard, AgentSkill, A2aServer, encodeAgentCardJson } from "cap:a2a";

const skills: [1]AgentSkill = [AgentSkill{ .id = "code-review", .name = "Code Review" }];
const card = AgentCard{ .name = "Reviewer", .url = "https://agent.local/a2a", .skills = &skills };
let srv = A2aServer.init(alloc, card, .{});
defer srv.deinit();

const resp = try srv.handleJsonRpc("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tasks/send\",\"params\":{\"id\":\"t-1\",\"message\":{\"text\":\"check diff\"}}}");
defer alloc.free(resp);
```
