# CP 标准库：RFC 8259 SIMD JSON 解析、序列化与 JSON Pointer 手册 (`cap:json`)

## 一、 架构总览与设计准则

`cap:json` 是 CP 语言官方内置的高性能 JSON 数据交换标准库，对照 Bun 生产级数据交换模块（`/data/dev/bun/src/interchange/`）构建，实现 **16 字节 SIMD 引号/转义/控制字符扫描 + 零拷贝切片借用 + 严格 RFC 8259 校验 + UTF-16 代理对解码 + 紧凑/美化序列化 + RFC 6901 JSON Pointer** 全栈能力。

```text
┌───────────────────────────────────────────────────────────────────────────┐
│                         CP 用户层应用 (Application)                        │
├──────────────────────────┬─────────────────────────┬──────────────────────┤
│   RFC 8259 解析器         │   RFC 6901 & 深度比对    │   RFC 8259 序列化器   │
│  JSON.parse(alloc, src)  │  val.pointer("/a/0/b")  │  JSON.stringifyValue │
│  JSON.parseWithOptions   │  JSON.pointer(val, ptr) │  JSON.stringifyPretty│
│  JSON.parseTyped(T, ...) │  val.equals(other)      │  JsonSerializer      │
├──────────────────────────┴─────────────────────────┴──────────────────────┤
│   FastJsonScanner (16B @Vector(16, u8) 扫描 '"' / '\\' / 控制字符 < 0x20)  │
├───────────────────────────────────────────────────────────────────────────┤
│   JsonBufferPool (线程局部 4KB 缓冲池，无锁复用，零跨线程竞争)               │
└───────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/json/index.cp` | `// Source: /data/dev/bun/src/interchange/interchange.cp` | `cap:json` 统一导出入口，`JSON` 门面类与全局 `json` 实例 |
| `lib/json/value.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonTag`、`JsonField`、`JsonValue`、RFC 6901 `pointer()` 与深度比较 `equals()` |
| `lib/json/parser.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `FastJsonScanner`、`JsonParseOptions`、`JsonError`、`JsonParser` 与 `fastParseType` |
| `lib/json/serializer.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonSerializer` 紧凑与缩进美化序列化器（完整控制字符与引号转义） |
| `lib/json/pool.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonBufferNode` 与 `JsonBufferPool` 线程局部 4096 字节复用缓冲池 |

### 2. RFC 8259 & RFC 6901 核心特性

1. **零拷贝快路径 + 完整转义与 UTF-16 代理对解码**：
   - 无转义字符的字符串直接以 `[]const u8` 切片借用原输入缓冲区（零堆分配）；
   - 包含转义的字符串自动解码 `\"`、`\\`、`\/`、`\b`、`\f`、`\n`、`\r`、`\t`、`\uXXXX` 以及 UTF-16 代理对（如 `\uD83D\uDE00` -> 4 字节 UTF-8 Emoji），同时拒绝孤立代理项与非法转义。
2. **严格 RFC 8259 语法守卫与嵌套深度防护**：
   - 严格校验数字语法 `-? (0 | [1-9][0-9]*) (\.[0-9]+)? ([eE][+-]?[0-9]+)?`，拒绝前导零（`01`）、前导 `+` 号（`+1`）、尾随逗号（`[1,]` / `{"a":1,}`）、未闭合字符串、未转义控制字符（`< 0x20`）及尾部多余字符；
   - 通过 `JsonParseOptions{ max_depth: N }`（默认 `512`）防止超深嵌套导致栈溢出。
3. **RFC 6901 JSON Pointer & 深度相等比较**：
   - `val.pointer("/users/0/name")` 与 `val.pointer("/a~1b/c~0d")` 支持 `~1` -> `/` 与 `~0` -> `~` 单遍解码，并严格校验数组索引语法；
   - `val.equals(other)` 支持对象键序无关的深度结构对比。

---

## 二、 核心 API 详解

| API 签名 | 说明 |
| :--- | :--- |
| `JSON.parse(allocator, src): JsonValue!` | 严格 RFC 8259 解析为动态 `JsonValue` AST |
| `JSON.parseWithOptions(allocator, src, opts): JsonValue!` | 带 `JsonParseOptions`（如 `max_depth`）解析 |
| `JSON.parseTyped(comptime T, allocator, src): T!` | 编译期强类型直译反序列化 |
| `JSON.stringifyValue(allocator, val): ([]u8)!` | 将 `JsonValue` 序列化为紧凑 JSON |
| `JSON.stringifyPretty(allocator, val, indent): ([]u8)!` | 将 `JsonValue` 序列化为带缩进美化的 JSON |
| `val.pointer(ptr: []const u8): JsonValue?` | RFC 6901 JSON Pointer 路径查询 |
| `val.equals(other: JsonValue): bool` | 深度结构相等比较 |

### 错误集 (`JsonError`)
`UnexpectedEof`、`InvalidToken`、`ExpectedColon`、`ExpectedComma`、`ExpectedBrace`、`UnknownField`、`MaxDepthExceeded`、`TrailingComma`、`InvalidEscape`、`InvalidUnicode`、`InvalidControlChar`、`TrailingCharacters`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { JSON, JsonParseOptions } from "cap:json";

pub fn demoJsonDeep(): !void {
    const allocator = std.heap.page_allocator;
    const raw = "{\"meta\":{\"emoji\":\"\\uD83D\\uDE00\",\"a/b\":\"ok\"},\"items\":[1,2]}";

    const doc = try JSON.parseWithOptions(allocator, raw, JsonParseOptions{ max_depth: 64 });
    const emoji = doc.pointer("/meta/emoji").?.asString().?; // "😀"
    const slash_val = doc.pointer("/meta/a~1b").?.asString().?; // "ok"
    * = emoji;
    * = slash_val;

    const pretty = try JSON.stringifyPretty(allocator, doc, 2);
    defer allocator.free(pretty);
}
```
