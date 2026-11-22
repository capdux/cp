# CP 标准库：RFC 8259/6901/6902/7396/8785 JSON 与 NDJSON 流式协议手册 (`cap:json`)

> 参考来源：`/data/dev/bun/docs/runtime/jsonl.mdx`, `/data/dev/bun/src/interchange/` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:json` 是 CP 语言官方内置的高性能 JSON 数据交换标准库，对照 Bun 生产级数据交换模块（`/data/dev/bun/src/interchange/`）构建，实现 **16 字节 SIMD 引号/转义/控制字符扫描 + 零拷贝切片借用 + 严格 RFC 8259 校验 + UTF-16 代理对解码 + RFC 6901 JSON Pointer + RFC 6902 JSON Patch + RFC 7396 Merge Patch + RFC 8785 Canonical/Pretty 序列化 + NDJSON/JSONL 流式读写** 全栈能力。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                          CP 用户层应用 (Application)                        │
├────────────────────┬───────────────────────┬───────────────┬───────────────┤
│  RFC 8259 解析器    │  RFC 6901/6902/7396   │ RFC 8259/8785 │ NDJSON/JSONL  │
│  JSON.parse        │  JsonPointer          │ JsonSerializer│ NdjsonReader  │
│  JSON.parseTyped   │  JsonPatch / Merge    │ Canonical/Opt │ NdjsonWriter  │
├────────────────────┴───────────────────────┴───────────────┴───────────────┤
│   FastJsonScanner (16B @Vector(16, u8) 扫描 '"' / '\\' / 控制字符 < 0x20)   │
├────────────────────────────────────────────────────────────────────────────┤
│   JsonBufferPool (线程局部 4KB 缓冲池，无锁复用，零跨线程竞争)                │
└────────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/json/index.cp` | `// Source: /data/dev/bun/src/interchange/interchange.cp` | `cap:json` 统一导出入口，`JSON` 门面类与全局 `json` 实例 |
| `lib/json/value.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonTag`、`JsonField`、`JsonValue`、RFC 6901 `pointer()` 与深度比较 `equals()` |
| `lib/json/parser.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `FastJsonScanner`、`JsonParseOptions`、`JsonError`、`JsonParser` 与 `fastParseType` |
| `lib/json/pointer.cp` | `// Source: cp/lib/json/pointer.cp (RFC 6901 JSON Pointer & RFC 6902 JSON Patch)` | `JsonPointer`（转义/解析/求值）、`JsonPatch`（6 种原子操作 + `test` 断言）、`JsonMergePatch`（RFC 7396） |
| `lib/json/serializer.cp` | `// Source: cp/lib/json/serializer.cp (RFC 8259 Pretty & RFC 8785 Canonical JSON Serializer)` | `JsonSerializeOptions`、`JsonSerializer`（紧凑、缩进美化、HTML 安全转义、RFC 8785 规范排序） |
| `lib/json/ndjson.cp` | `// Source: cp/lib/json/ndjson.cp (NDJSON / JSON Lines Streaming Reader & Writer)` | `NdjsonReader`（跨 chunk 增量解析、行号统计、单行长度 DoS 防护）、`NdjsonWriter` |
| `lib/json/pool.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonBufferNode` 与 `JsonBufferPool` 线程局部 4096 字节复用缓冲池 |

### 2. 核心协议特性

1. **零拷贝快路径 + 完整转义与 UTF-16 代理对解码**：
   - 无转义字符的字符串直接以 `[]const u8` 切片借用原输入缓冲区（零堆分配）；
   - 包含转义的字符串自动解码 `\"`、`\\`、`\/`、`\b`、`\f`、`\n`、`\r`、`\t`、`\uXXXX` 以及 UTF-16 代理对（如 `\uD83D\uDE00` -> 4 字节 UTF-8 Emoji），同时拒绝孤立代理项与非法转义。
2. **严格 RFC 8259 语法守卫与嵌套深度防护**：
   - 严格校验数字语法 `-? (0 | [1-9][0-9]*) (\.[0-9]+)? ([eE][+-]?[0-9]+)?`，拒绝前导零（`01`）、前导 `+` 号（`+1`）、尾随逗号（`[1,]` / `{"a":1,}`）、未闭合字符串、未转义控制字符（`< 0x20`）及尾部多余字符；
   - 通过 `JsonParseOptions{ max_depth: N }`（默认 `512`）防止超深嵌套导致栈溢出。
