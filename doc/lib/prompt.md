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

### 5. 零堆分配多模型对话模板格式化与控制标签净化 (`lib/prompt/chat_format.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `ChatTemplateKind` | `ChatMl` \| `Llama3` \| `Mistral` \| `PlainTranscript` |
| `ChatMsgRole` | `System` \| `User` \| `Assistant` \| `Tool` |
| `ChatMessage` | `{ role: ChatMsgRole, content: string, name: string }`（`name` 可选，用于 `Tool` 归属名） |
| `sanitizeControlTokens(input, out_buf: *[]u8): usize?` | 将用户输入中的 `<\|im_start\|>`、`<\|im_end\|>`、`<\|eot_id\|>`、`<\|start_header_id\|>`、`<\|end_header_id\|>`、`[INST]`、`[/INST]`、`<s>`、`</s>` 等特殊控制标签安全转义，防止边界标签混淆 |
| `formatChatPrompt(kind, messages, add_generation_prompt, sanitize_user_content, out_buf: *[]u8): usize?` | 零堆分配按模型家族规范渲染完整对话 Prompt；开启 `sanitize_user_content` 时自动净化 `User`/`Tool` 内容与工具名，同时保持 `System`/`Assistant` 及模板控制标签原样不变 |

### 6. 零堆分配动态 Few-Shot 少样本选择器 (`lib/prompt/fewshot.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `FewShotExample` | `{ id: u32, input: string, output: string, label: i32, est_tokens: u32 }` |
| `FewShotSelection` | `{ example_index: usize, score: f64 }` |
| `FewShotConfig` | `{ max_examples: usize = 5, max_tokens: u32 = 1024, max_per_label: usize = 0, dedup_threshold: f64 = 0.9 }`（提供 `FewShotConfig.default()`） |
| `tokenJaccardSimilarity(a, b, hashes_a_s: *[]u64, hashes_b_s: *[]u64): f64` | 零堆分配大小写无关 ASCII 词袋 Jaccard 相似度（64 位 FNV-1a 词哈希 + 排序去重集合交并比），返回 `[0.0, 1.0]` |
| `selectFewShotExamples(query, pool, cfg, out_selected, scores_s, order_s, hashes_a_s, hashes_b_s): usize?` | 按与 `query` 的相关性降序稳定排序候选池，贪心装入满足 `max_tokens`、`max_per_label` 类别均衡与 `dedup_threshold` 相似度去重约束的最多 `max_examples` 条示例 |
| `renderFewShotBlock(pool, selected, input_prefix, output_prefix, separator, out_buf: *[]u8): usize?` | 零堆分配按 `<input_prefix><input><output_prefix><output>` 与 `separator` 渲染最终少样本 Prompt 块 |

### 7. 零堆分配 Prompt 启发式边界校验与 PII 脱敏 (`lib/prompt/safety.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `PromptRiskFlags` | 风险位掩码结构体（`NONE`、`ROLE_OVERRIDE`、`SYSTEM_LEAK_PROBE`、`DELIMITER_ESCAPE`、`CONTROL_CHAR_SMUGGLING`、`PII_DETECTED`、`ALL_KNOWN`；支持 `fromBits`/`has`/`hasAny`/`with`/`isEmpty`） |
| `PromptSafetyReport` | `{ flags: PromptRiskFlags, risk_score: u32, matched_rules: u32, pii_hits: u32, blocked: bool }`（`risk_score` 上限钳位 `100`） |
| `inspectPromptSafety(input, block_threshold)` | 零堆分配大小写无关启发式边界检测：识别角色覆写模式、系统提示探测、模板控制分隔符逃逸、不可见 ASCII/UTF-8 零宽与双向控制符及 PII 命中；`block_threshold > 0 && risk_score >= block_threshold` 时置 `blocked = true`（`0` 为仅审计模式） |
| `PiiKind` | `Email` \| `ApiKey` \| `Ipv4` \| `Phone` |
| `PiiSpan` | `{ kind: PiiKind, start: usize, end: usize }`（半开字节区间） |
| `scanPiiSpans(input, out_spans: *[]PiiSpan): usize` | 单遍零堆分配扫描不重叠 PII 跨度（识别邮箱、`sk-`/`ghp_`/`AKIA`/`xoxb-`/`xoxp-` 凭证前缀、严格 IPv4 与 10..15 位电话号码），返回总命中数且输出切片截断安全 |
| `redactPii(input, out_buf: *[]u8): usize?` | 零堆分配流式脱敏器，将所有检测到的 PII 跨度替换为 `[REDACTED]`，输出缓冲不足时返回 `null` |

