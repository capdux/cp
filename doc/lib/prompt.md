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

### 5. 零堆分配多模型对话模板格式化与控制标签净化 (`lib/prompt/chatFmt.cp`)

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

### 8. 零堆分配 Structured Outputs JSON Schema 与工具声明发射器 (`lib/prompt/jsonSchema.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `JsonSchemaType` | `String` \| `Integer` \| `Number` \| `Boolean` \| `Array` \| `Object` |
| `SchemaProp` | `{ name, kind, description, required: bool, enum_csv: string, item_kind: JsonSchemaType }`（`enum_csv` 支持逗号分隔枚举值如 `"asc,desc"`） |
| `ToolSchemaSpec` | `{ tool_name, description, strict: bool, props: []const SchemaProp }` |
| `emitJsonSchemaObject(props, strict, out_buf: *[]u8): usize?` | 零堆分配发射标准 JSON Schema Draft 2020-12 对象（含 `type`/`properties`/`items`/`enum`/`required`/`additionalProperties: false` 与完整 RFC 8259 转义），空属性名或缓冲不足返回 `null` |
| `emitOpenAiToolJson(spec, out_buf: *[]u8): usize?` | 零堆分配发射 OpenAI Function Calling `{"type":"function","function":{...}}` 工具声明 |
| `emitAnthropicToolJson(spec, out_buf: *[]u8): usize?` | 零堆分配发射 Anthropic `{"name":...,"description":...,"input_schema":{...}}` 工具声明 |

### 9. 零堆分配推理模型流式 `<think>` / `<tool_call>` 标签分流状态机 (`lib/prompt/reasoning.cp`)

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

### 4. 零堆分配多模型对话模板切换与控制标签净化示例 (`chatFmt.cp`)

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

### 7. 零堆分配 OpenAI / Anthropic 工具 JSON Schema 声明发射示例 (`jsonSchema.cp`)

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

### 7. 推理模型流式 `<think>` / `<tool_call>` 增量分流示例 (`reasoning.cp`)

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

### 8. LLM 结构化输出提取、容错 JSON 修复与自动纠错提示器 (`structuredOutput.cp`)

面向生产级 LLM 结构化输出解析与自愈重试的 100% 零堆分配管线：

- **`JsonCandidateSpan` / `extractJsonCandidate(raw_output: string): JsonCandidateSpan?`**：自动剥离 `<think>...</think>` 推理块（含孤立 `</think>`）、Markdown 代码围栏（```` ```json ... ``` ````）与前后自然语言解释，零拷贝定位最外层 `{...}` / `[...]` 区间并标记 `from_code_fence`、`skipped_thought` 与 `is_truncated`。
- **`JsonRepairReport` / `repairLlmJson(input: string, out_buf: *[]u8): JsonRepairReport?`**：确定性修复 LLM 常见畸形 JSON——剔除尾随逗号（`,}` / `,]` / 尾部 `,`）、闭合截断的字符串与嵌套括号、补全截断键值冒号（`{"k":` -> `{"k":null}`）、规范化单引号键/值字符串并转义内部双引号与原始换行 `\n`/`\r`，缓冲区不足或嵌套超过 64 层安全返回 `null`。
- **`JsonValueKind` / `RequiredFieldSpec` / `FieldCheckIssueKind` / `FieldCheckIssue` / `RequiredFieldsReport`**：
  - **`checkRequiredJsonFields(json_text, specs, out_issues: *[]FieldCheckIssue): RequiredFieldsReport`**：跳过嵌套结构精准校验顶层必填字段存在性与类型匹配（`Integer` 自动兼容 `Number`）。
  - **`formatRepairRetryPrompt(issues, issue_count, schema_hint, out_buf: *[]u8): usize?`**：基于校验问题清单零堆分配生成简洁精准的 LLM Self-Correction 重试提示词。