3. **RFC 6901 JSON Pointer、RFC 6902 JSON Patch 与 RFC 7396 Merge Patch**：
   - `JsonPointer` 提供 `escapeToken`/`unescapeToken`（`~1` <-> `/`，`~0` <-> `~` 单遍精确顺序）、`parseTokens`/`format`/`resolve`，拒绝数组前导零（如 `"01"`）；
   - `JsonPatch.apply` 原生支持 `add`/`remove`/`replace`/`move`/`copy`/`test` 原子补丁操作与深拷贝隔离（任意步骤失败时不污染原文档），并禁止将节点 `move` 到自身子路径；
   - `JsonMergePatch.apply` 实现 RFC 7396 递归合并，`null` 值删除目标键，非对象补丁整树替换。
4. **RFC 8259 Pretty & RFC 8785 Canonical 序列化引擎**：
   - `JsonSerializer.stringifyCanonical` 按 UTF-16 码元字典序递归排序对象键、归一化 `-0` 为 `0`、紧凑无空白输出并严格转义控制字符，适用于确定性签名与内容哈希；
   - `JsonSerializer.stringifyWithOptions` 支持 `indent_spaces`、`sort_keys` 与 `escape_html_safe`（转义 `<`、`>`、`&` 为 `\u003c`、`\u003e`、`\u0026`）。
5. **NDJSON / JSON Lines 增量流式读写**：
   - `NdjsonReader` 支持任意字节切片跨 chunk `feed(chunk)` 喂入，自动处理 `\n` 与 `\r\n` 行边界、跳过空白行、记录 `currentLine()`/`totalRecords()`、并通过 `max_line_bytes` 防止超长单行 DoS；
   - `NdjsonWriter` 将单条或批量 `JsonValue` 序列化为单行紧凑 JSON + `\n`。

---

## 二、 核心 API 详解

| API 签名 | 说明 |
| :--- | :--- |
| `JSON.parse(allocator, src): JsonValue!` | 严格 RFC 8259 解析为动态 `JsonValue` AST |
| `JSON.parseWithOptions(allocator, src, opts): JsonValue!` | 带 `JsonParseOptions`（如 `max_depth`）解析 |
| `JSON.parseTyped(comptime T, allocator, src): T!` | 编译期强类型直译反序列化 |
| `JSON.stringifyValue(allocator, val): ([]u8)!` | 将 `JsonValue` 序列化为紧凑 JSON |
| `JSON.stringifyPretty(allocator, val, indent): ([]u8)!` | 将 `JsonValue` 序列化为带缩进美化的 JSON |
| `JSON.stringifyWithOptions(allocator, val, opts): ([]u8)!` | 按 `JsonSerializeOptions` 序列化（缩进/键排序/HTML 安全转义） |
| `JSON.stringifyCanonical(allocator, val): ([]u8)!` | RFC 8785 确定性规范化 JSON 序列化 |
| `JsonPointer.escapeToken` / `unescapeToken` / `resolve` | RFC 6901 引用令牌转义、反转义与指针求值 |
| `JsonPatch.apply(allocator, doc, ops)` / `applyValue` | RFC 6902 原子补丁应用（`Add`/`Remove`/`Replace`/`Move`/`Copy`/`Test`） |
| `JsonMergePatch.apply(allocator, target, patch)` | RFC 7396 递归合并补丁（`null` 删键） |
| `NdjsonReader.init` / `feed` / `finish` / `parseAll` | NDJSON / JSONL 增量跨 chunk 流式读取器 |
| `NdjsonWriter.writeRecord` / `stringifyRecord` / `stringifyAll` | NDJSON / JSONL 单行紧凑序列化写入器 |
| `val.equals(other: JsonValue): bool` | 深度结构相等比较 |