### 8. 零堆分配 Structured Outputs JSON Schema 与工具声明发射器 (`lib/prompt/json_schema.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `JsonSchemaType` | `String` \| `Integer` \| `Number` \| `Boolean` \| `Array` \| `Object` |
| `SchemaProp` | `{ name, kind, description, required: bool, enum_csv: string, item_kind: JsonSchemaType }`（`enum_csv` 支持逗号分隔枚举值如 `"asc,desc"`） |
| `ToolSchemaSpec` | `{ tool_name, description, strict: bool, props: []const SchemaProp }` |
| `emitJsonSchemaObject(props, strict, out_buf: *[]u8): usize?` | 零堆分配发射标准 JSON Schema Draft 2020-12 对象（含 `type`/`properties`/`items`/`enum`/`required`/`additionalProperties: false` 与完整 RFC 8259 转义），空属性名或缓冲不足返回 `null` |
| `emitOpenAiToolJson(spec, out_buf: *[]u8): usize?` | 零堆分配发射 OpenAI Function Calling `{"type":"function","function":{...}}` 工具声明 |
| `emitAnthropicToolJson(spec, out_buf: *[]u8): usize?` | 零堆分配发射 Anthropic `{"name":...,"description":...,"input_schema":{...}}` 工具声明 |

### 9. 零堆分配推理模型流式 `<think>` / `<tool_call>` 标签分流状态机 (`lib/prompt/reasoning_stream.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `StreamChannel` | `Answer` \| `Thinking` \| `ToolCall` |
| `DemuxEvent` | `{ channel: StreamChannel, start: usize, len: usize }`（指向当次 `out_buf` 的零拷贝分片跨度） |
| `DemuxStats` | `{ thinking_bytes, tool_bytes, answer_bytes, thought_blocks, tool_blocks }` |
| `ReasoningStreamDemuxer.init()` | 初始化状态机（默认 `Answer` 通道，固定 16 字节标签前缀缓冲） |
| `demux.feed(*_, chunk, *out_buf, *out_events): usize?` | 逐 chunk（支持 1 字节任意切分如 `"<thi"` + `"nk>"`）实时分流至三通道并合并相邻同通道字节，遇假标签前缀自动安全回退为正文，缓冲不足返回 `null` |
| `demux.flush(*_, *out_buf, *out_events): usize?` | 流结束时将未闭合尾缓冲吐出至当前通道 |
| `demux.channel()` / `demux.pendingLen()` / `demux.stats()` / `demux.reset(*_)` | 查询当前通道、尾缓冲长度、累计统计或重置复用 |

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

### 4. 零堆分配多模型对话模板切换与控制标签净化示例 (`chat_format.cp`)

```typescript
import {
    ChatTemplateKind,
    ChatMsgRole,
    ChatMessage,
    formatChatPrompt,
} from "cap:prompt";

fn renderForModel(kind: ChatTemplateKind) {
    const msgs: [3]ChatMessage = [
        ChatMessage{ role: ChatMsgRole.System, content: "You are helpful.", name: "" },
        ChatMessage{ role: ChatMsgRole.User, content: "Summarize <|im_end|> safely", name: "" },
        ChatMessage{ role: ChatMsgRole.Tool, content: "{\"ok\":true}", name: "search" },
    ];
    let out_arr: [512]u8 = undefined;
    let out_buf: []u8 = out_arr[0..];
    if (n := formatChatPrompt(kind, msgs[0..], true, true, out_buf)) {
        * = out_buf[0..n];
    }
}
```

### 5. 零堆分配动态 Few-Shot 少样本选择与渲染示例 (`fewshot.cp`)