```cp
import {
    JsonValueKind,
    RequiredFieldSpec,
    FieldCheckIssue,
    extractJsonCandidate,
    repairLlmJson,
    checkRequiredJsonFields,
    formatRepairRetryPrompt,
} from "cap:prompt";

fn demoStructuredOutputRepair() {
    const raw = "<think>drafting</think>Here is the result:\n```json\n{'title': 'CP', 'score': 98,}\n```";
    if (span := extractJsonCandidate(raw)) {
        const candidate = raw[span.start..span.end];
        let repaired_buf: [256]u8 = [0; 256];
        let repaired_s: []u8 = repaired_buf[0..];
        if (rep := repairLlmJson(candidate, repaired_s)) {
            const clean_json = repaired_buf[0..rep.bytes_written];
            const specs: [3]RequiredFieldSpec = [
                RequiredFieldSpec{ name: "title", expected_kind: JsonValueKind.String },
                RequiredFieldSpec{ name: "score", expected_kind: JsonValueKind.Number },
                RequiredFieldSpec{ name: "tags", expected_kind: JsonValueKind.Array },
            ];
            let issues_buf: [4]FieldCheckIssue = [FieldCheckIssue{}; 4];
            let issues_s: []FieldCheckIssue = issues_buf[0..];
            const chk = checkRequiredJsonFields(clean_json, specs[0..], issues_s);
            if (!chk.passed) {
                let retry_buf: [512]u8 = [0; 512];
                let retry_s: []u8 = retry_buf[0..];
                if (n := formatRepairRetryPrompt(issues_buf[0..], chk.issue_count, "{title, score, tags}", retry_s)) {
                    * = retry_buf[0..n];
                }
            }
        }
    }
}
```

### 9. Agent 系统提示词与工具/记忆/RAG 一体化编排器 (`composer.cp`)

100% 纯 CP 零堆分配的 Agent Prompt 编排器，在总 `max_tokens` 预算下按优先级分配预算、支持超预算按比例截断与丢弃，并按标准 LLM 分段顺序（`SystemRole -> ToolCatalog -> MemoryContext -> RagContext -> UserQuery`）输出结构化提示词与 `AgentPromptBuildReport` 分段计数报告：

- **导出符号**：`MAX_COMPOSER_BLOCKS`、`AgentPromptSection`（`SystemRole | MemoryContext | RagContext | ToolCatalog | UserQuery`）、`AgentPromptBlock`、`AgentPromptBuildReport`、`AgentPromptComposer`（`init`, `addSection`, `addSystemRole`, `addUserQuery`, `addToolCatalog`, `addRagContext`, `addMemoryContext`, `build`, `sectionCount`, `reset`）。

```cp
import { AgentPromptBlock, AgentPromptComposer } from "cap:prompt";

fn demoAgentPromptComposer() {
    let blocks: [8]AgentPromptBlock = [AgentPromptBlock{}; 8];
    let blocks_s: []AgentPromptBlock = blocks[0..];
    let composer = AgentPromptComposer.init(blocks_s, 120) ?? return;

    * = composer.addSystemRole("You are a precise CP systems assistant.", 12);
    * = composer.addToolCatalog("read_file(path), run_test(filter)", 14);
    * = composer.addMemoryContext("[mem#1] User prefers zero-alloc stack buffers.", 16);
    * = composer.addRagContext("[doc:10] CP supports 8-lane SIMD @dot.", 16);
    * = composer.addUserQuery("Explain how to build a zero-alloc RAG loop.", 14);

    let out: [1024]u8 = [0; 1024];
    let out_s: []u8 = out[0..];
    if (rep := composer.build(out_s)) {
        * = out_s[0..rep.bytes_written];
    }
}
```

### 10. 提示词边界防火墙与凭证防泄漏扫描器 (`firewall.cp`)

100% 纯 CP 零堆分配的多层输入/输出边界扫描与凭证脱敏器，单遍检测角色分隔符越界（`<|im_start|>`/`<|system|>`/`[INST]`/`### System:`）、凭证泄漏（`sk-`/`ghp_`/`AKIA`/`-----BEGIN`）、路径穿越（`../`/`%2e%2e/`）与危险 URL Scheme（`file://`/`javascript:`/`data:text/html`），输出 `0..100` 风险分与 `Allow | Redact | Block` 裁决，并支持零堆替换为 `[REDACTED_SECRET]`：

- **导出符号**：`FIREWALL_FLAG_DELIMITER_OVERRIDE`、`FIREWALL_FLAG_SECRET_LEAK`、`FIREWALL_FLAG_PATH_TRAVERSAL`、`FIREWALL_FLAG_UNSAFE_URL`、`FirewallAction`、`FirewallPolicy`（`default`, `strict`）、`FirewallFinding`、`FirewallVerdict`、`evaluatePromptFirewall`、`redactFirewallSecrets`。

