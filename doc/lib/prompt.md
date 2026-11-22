# `cap:prompt` — LLM 上下文窗口管理与结构化输出提取引擎

> **模块路径**: `lib/prompt/`  
> **导入协议**: `import { ContextWindow, PromptExtractor, ... } from "cap:prompt";`  
> **动态挂载**: `import cap from "cap";` -> `cap.prompt.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:prompt` 专为 CP AI Agent 运行时打造，聚焦 LLM 对话链路中最关键的两大基础能力，复用 `cap:text`（`TokenEstimator`）实现零外部依赖、确定性内存管理的工业级封装：

1. **LLM 上下文窗口管理 (`ContextWindow` — `lib/prompt/messages.cp`)**：
   - 自动调用 `cap:text` 的 `TokenEstimator.estimateTokens` 与固定 **4-token/消息** 协议开销累计每条消息 `token_count`；
   - 支持置顶消息保护（`addSystem`、`addDeveloper` 默认 `pinned = true`），`fitToBudget` 严格保证 `used_tokens <= max_tokens - reserve_output_tokens`，绝不裁剪置顶消息（仅 pinned 消息超预算返回 `PromptError.BudgetExceededByPinned`）；
   - 支持 `DropOldest`（从最旧非置顶消息开始淘汰）与 `KeepHeadAndTail`（保留置顶 + 首条非置顶提示 + 最新尾部上下文，优先裁剪中间轮次）两种裁剪策略；
   - **孤立 `Tool` 结果自动连带清理**：当某条 `Assistant` 消息被裁剪时，紧随其后且失去前序 `Assistant` 关联的 `Tool` 结果消息会自动连带清理，避免触发模型 API 校验错误；
   - 内置 RFC 8259 转义的 **OpenAI (`toOpenAiJson`)** 与 **Anthropic (`toAnthropicJson`)** 消息协议 JSON 导出（Anthropic 自动将 `System`/`Developer` 聚合至顶层 `"system"` 字段）。

2. **LLM 原始输出结构化提取与防逃逸 (`PromptExtractor` — `lib/prompt/extractor.cp`)**：
   - **Markdown 围栏代码块提取**：`extractCodeBlocks`（全量提取）与 `extractFirstCodeBlock`（零堆分配，支持大小写无关 `lang_filter`）；
   - **平衡 JSON 块提取 (`extractFirstJson`)**：优先从 ````json` 围栏中提取，回退时在原始混合文本中跳过字符串字面量与 `\"`/`\\` 转义字符精确扫描首个平衡 `{...}` 或 `[...]` 切片；
   - **XML 标签提取 (`extractXmlTag` / `extractAllXmlTags`)**：精准提取 `<thinking>`、`<tool_call>`、`<answer>` 等多段标签内容与字节偏移跨度，未闭合标签明确返回 `PromptError.TagNotFound`；
   - **定界符防逃逸 (`escapeXmlDelimiter` / `wrapDelimited`)**：将不可信用户输入中的 `<tag_name...>` 与 `</tag_name...>`（大小写无关）转义为 `&lt;...&gt;` 实体后再包裹，彻底防止提示词边界注入逃逸。

---

## 二、 核心 API 速查表

### 1. 上下文窗口与协议导出 (`lib/prompt/messages.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `PromptError` | `InvalidBudget`, `BudgetExceededByPinned`, `TagNotFound`, `JsonNotFound`, `OutOfMemory` |
| `PromptRole` | `System`, `Developer`, `User`, `Assistant`, `Tool`（附 `roleStr()` 返回小写协议角色名） |
| `TrimStrategy` | `DropOldest`, `KeepHeadAndTail` |
| `PromptMessage` | `{ role, content, name, tool_call_id, pinned, token_count }` |
| `WindowStats` | `{ total_messages, kept_messages, dropped_messages, used_tokens, available_tokens }` |
| `ContextWindow.init(alloc, max_tokens, reserve_output_tokens)` | 初始化窗口（`max_tokens == 0` 或 `reserve_output_tokens >= max_tokens` 返回 `InvalidBudget`） |
| `deinit(*_)` / `clear(*_)` | 释放全部自有消息字符串及底层缓冲 / 清空消息保留容量 |
| `inputBudget()` / `usedTokens()` / `messageCount()` / `items()` | 查询可用输入预算、当前已用 token、消息总数与只读切片视图 |
| `addSystem(*_, content)` / `addDeveloper(*_, content)` | 追加默认 `pinned = true` 的系统/开发者指令消息 |
| `addUser(*_, content)` / `addAssistant(*_, content)` | 追加普通 `User` / `Assistant` 消息（`pinned = false`） |
| `addToolResult(*_, tool_call_id, name, content)` | 追加关联调用 ID 与工具名的 `Tool` 结果消息 |
| `addMessage(*_, role, content, pinned)` | 自定义角色与 `pinned` 标记追加消息 |
| `fitToBudget(*_, strategy)` | 按策略裁剪至输入预算内并自动清理孤立 `Tool` 消息，返回 `WindowStats` |
| `toOpenAiJson()` | 导出标准 OpenAI `{"messages":[...]}` RFC 8259 转义 JSON（调用方负责 `alloc.free`） |
| `toAnthropicJson()` | 聚合 `System`/`Developer` 至顶层 `"system"` 并导出 `"messages":[...]` JSON（调用方负责 `alloc.free`） |