### 错误集 (`JsonError` / `JsonPatchError` / `NdjsonError`)
- `JsonError`：`UnexpectedEof`、`InvalidToken`、`ExpectedColon`、`ExpectedComma`、`ExpectedBrace`、`UnknownField`、`MaxDepthExceeded`、`TrailingComma`、`InvalidEscape`、`InvalidUnicode`、`InvalidControlChar`、`TrailingCharacters`。
- `JsonPatchError`：`InvalidPointer`、`PathNotFound`、`IndexOutOfBounds`、`InvalidTarget`、`TestFailed`、`MissingValue`、`MissingFrom`、`CannotMoveIntoChild`、`InvalidOperation`。
- `NdjsonError`：`LineTooLong`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    JSON,
    JsonPatchOp,
    JsonPatch,
    JsonMergePatch,
    NdjsonReader,
} from "cap:json";

pub fn demoJsonRfcAndNdjson(): !void {
    const allocator = std.heap.page_allocator;
    const doc = try JSON.parse(allocator, "{\"z\":1,\"a/b\":{\"k\":\"v\",\"drop\":true}}");

    // 1. RFC 6902 JSON Patch
    let ops: [1]JsonPatchOp = [JsonPatchOp.remove("/a~1b/drop")];
    const patched = try JsonPatch.apply(allocator, doc, &ops);

    // 2. RFC 7396 Merge Patch + RFC 8785 Canonical JSON
    const merge = try JSON.parse(allocator, "{\"a/b\":{\"k\":\"updated\"}}");
    const merged = try JsonMergePatch.apply(allocator, patched, merge);
    const canon = try JSON.stringifyCanonical(allocator, merged);
    defer allocator.free(canon);

    // 3. NDJSON Streaming Reader
    let reader = NdjsonReader.init(allocator);
    defer reader.deinit();
    const r1 = try reader.feed("{\"ok\":true}\n{\"n\":");
    defer { if (r1.len > 0) allocator.free(r1); }
    const r2 = try reader.feed("42}");
    defer { if (r2.len > 0) allocator.free(r2); }
    const tail = try reader.finish();
    defer { if (tail.len > 0) allocator.free(tail); }
}
```

---

## 零分配编解码辅助子模块（收敛自原 `json_*` 系列）

- **RFC 8259 字符串转义/反转义 (`escape.cp`)**：
  - `needsJsonEscape`、`escapeJsonContent`、`quoteJsonString`、`unescapeJsonContent`。
- **JSON / JSONC 压缩器 (`minify.cp`)**：
  - `minifyJson`、`minifyJsonc`、`minifyJsoncWithOptions`。
- **RFC 6901 JSON Pointer 零分配工具 (`ptr.cp`)**：
  - `isValidPointer`、`parseArrayIndex`、`unescapeToken`、`escapeToken`、`PointerIter`、`appendToken`、`parentPointer`。

---

## 上游 Bun 行为契约与安全边界注意事项

1. **NDJSON / JSONL 分块流式边界（参考 `/data/dev/bun/docs/runtime/jsonl.mdx`）**：
   - `NdjsonReader` 对标上游 `Bun.JSONL.parseChunk` 增量流语义：当网络分片在 JSON 对象中间截断时，`feed(chunk)` 仅产出完整已闭合的记录并将半行缓冲在内部，最后在流结束时必须调用 `finish()` 冲刷尾部无换行符的最后一条记录；单行超长输入内置长度上限保护以防止内存耗尽。
2. **严格 RFC 8259 vs JSONC 预处理分离**：
   - `JSON.parse` / `JsonParser.parse` 严格遵循 RFC 8259（拒绝注释、尾随逗号与非法控制字符 `< 0x20`），并支持通过 `JsonParseOptions` 限制最大嵌套深度防止递归栈溢出；需要处理带注释的配置文件时，先使用 `minifyJsonc` / `minifyJsoncWithOptions` 剥离注释。
3. **JSON Pointer 与 Patch 原子性（RFC 6901 / 6902）**：
   - RFC 6901 转义序列严格按顺序解码：`~1` 先转为 `/`，再将 `~0` 转为 `~`（`unescapeToken`）；数组索引拒绝前导零（如 `"01"` 非法，仅 `"0"` 或正整数合法）；`JsonPatch` 中的 `test` 操作失败时中止补丁应用。
4. **RFC 8785 规范化输出与签名场景**：
   - 需要对 JSON 载荷做哈希或密码学签名时，使用 `JSON.stringifyCanonical`（键名按字典序确定性排列、无多余空白）保证跨节点序列化字节完全一致。