```cp
import {
    FirewallAction,
    FirewallPolicy,
    FirewallFinding,
    evaluatePromptFirewall,
    redactFirewallSecrets,
} from "cap:prompt";

fn demoPromptFirewall(text: string) {
    let findings: [8]FirewallFinding = [FirewallFinding{}; 8];
    const verdict = evaluatePromptFirewall(text, FirewallPolicy.default(), findings[0..]);

    if (verdict.action == FirewallAction.Redact) {
        let safe_buf: [512]u8 = [0; 512];
        if (n := redactFirewallSecrets(text, safe_buf[0..])) {
            * = safe_buf[0..n];
        }
    }
}
```

### 11. 零堆分配多模态附件校验与视觉瓦片 Token 预算估算器 (`multimodal.cp`)

面向多模态 AI Agent（文本 / 图像 / 音频 / 文档）的 100% 纯 CP 零堆分配附件治理与 Token 估算器：

- **导出符号**：`MediaPartKind`（`Text | Image | Audio | Document`）、`ImageDetailMode`（`Low | High | Auto`）、`MultimodalRejectReason`、`MultimodalPart`、`MultimodalLimits`（`default`, `isValid`）、`MultimodalAuditResult`、`estimateImageTokens`、`estimateAudioTokens`、`isAllowedMultimodalMime`、`auditMultimodalPayload`。
- **核心算法**：
  - `estimateImageTokens(w, h, detail)`：`Low` 固定 `85` tokens；`High` 遵循先缩放至 `2048×2048` 内、再将短边缩放至 `768px` 按 `512×512` 瓦片计 `85 + 170 * tiles`；`Auto` 对 `<= 512×512` 小图走 `Low`，大图走 `High`。
  - `estimateAudioTokens(duration_ms, tokens_per_sec)`：默认 `32 tokens/sec` 向上取整。
  - `isAllowedMultimodalMime(kind, mime_type)`：大小写无关且自动忽略 `;charset=...` 参数的严格 MIME 白名单。
  - `auditMultimodalPayload(parts, limits)`：一站式校验附件数、总字节数、尺寸/时长/页数上限、MIME 白名单与多模态总 Token 预算，精准返回违规原因与 `offending_index`。

```cp
import {
    MediaPartKind,
    ImageDetailMode,
    MultimodalPart,
    MultimodalLimits,
    estimateImageTokens,
    auditMultimodalPayload,
} from "cap:prompt";

fn demoMultimodalBudgetAudit() {
    // 1024x1024 High 图像 => 缩放至 768x768 => 2x2=4 瓦片 => 85 + 170*4 = 765 tokens
    const img_toks = estimateImageTokens(1024, 1024, ImageDetailMode.High);
    * = img_toks;

    const parts: [3]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Text, mime_type: "text/plain; charset=utf-8", byte_size: 400 },
        MultimodalPart{ kind: MediaPartKind.Image, mime_type: "image/png", byte_size: 250000, width: 1024, height: 1024, image_detail: ImageDetailMode.High },
        MultimodalPart{ kind: MediaPartKind.Audio, mime_type: "audio/wav", byte_size: 160000, duration_ms: 5000 },
    ];
    const audit = auditMultimodalPayload(parts[0..], MultimodalLimits.default());
    if (audit.allowed) {
        * = audit.total_tokens; // 100 + 765 + 160 = 1025
    }
}
```

### 12. 零堆分配 Prompt 版本注册表与灰度 A/B 流量分配器 (`versions.cp`)

面向生产级 AI Agent Prompt 运维与实验平台的 100% 纯 CP 零堆分配版本管理与灰度切流内核（内联固定容量 `MAX_PROMPT_VERSIONS = 16` 与定长字节缓冲，零动态内存分配）：

- **导出符号**：`MAX_PROMPT_VERSIONS`、`BPS_SCALE`（`10000`）、`PromptVariantStatus`（`draft | canary | active | shadow | deprecated | rolled_back`）、`PromptVersionEntry`、`RouteDecision`、`RolloutGuardConfig`、`RegistrySummaryStats`、`PromptVersionRegistry`。
- **核心能力**：
  - `register_version(*_, id, template_hash, model_hint, summary, major, minor, patch, created_ms): usize?`：去重 ID、校验定长字节上限与容量边界；
  - `set_active` / `set_shadow` / `start_canary` / `promote_canary` / `rollback`：原子状态流转与基点权重（`0..=10000` bps）自动归一化，支持灰度中一键回滚与晋升后一键回滚至 `prev_active_slot`；
  - `route_request(tenant_key: []const u8, seed: u64): RouteDecision`：基于 FNV-1a + SplitMix64 混合映射到 `[0, 10000)` 基点桶，确保同租户 100% 粘性路由与扩量单调性，支持影子流量标记与无 `active` 时安全降级（`is_fallback = true`）；
  - `record_outcome` / `should_auto_rollback(guard)` / `evaluate_and_auto_rollback(guard)` / `variant_error_rate_bps` / `variant_avg_latency_us` / `summary_stats`。