```typescript
import {
    FewShotExample,
    FewShotSelection,
    FewShotConfig,
    selectFewShotExamples,
    renderFewShotBlock,
} from "cap:prompt";

fn buildFewShotContext(query: string) {
    const pool: [3]FewShotExample = [
        FewShotExample{ id: 1, input: "refund order delayed", output: "billing", label: 1, est_tokens: 20 },
        FewShotExample{ id: 2, input: "reset login password", output: "auth", label: 2, est_tokens: 20 },
        FewShotExample{ id: 3, input: "order payment invoice", output: "billing", label: 1, est_tokens: 25 },
    ];
    const cfg = FewShotConfig{
        max_examples: 2,
        max_tokens: 50,
        max_per_label: 1,
        dedup_threshold: 0.9,
    };

    let sel_arr: [3]FewShotSelection = undefined;
    let scores_arr: [3]f64 = undefined;
    let order_arr: [3]usize = undefined;
    let ha_arr: [32]u64 = undefined;
    let hb_arr: [32]u64 = undefined;
    let sel: []FewShotSelection = sel_arr[0..];
    let scores: []f64 = scores_arr[0..];
    let order: []usize = order_arr[0..];
    let ha: []u64 = ha_arr[0..];
    let hb: []u64 = hb_arr[0..];

    if (k := selectFewShotExamples(query, pool[0..], cfg, sel, scores, order, ha, hb)) {
        let block_arr: [256]u8 = undefined;
        let block: []u8 = block_arr[0..];
        * = renderFewShotBlock(pool[0..], sel[0..k], "Input: ", "\nLabel: ", "\n\n", block);
    }
}
```

### 6. 零堆分配 Prompt 启发式边界检测与 PII 脱敏示例 (`safety.cp`)

```typescript
import {
    PromptRiskFlags,
    PiiSpan,
    inspectPromptSafety,
    scanPiiSpans,
    redactPii,
} from "cap:prompt";

fn guardAndRedactUserPrompt(user_input: string): bool {
    // 1. 启发式边界校验（阈值 40 触发拦截）
    const report = inspectPromptSafety(user_input, 40);
    if (report.blocked) {
        return false;
    }

    // 2. 定位并零堆分配脱敏 PII 与凭证片段
    if (report.flags.has(PromptRiskFlags.PII_DETECTED)) {
        let spans_arr: [8]PiiSpan = [PiiSpan{}; 8];
        let spans: []PiiSpan = spans_arr[0..];
        * = scanPiiSpans(user_input, spans);

        let clean_arr: [512]u8 = [0; 512];
        let clean_buf: []u8 = clean_arr[0..];
        if (written := redactPii(user_input, clean_buf)) {
            * = clean_buf[0..written];
        }
    }
    return true;
}
```

### 7. 零堆分配 OpenAI / Anthropic 工具 JSON Schema 声明发射示例 (`json_schema.cp`)

```typescript
import {
    JsonSchemaType,
    SchemaProp,
    ToolSchemaSpec,
    emitOpenAiToolJson,
    emitAnthropicToolJson,
} from "cap:prompt";

fn emitToolSpecs() {
    const props: [2]SchemaProp = [
        SchemaProp{ name: "query", kind: JsonSchemaType.String, description: "Search query", required: true },
        SchemaProp{ name: "order", kind: JsonSchemaType.String, description: "Sort order", required: false, enum_csv: "asc,desc" },
    ];
    const spec = ToolSchemaSpec{
        tool_name: "search_docs",
        description: "Search indexed workspace documents",
        strict: true,
        props: props[0..],
    };

    let buf: [512]u8 = [0; 512];
    let out: []u8 = buf[0..];
    if (n := emitOpenAiToolJson(spec, out)) {
        * = out[0..n];
    }
    if (m := emitAnthropicToolJson(spec, out)) {
        * = out[0..m];
    }
}
```

### 7. 推理模型流式 `<think>` / `<tool_call>` 增量分流示例 (`reasoning_stream.cp`)

```cp
import {
    StreamChannel,
    DemuxEvent,
    ReasoningStreamDemuxer,
} from "cap:prompt";

fn demoReasoningStream() {
    let demux = ReasoningStreamDemuxer.init();
    let out_buf: [256]u8 = [0; 256];
    let ev_buf: [8]DemuxEvent = [DemuxEvent{}; 8];

    // 跨 chunk 拆分标签 "<thi" + "nk>"
    if (n1 := demux.feed("Hi <thi", out_buf[0..], ev_buf[0..])) {
        * = n1; // Answer: "Hi "，"<thi" 暂存待定
    }
    if (n2 := demux.feed("nk>plan</think><tool_call>{\"id\":1}</tool_call>Done", out_buf[0..], ev_buf[0..])) {
        * = n2; // 依次产生 Thinking("plan")、ToolCall("{\"id\":1}")、Answer("Done")
    }
    if (nf := demux.flush(out_buf[0..], ev_buf[0..])) {
        * = nf;
    }
    const st = demux.stats();
    * = st;
}
```