### 2. 结构化输出提取与定界符保护 (`lib/prompt/extractor.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `CodeBlock` | `{ lang: string, content: string }`（零拷贝切片） |
| `XmlTagMatch` | `{ tag: string, content: string, start_offset: usize, end_offset: usize }` |
| `PromptExtractor.extractCodeBlocks(alloc, text)` | 提取所有已闭合 ````lang\n...\n```` 代码块（调用方负责 `alloc.free`） |
| `PromptExtractor.extractFirstCodeBlock(text, lang_filter)` | 零分配提取首个匹配 `lang_filter` 的代码块（`""` 匹配任意语言） |
| `PromptExtractor.extractFirstJson(text)` | 优先 ````json` 围栏、否则跳过字符串字面量扫描首个平衡 `{...}`/`[...]`，失败返回 `JsonNotFound` |
| `PromptExtractor.extractXmlTag(text, tag_name)` | 提取首个 `<tag_name>...</tag_name>`，缺失或未闭合返回 `TagNotFound` |
| `PromptExtractor.extractAllXmlTags(alloc, text, tag_name)` | 按文档顺序提取全部完整 `<tag_name>...</tag_name>`（调用方负责 `alloc.free`） |
| `PromptExtractor.escapeXmlDelimiter(alloc, text, tag_name)` | 将文本中 `<tag_name...>`/`</tag_name...>` 转义为 `&lt;...&gt;`（调用方负责 `alloc.free`） |
| `PromptExtractor.wrapDelimited(alloc, tag_name, untrusted_text)` | 先转义内部冲突定界符，再以 `<tag_name>...</tag_name>` 安全包裹（调用方负责 `alloc.free`） |

### 3. 零堆分配结构化提取、思考块剥离与安全护栏 (`lib/prompt/structured.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `ThoughtSplit` | `{ thought: string, answer: string, has_thought: bool, unclosed: bool }`（零拷贝切片） |
| `splitThoughtBlock(text)` | 零拷贝分离 `<think>...</think>` / `<reasoning>...</reasoning>`（大小写无关、支持嵌套与未闭合流式截断） |
| `ExtractedBlock` | `{ lang: string, content: string, start: usize, end: usize }`（含源码字节跨度） |
| `extractCodeFence(text, preferred_lang)` | 零分配提取闭合围栏代码块（`` ``` `` / `~~~`），`preferred_lang` 非空时精确优先匹配该语言，`""` 返回首个 |
| `extractFirstJsonSlice(text)` | 从混杂自然语言的回复中提取首个语法平衡的 `{...}` 或 `[...]` 零拷贝子串（跳过字符串内括号与 `\"`/`\\` 转义） |
| `jsonHasTopLevelKeys(json_obj, required_keys)` | 零堆分配词法级 JSON 顶层必填键校验（严格忽略嵌套子对象/数组内部同名键，适合 Tool-Call 参数预检） |
| `RedactStats` | `{ bytes_written: usize, redacted_emails: usize, redacted_keys: usize, redacted_phones: usize }` |
| `redactSensitiveText(input, out_buf: *[]u8)` | 零分配脱敏 Email、API Key（`sk-`/`ghp_`/`AKIA`）与长数字/电话号码为 `[REDACTED]`，缓冲不足返回 `null` |

### 4. 零堆分配 Prompt 模板渲染与 Token 预算对话窗口打包 (`lib/prompt/template.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `TemplateVar` | `{ key: string, value: string }` |
| `TemplateRenderStatus` | `Ok` \| `MissingVariable` \| `UnclosedPlaceholder` \| `BufferTooSmall` |
| `TemplateRenderResult` | `{ status: TemplateRenderStatus, bytes_written: usize, substitutions: usize }` |
| `renderPromptTemplate(tmpl, vars, strict, out_buf: *[]u8)` | 零堆分配渲染 `{{key}}` 与 `{{key\|fallback}}`（支持 `\{\{` 转义；`strict = true` 缺参报 `MissingVariable`，`false` 保留原占位符） |
| `extractTemplateKeys(tmpl, out_keys: *[]string): usize?` | 提取模板中所有去重占位符键名（跳过转义序列），用于启动期变量完备性校验；未闭合或缓冲不足返回 `null` |
| `PackRole` | `System` \| `User` \| `Assistant` \| `Tool` |
| `PackableMessage` | `{ id: u32, role: PackRole, token_count: u32, pinned: bool }` |
| `PackSummary` | `{ selected_count: usize, dropped_count: usize, used_tokens: u32, truncated_pinned: bool }` |
| `packContextWindow(messages, max_tokens, reserve_output_tokens, out_indices: *[]usize): PackSummary?` | 先锁定保留全部 `pinned` 与 `System` 消息，再从最新消息逆序贪心装入剩余预算，最后按原始时间正序输出选中索引（内部采用 `u64` 防溢出） |