```cp
import {
    RolloutGuardConfig,
    PromptVersionRegistry,
} from "cap:prompt";

fn demoPromptCanaryRollout() {
    let reg = PromptVersionRegistry.init();
    const v1 = reg.register_version("sys-v1", 0x1111, "claude-3.5", "Baseline assistant", 1, 0, 0, 1000) ?? return;
    const v2 = reg.register_version("sys-v2", 0x2222, "claude-3.5", "Concise JSON assistant", 1, 1, 0, 2000) ?? return;

    * = reg.set_active(v1);
    * = reg.start_canary(v2, 2000); // 20.00% 灰度切流至 v2，80.00% 留在 v1

    const route = reg.route_request("tenant-acme", 42);
    if (route.found) {
        * = reg.record_outcome(route.slot_idx, true, 120000, 350);
    }

    if (guard := RolloutGuardConfig.init(20, 500, 1500000)) {
        * = reg.evaluate_and_auto_rollback(guard);
    }
}
```

### 13. 零分配 LLM 流式 JSON 增量字段解析器与 Tool-Call 实时提取引擎 (`jsonStream.cp`)

面向流式 LLM Tool-Call 参数（分片到达的 JSON 对象）的 100% 纯 CP 零动态堆分配逐字节/分片可恢复状态机解析器：

- **常量与枚举**：`JSON_STREAM_MAX_DEPTH` (`16`)、`JSON_STREAM_MAX_KEY_LEN` (`64`)、`JSON_STREAM_MAX_VAL_LEN` (`256`)、`JSON_STREAM_MAX_EVENTS` (`32`)、`JsonStreamEventKind`（`key_ready | string_delta | field_complete | object_complete | syntax_error`）、`JsonStreamValueKind`（`string | number | boolean | null_val | nested_object | nested_array`）。
- **数据模型**：`JsonStreamEvent`（提供 `keySlice()` 与 `dataSlice()`）、`JsonStreamStats`。
- **核心引擎 `JsonStreamScanner`**：
  - `init()` / `reset()` / `is_complete()` / `has_error()` / `stats()`
  - `feed(*_, chunk: []const u8, out_events: *[]JsonStreamEvent): usize`：支持任意 1 字节到任意分片跨 chunk 恢复；完整解码跨 chunk 转义（`\"`/`\\`/`\/`/`\n`/`\r`/`\t`）；顶层字符串值到达时立即流式发射 `string_delta` 供前端零延迟预览/推测预取，字段结束发射 `field_complete`（支持 `string`/`number`/`boolean`/`null`/嵌套 `{...}`/`[...]`）；顶层 `}` 闭合发射 `object_complete`；非对象根、非法转义、括号不匹配、未转义控制字符或嵌套超 16 层立即发射 `syntax_error` 并锁定错误态。

```cp
import {
    JsonStreamEventKind,
    JsonStreamEvent,
    JsonStreamScanner,
} from "cap:prompt";

fn demoJsonStreamScanner() {
    let sc = JsonStreamScanner.init();
    let evs: [16]JsonStreamEvent = [JsonStreamEvent{}; 16];
    let evs_s: []JsonStreamEvent = evs[0..];

    // 模拟 LLM Token 分片流式到达
    * = sc.feed("{\"tool\":\"search\",\"q\":\"hello ", evs_s); // 已发射 key_ready("q") + string_delta("hello ")
    * = sc.feed("world\",\"limit\":10}", evs_s);              // 发射 string_delta("world") + field_complete + object_complete
    * = sc.is_complete(); // true
}
```

### 14. 零分配 RAG 回答引用解析与忠实度/幻觉溯源校验引擎 (`citations.cp`)

面向 Perplexity / 企业级 RAG 问答与 AI Agent 输出校验的 100% 纯 CP 零动态堆分配内联引用解析、越界幻觉拦截与源文词元忠实度门禁内核：

- **常量与数据模型**：
  - `CITATION_MAX_REFS` (`64`)、`CITATION_MAX_SENTENCES` (`32`)
  - `CitationRefSpan`（`source_id_1based: u32`, `start_byte`, `end_byte`, `in_bounds: bool`）
  - `CitationSentenceAudit`（`start_byte`, `end_byte`, `citation_count`, `valid_citations`, `invalid_citations`, `support_overlap_bps: u16` [`0..=10000`], `is_grounded: bool`）
  - `CitationGateConfig`（`min_support_overlap_bps` [默认 `3500` 即 35%], `min_token_len` [默认 `3`], `allow_uncited_sentences`, `reject_out_of_bounds`；提供 `defaults()` 与 `isValid()`）
  - `CitationAuditReport`（`total_sentences`, `cited_sentences`, `grounded_sentences`, `weak_grounding_sentences`, `valid_citation_refs`, `out_of_bounds_refs`, `avg_support_bps`, `passed_gate`）
- **核心纯函数**：
  - `extractInlineCitations(answer, source_count, out_refs): usize`：零堆解析 `[1]`、`[2, 3]`、`[1][4]` 等 1-based 数字引用，自动忽略 `[note]`/`[todo]`/`[]`/`[1a]`/`[1 2]` 等非引用方括号，精确标记 `[0]` 与 `> source_count` 为 `in_bounds = false`。
  - `computeClaimSupportBps(claim, source, min_token_len): u16`：零堆 ASCII 大小写折叠去重分词，自动跳过内联引用标记，计算 `claim` 内容词元在 `source` 中的覆盖基点 `[0, 10000]`。
  - `auditRagCitations(answer, sources, cfg, out_sentences): CitationAuditReport`：自动分句（保留小数点 `3.14` 与紧贴句尾的 `.[1]`）、逐句提取引用并对所引用全部有效源文计算联合词元覆盖度，生成逐句审计明细与全局 `passed_gate` 门禁裁决。

```cp
import {
    CitationGateConfig,
    CitationSentenceAudit,
    auditRagCitations,
} from "cap:prompt";

fn demoRagCitationGrounder() {
    const sources: [2][]const u8 = [
        "CP compiles directly to native Zig state machines without garbage collection.",
        "The BEAM scheduler preemptively schedules millions of concurrent AI agents.",
    ];
    const answer = "CP compiles to native Zig state machines [1]. The BEAM scheduler manages concurrent agents [2].";

    let audits: [8]CitationSentenceAudit = [CitationSentenceAudit{}; 8];
    let audits_s: []CitationSentenceAudit = audits[0..];
    const report = auditRagCitations(answer, sources[0..], CitationGateConfig.defaults(), audits_s);
    * = report; // passed_gate == true, grounded_sentences == 2, out_of_bounds_refs == 0
}
```

### 15. 零分配多段优先级 Prompt Token 预算打包与 Middle-Out 裁剪引擎 (`budgetPack.cp`)

面向长上下文 AI Agent（系统指令 + Tool Schema + RAG 检索块 + 对话历史 + 当前用户输入）的 100% 纯 CP 零动态堆分配 Token 预算规划与 Middle-Out 裁剪内核（有效缓解长上下文 "Lost in the Middle" 注意力衰减）：

- **常量与枚举**：
  - `BUDGET_PACKER_MAX_ITEMS` (`64`)
  - `BudgetItemKind`（`system_pinned | tool_schema | rag_context | history_turn | user_query`）
  - `BudgetTruncatePolicy`（`must_keep | drop_whole | keep_head | keep_tail | keep_middle_out`）
- **数据模型**：
  - `BudgetPackConfig`（`max_context_tokens`, `reserved_output_tokens`, `ellipsis_overhead_tokens` [默认 `4`]；提供 `effectiveBudget()` 与 `isValid()`）
  - `BudgetedPromptItem`（`item_id`, `kind`, `priority`, `token_count`, `min_tokens`, `policy`, `recency_seq`）
  - `PackedSlotDecision`（`item_id`, `included`, `allocated_tokens`, `truncated`, `head_tokens`, `tail_tokens`）
  - `BudgetPackReport`（`effective_budget`, `allocated_total`, `remaining_tokens`, `included_count`, `truncated_count`, `dropped_count`, `pinned_overflow`）
- **核心纯函数**：
  - `packPromptWithinBudget(items, cfg, out_decisions): BudgetPackReport`：两阶段零堆分配预算规划——第一阶段优先锁定全部 `must_keep` 项（若超预算置 `pinned_overflow = true` 且仍不越界）；第二阶段按 `(priority 降序, recency_seq 降序, 输入索引 升序)` 确定性排序并依次打包或按策略执行 `keep_head` / `keep_tail` / `keep_middle_out`（对半切分头尾并扣除 `ellipsis_overhead_tokens`），所有决策严格按输入原序写出至 `out_decisions`。