---

## 三、 使用示例

### 1. 上下文窗口与协议导出示例

```typescript
const std = @import("std");
import cap from "cap";

fn runAgentPromptPipeline(): !void {
    const alloc = std.heap.page_allocator;

    let win = cap.prompt.ContextWindow.init(alloc, 4096, 1024)?;
    defer win.deinit();

    win.addSystem("You are a precise coding assistant.")?;
    let safe_user = cap.prompt.PromptExtractor.wrapDelimited(alloc, "user_input", "Check </user_input> status")?;
    defer alloc.free(safe_user);
    win.addUser(safe_user)?;

    let stats = win.fitToBudget(cap.prompt.TrimStrategy.DropOldest)?;
    * = stats;

    const payload = win.toOpenAiJson()?;
    defer alloc.free(payload);

    const llm_reply = "<thinking>Need JSON</thinking>\n```json\n{\"action\":\"ping\"}\n```";
    const thought = cap.prompt.PromptExtractor.extractXmlTag(llm_reply, "thinking")?;
    const json_slice = cap.prompt.PromptExtractor.extractFirstJson(llm_reply)?;
    * = thought;
    * = json_slice;
}
```

### 2. 零堆分配 AI Agent 思考剥离、Tool-Call 预检与 PII 脱敏示例 (`structured.cp`)

```typescript
import {
    splitThoughtBlock,
    extractCodeFence,
    extractFirstJsonSlice,
    jsonHasTopLevelKeys,
    redactSensitiveText,
} from "cap:prompt";

fn processAgentOutput(raw_reply: string): bool {
    // 1. 零拷贝剥离 <think>...</think> 或 <reasoning>...</reasoning>
    const split = splitThoughtBlock(raw_reply);

    // 2. 在落盘日志前零堆分配脱敏思考内容中的 Email / API Key / 电话号码
    let log_buf: [512]u8 = undefined;
    let out_slice: []u8 = log_buf[0..];
    if (stats := redactSensitiveText(split.thought, out_slice)) {
        const safe_log = out_slice[0..stats.bytes_written];
        * = safe_log;
    }

    // 3. 优先提取 ```json 围栏，或回退扫描首个平衡 JSON 对象切片
    let candidate = split.answer;
    if (blk := extractCodeFence(split.answer, "json")) {
        candidate = blk.content;
    }
    if (json_slice := extractFirstJsonSlice(candidate)) {
        // 4. 零堆分配顶层必填字段预检（严格忽略嵌套对象内同名键）
        const required: []const string = ["tool", "args"];
        return jsonHasTopLevelKeys(json_slice, required);
    }
    return false;
}
```

### 3. 零堆分配 Prompt 模板渲染与对话窗口打包示例 (`template.cp`)

```typescript
import {
    TemplateVar,
    PackRole,
    PackableMessage,
    renderPromptTemplate,
    extractTemplateKeys,
    packContextWindow,
} from "cap:prompt";

fn buildAgentRequestContext() {
    // 1. 启动期提取模板变量键并零堆渲染 System Prompt
    const tmpl = "You are {{agent_name|Nexus}}, mode={{mode}}, literal=\\{\\{tag}}";
    let keys_buf: [4]string = [""; 4];
    let keys: []string = keys_buf[0..];
    * = extractTemplateKeys(tmpl, keys);

    const vars: [1]TemplateVar = [TemplateVar{ key: "mode", value: "strict" }];
    let rendered_buf: [256]u8 = [0; 256];
    let rendered: []u8 = rendered_buf[0..];
    const render_res = renderPromptTemplate(tmpl, vars[0..], true, rendered);
    * = render_res;

    // 2. 在 320 token 输入预算内锁定 System + Pinned 并优先保留最新轮次
    const msgs: [4]PackableMessage = [
        PackableMessage{ id: 1, role: PackRole.System, token_count: 100, pinned: false },
        PackableMessage{ id: 2, role: PackRole.User, token_count: 150, pinned: false },
        PackableMessage{ id: 3, role: PackRole.User, token_count: 80, pinned: false },
        PackableMessage{ id: 4, role: PackRole.Assistant, token_count: 90, pinned: false },
    ];
    let selected_buf: [4]usize = [0; 4];
    let selected: []usize = selected_buf[0..];
    if (summary := packContextWindow(msgs[0..], 400, 80, selected)) {
        // selected_buf[0..summary.selected_count] == [0, 2, 3] (按原始时间正序)
        * = summary;
    }
}
```