```cp
import {
    BudgetItemKind,
    BudgetTruncatePolicy,
    BudgetPackConfig,
    BudgetedPromptItem,
    PackedSlotDecision,
    packPromptWithinBudget,
} from "cap:prompt";

fn demoTokenBudgetPacker() {
    const items: [3]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.system_pinned, priority: 100, token_count: 200, min_tokens: 200, policy: BudgetTruncatePolicy.must_keep, recency_seq: 1 },
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.rag_context,   priority: 50,  token_count: 500, min_tokens: 40,  policy: BudgetTruncatePolicy.keep_middle_out, recency_seq: 2 },
        BudgetedPromptItem{ item_id: 3, kind: BudgetItemKind.user_query,    priority: 100, token_count: 100, min_tokens: 100, policy: BudgetTruncatePolicy.must_keep, recency_seq: 3 },
    ];
    const cfg = BudgetPackConfig{ max_context_tokens: 500, reserved_output_tokens: 100, ellipsis_overhead_tokens: 4 };
    let decisions: [3]PackedSlotDecision = [PackedSlotDecision{}; 3];
    let dec_s: []PackedSlotDecision = decisions[0..];

    const rep = packPromptWithinBudget(items[0..], cfg, dec_s);
    // effective_budget == 400; system(200) + user(100) 锁定，rag 获剩余 100 -> head=48, tail=48, ellipsis=4
    * = rep;
}
```

### 16. 零分配 Prompt 输入输出合规校验与敏感凭证脱敏扫描器 (`guardrail.cp`)

面向 AI Agent 输入/输出合规边界校验与敏感凭证脱敏的 100% 纯 CP 零动态堆分配扫描与脱敏引擎：

- **常量与枚举**：
  - `GUARDRAIL_MAX_FINDINGS = 64`、`GUARDRAIL_MAX_CUSTOM_TERMS = 16`、`GUARDRAIL_MAX_TERM_LEN = 64`
  - `GuardrailCategory`（`email | api_secret | jwt_token | payment_card | private_ipv4 | control_anomaly | custom_term | length_exceeded`）
  - `GuardrailAction`（`allow | redact | block`）
- **数据模型与纯函数**：
  - `GuardrailFinding`（`category`, `start`, `end`, `weight`）、`GuardrailPolicy`（`defaults()` / `isValid()`，可配置最大字节上限、控制/零宽字符占比阈值 `max_control_ratio_bps` 与各类规则权重）、`GuardrailScanReport`
  - `luhnCheckDigits(digits): bool`：标准 Luhn 校验（13..19 位数字）。
  - `redactSensitiveSpans(input, findings, out_buf: *[]u8): usize?`：零分配脱敏重写，自动排序并合并重叠区间（保留高权重类别标签），替换为 `[REDACTED:EMAIL]`、`[REDACTED:SECRET]`、`[REDACTED:JWT]`、`[REDACTED:CARD]`、`[REDACTED:PRIVATE_IP]`、`[REDACTED:TERM]`；缓冲区不足安全返回 `null`。
- **核心引擎 `PromptGuardrailScanner`**：
  - `init(policy)` / `addCustomTerm(*_, term)`（大小写不敏感自定义词组）
  - `scan(input, out_findings: *[]GuardrailFinding): GuardrailScanReport`：识别 Email、API 凭证前缀（`sk-` / `ghp_` / `AKIA` / `Bearer `）、JWT 三段式令牌、Luhn 校验银行卡号、RFC 1918/回环私有 IPv4、控制/UTF-8 零宽字符异常比例与超长截断，聚合风险分值输出 `allow | redact | block`。

```cp
import {
    GuardrailPolicy,
    GuardrailFinding,
    redactSensitiveSpans,
    PromptGuardrailScanner,
} from "cap:prompt";

fn demoGuardrailScanner() {
    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    const text = "Contact ops@example.com from 10.0.1.25";
    let findings: [16]GuardrailFinding = [GuardrailFinding{}; 16];
    let f_s: []GuardrailFinding = findings[0..];

    const rep = scanner.scan(text, *f_s);
    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    * = redactSensitiveSpans(text, findings[0..rep.finding_count], *out_s);
    // "Contact [REDACTED:EMAIL] from [REDACTED:PRIVATE_IP]"
}
```


### 17. 零分配编译型 Prompt 模板字节码引擎 (`compiler.cp`)

面向 AI Agent 系统提示词与动态上下文编排的 100% 纯 CP 零动态堆分配编译型模板引擎，支持单遍词法编译为紧凑字节码、线性零堆执行与静态元数据内省：

- **常量与枚举**：
  - `TPL_MAX_INSTRUCTIONS = 128`、`TPL_MAX_NESTING_DEPTH = 16`、`TPL_MAX_CONTEXT_VARS = 32`、`TPL_MAX_CONTEXT_LISTS = 16`、`TPL_MAX_LIST_ITEMS = 32`、`TPL_MAX_MISSING_NAME = 64`
  - `TplOpKind`（`literal | var_lookup | if_jump_false | else_jump_end | each_begin | each_end`）
  - `TplCompileStatus`（`ok | unclosed_tag | empty_tag | mismatched_close_tag | unclosed_block | nesting_too_deep | instruction_overflow`）
  - `TplExecStatus`（`ok | invalid_template | missing_variable | buffer_too_small | runtime_stack_error`）
- **数据模型**：
  - `TplInstruction`、`CompiledPromptTemplate`（`isValid()`, `error_offset`, `inst_count`, `max_depth_seen`）
  - `TplRenderContext`（`init`, `setVar`, `setBool`, `setList`, `getVar`, `isTruthy`）
  - `TplExecResult`（`status`, `bytes_written`, `substitutions`, `missingVarSlice()`）
- **核心纯函数**：
  - `compilePromptTemplate(src): CompiledPromptTemplate`：单遍编译 `{{name}}`、`{{name|fallback}}`、嵌套 `{{#if flag}}...{{else}}...{{/if}}` 与 `{{#each items}}...{{@index}}...{{.}}...{{/each}}`。
  - `renderCompiledTemplate(tpl, ctx, strict_missing, out_buf: *[]u8): TplExecResult`：线性零堆执行字节码；严格模式拦截缺失变量并报告变量名，宽松模式输出空串或默认值。
  - `collectRequiredVars(tpl, out_names)` / `collectAllReferencedVars(tpl, out_names)`：编译后零扫描静态提取全部唯一引用变量名（自动排除 `.` 与 `@index`）。

```cp
import {
    TplRenderContext,
    compilePromptTemplate,
    renderCompiledTemplate,
} from "cap:prompt";

fn demoPromptCompiler() {
    const tpl = compilePromptTemplate(
        "Hello {{user}}, role={{role|assistant}}.{{#if tools}} Tools:{{#each tools}} [{{@index}}]{{.}}{{/each}}{{/if}}",
    );
    let ctx = TplRenderContext.init();
    * = ctx.setVar("user", "Alice");
    const tools: [2][]const u8 = ["search", "calc"];
    * = ctx.setList("tools", tools[0..]);

    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    const res = renderCompiledTemplate(tpl, ctx, true, *out_s);
    * = res.bytes_written; // "Hello Alice, role=assistant. Tools: [0]search [1]calc"
}
```


### 18. 零分配流式 Chain-of-Thought 多通道标签分流与思考预算守卫 (`cotParser.cp`)

面向推理模型（DeepSeek-R1、Qwen3、Claude、o 系列）的 100% 纯 CP 零动态堆分配流式 CoT 标签解析与思考预算守卫引擎，支持跨任意分片（含单字节切片）精准分流与代码围栏防误触发：

- **标签族位掩码与枚举**：
  - `COT_TAG_THINK` (`1`)、`COT_TAG_THINKING` (`2`)、`COT_TAG_REFLECTION` (`4`)、`COT_TAG_ANSWER` (`8`)、`COT_TAG_ALL` (`15`)
  - `CotSegmentKind`（`thinking | answer | code_fence | budget_exceeded`）
- **数据模型**：
  - `CotParserConfig`（`tag_mask`、`max_thinking_bytes`、`respect_code_fences`；提供 `defaults()` / `isValid()`）
  - `CotSegmentEvent`（零拷贝 `out_buf` 区间 `kind`, `start`, `len`）
  - `CotFeedResult`（`bytes_consumed`, `bytes_written`, `events_emitted`, `budget_exceeded`, `needs_drain`）
  - `CotParseSummary`（`thinking_bytes`, `estimated_thinking_tokens`, `dropped_thinking_bytes`, `answer_bytes`, `code_fence_bytes`, `thinking_blocks`, `answer_blocks`, `code_fence_blocks`, `unclosed_tags_repaired`, `orphan_close_tags`, `nested_open_tags`, `ignored_code_fence_tags`, `thinking_budget_exceeded`）
- **核心引擎 `CotStreamParser`**：
  - `init(config)` / `reset(*_)` / `currentKind()` / `isBudgetExceeded()` / `summary()`
  - `feed(*_, chunk, *out_buf, *out_events): CotFeedResult`：增量消费任意分片，处理跨分片开/闭标签与 ` ``` ` 代码围栏（围栏内伪 `<think>` 自动忽略并计数）；超出 `max_thinking_bytes` 时触发 `budget_exceeded` 信号并丢弃超额思考字节；输出缓冲满时通过 `needs_drain` + `bytes_consumed` 支持零丢字节分批排空。
  - `finish(*_, *out_buf, *out_events): CotFeedResult`：排空末尾残留缓冲并自动修复未闭合标签。

```cp
import {
    COT_TAG_ALL,
    CotParserConfig,
    CotSegmentEvent,
    CotStreamParser,
} from "cap:prompt";

fn demoCotStreamParser() {
    const cfg = CotParserConfig{ tag_mask: COT_TAG_ALL, max_thinking_bytes: 256, respect_code_fences: true };
    let parser = CotStreamParser.init(cfg) ?? return;

    let out_buf: [256]u8 = [0; 256];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [16]CotSegmentEvent = [CotSegmentEvent{}; 16];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    * = parser.feed("<thi", *out_s, *ev_s);
    * = parser.feed("nk>plan</think>Final answer", *out_s, *ev_s);
    * = parser.finish(*out_s, *ev_s);
    * = parser.summary();
}
```


### 19. 零分配 LLM 流式工具调用提取与状态机解析器 (`toolCallParser.cp`)

面向大模型流式推理（Qwen/Hermes `<tool_call>`、Markdown ```` ```json ```` 代码块与裸 JSON 工具调用）的 100% 纯 CP 零动态堆分配增量提取状态机：

- **常量与枚举**：
  - `TOOL_CALL_MAX_CALLS = 16`、`TOOL_CALL_NAME_MAX_LEN = 64`、`TOOL_CALL_ID_MAX_LEN = 64`、`TOOL_CALL_ARGS_MAX_LEN = 512`、`TOOL_CALL_TEXT_MAX_LEN = 1024`、`TOOL_CALL_WORK_BUF_LEN = 1024`
  - `ToolCallStatus`（`complete | truncated | malformed`）
  - `ToolCallWrapperKind`（`xml_tag | code_fence | bare_json`）
- **数据模型**：
  - `ParsedToolCall`（`nameSlice()`, `idSlice()`, `argsJsonSlice()`, `status`, `wrapper`）
  - `ToolCallParserStats`（`bytes_fed`, `text_bytes_emitted`, `text_bytes_dropped`, `total_calls`, `complete_calls`, `truncated_calls`, `malformed_calls`, `dropped_calls`）
- **核心引擎 `ToolCallStreamParser`**：
  - `init()` / `reset(*_)`
  - `feed(*_, chunk: []const u8)` / `finish(*_): ToolCallParserStats`：支持任意分片（低至单字节），内置括号/字符串/转义深度状态机（免疫引号内花括号、转义引号、伪闭合标签与多字节 UTF-8），自动分离自然语言文本与单回合多工具并发调用，并在流截断时抢救已解析工具名标记 `truncated`。
  - `textSlice()` / `toolCalls()` / `stats()`。

```cp
import {
    ToolCallStatus,
    ToolCallStreamParser,
} from "cap:prompt";

fn demoToolCallStreamParser() {
    let parser = ToolCallStreamParser.init();
    parser.feed("Checking weather... <tool_call>{\"id\":\"c1\",\"name\":\"get_weather\",");
    parser.feed("\"arguments\":{\"city\":\"Tokyo\"}}</tool_call> Done.");
    const st = parser.finish();
    if (st.complete_calls == 1) {
        const call = parser.toolCalls()[0];
        * = call.nameSlice();     // "get_weather"
        * = call.argsJsonSlice(); // "{\"city\":\"Tokyo\"}"
        * = parser.textSlice();   // "Checking weather...  Done."
        * = (call.status == ToolCallStatus.complete);
    }
}
```
