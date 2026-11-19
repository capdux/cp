# CP 标准库：SIMD 零拷贝 JSON 解析与强类型反序列化手册 (`cap:json`)

## 一、 架构总览与设计准则

`cap:json` 是 CP 语言官方内置的高性能 JSON 数据交换标准库，对照 Bun 生产级数据交换模块（`/data/dev/bun/src/interchange/`）构建，采用 **16 字节 SIMD 扫描 + 零拷贝切片借用 + 编译期强类型直译** 三位一体架构。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├──────────────────────────────┬─────────────────────────────────────────┤
│   动态零拷贝 DOM 引擎         │     编译期强类型反序列化引擎             │
│  JSON.parse / JsonParser     │   JSON.parseTyped(T, alloc, src)        │
│  JsonValue / JsonField       │   @typeInfo 静态字段反射 + skipValue     │
├──────────────────────────────┴─────────────────────────────────────────┤
│   FastJsonScanner (16B @Vector(16, u8) SIMD 引号/转义符极速扫描)        │
├────────────────────────────────────────────────────────────────────────┤
│   JsonBufferPool (线程局部 4KB 缓冲池，无锁复用，零跨线程竞争)          │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/json/index.cp` | `// Source: /data/dev/bun/src/interchange/interchange.cp` | `cap:json` 统一导出入口，`JSON` 门面类与全局 `json` 实例 |
| `lib/json/value.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonTag`、`JsonField`、`JsonValue` 零拷贝动态值 AST 与安全访问器 |
| `lib/json/parser.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `FastJsonScanner` SIMD 扫描器、`JsonError`、`JsonParser` 与 `fastParseType` |
| `lib/json/pool.cp` | `// Source: /data/dev/bun/src/interchange/json.cp` | `JsonBufferNode` 与 `JsonBufferPool` 线程局部 4096 字节复用缓冲池 |

### 2. 核心设计优势

1. **零拷贝字符串借用（Zero-Copy Slice Borrowing）**：
   - `JsonValue` 中的字符串键（`JsonField.key`）和字符串值（`JsonValue.str_val`）均以 `[]const u8` 切片直接指向原始输入缓冲区，解析阶段不发生额外堆字符串复制。
2. **16 字节 SIMD 引号/转义扫描**：
   - `FastJsonScanner.indexOfQuoteOrEscape` 使用 `@Vector(16, u8)` 一次并行比对 16 个字节的 `"` 与 `\`，并通过 `@bitCast` + `@ctz` 单指令定位首个命中偏移。
3. **编译期强类型直译 + 未知字段零分配跳过**：
   - `JSON.parseTyped` 根据目标结构体 `T` 在编译期展开字段匹配逻辑，支持基础标量、可选类型、`[]const u8`、切片数组、嵌套结构体与字段默认值；遇到未知字段时通过 `skipValue` 原地跳过，不分配任何中间 AST 节点。
4. **线程局部缓冲池复用**：
   - `JsonBufferPool.acquire` 与 `JsonBufferPool.recycle` 基于线程局部侵入式单向链表缓存 `4096` 字节节点，避免高频请求反复向系统分配器申请与归还内存。

---

## 二、 核心 API 详解

### 1. `JSON` 门面与 `JsonParser`

| API 签名 | 说明 |
| :--- | :--- |
| `JSON.parse(allocator, src): !JsonValue` | 将 UTF-8 JSON 字节切片解析为零拷贝 `JsonValue` 动态 AST |
| `JSON.parseTyped(comptime T, allocator, src): !T` | 编译期直接反序列化为强类型结构体/切片/标量，自动跳过未声明字段并保留字段默认值 |
| `JSON.stringify(obj): string` | 将 CP 值序列化为 JSON 字符串 |
| `JsonParser.init(allocator, src): JsonParser` | 创建流式 JSON 解析器游标 |
| `parser.parseValue(): !JsonValue` | 从当前游标位置解析单个任意 JSON 值 |
| `parser.parseString(): ![]const u8` | SIMD 加速解析字符串字面量（零拷贝切片） |
| `parser.parseNumber(): !f64` | 解析浮点或整数数字 |
| `parser.skipValue(): !void` | 零分配快速跳过当前位置的任意 JSON 复合或标量值 |

### 2. `JsonValue` 与 `JsonTag`

- `JsonTag` 枚举：`Null`、`Bool`、`Number`、`String`、`Array`、`Object`。
- 类型判断：`isNull()`、`isBool()`、`isNumber()`、`isString()`、`isArray()`、`isObject()`。
- 安全拆包：`asBool(): bool?`、`asNumber(): f64?`、`asString(): ([]const u8)?`、`asArray(): ([]JsonValue)?`、`asObject(): ([]JsonField)?`。
- 容器访问：`len(): usize`、`get(key: []const u8): JsonValue?`、`getIndex(idx: usize): JsonValue?`。

### 3. `FastJsonScanner` 与 `JsonBufferPool`

| API 签名 | 说明 |
| :--- | :--- |
| `FastJsonScanner.indexOfQuoteOrEscape(input): usize?` | 16 字节 SIMD 查找首个 `"` 或 `\` 的字节索引 |
| `JsonBufferPool.acquire(allocator): *JsonBufferNode` | 从当前线程缓冲池获取一个 4096 字节缓冲节点 |
| `JsonBufferPool.recycle(node: *JsonBufferNode): void` | 将缓冲节点归还至当前线程局部链表以便下次复用 |

### 4. 错误集 (`JsonError`)

`UnexpectedEof`、`InvalidToken`、`ExpectedColon`、`ExpectedComma`、`ExpectedBrace`、`UnknownField`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { JSON, JsonValue, JsonTag, FastJsonScanner, JsonBufferPool } from "cap:json";

pub struct AgentConfig {
    id: i64 = 0,
    name: []const u8 = "",
    enabled: bool = false,
    weight: f64 = 1.0,
    tags: [][]const u8 = [],
    tier: []const u8 = "standard",
}

pub fn demoJson(): !void {
    const allocator = std.heap.page_allocator;

    // 1. 零拷贝动态 DOM 解析
    const raw = "{\"name\":\"cp_agent\",\"active\":true,\"scores\":[10,20.5,30]}";
    const root = try JSON.parse(allocator, raw);

    if (name_val := root.get("name")) {
        if (s := name_val.asString()) {
            // s 直接借用自 raw 内存切片，无堆拷贝
            @print("name len = ");
        }
    }

    if (scores := root.get("scores")) {
        if (first := scores.getIndex(0)) {
            const v = first.asNumber().?;
            _ = v;
        }
    }

    // 2. 强类型结构体直译（自动忽略 unknown_meta 并保留 tier 默认值）
    const cfg_json = "{\"id\":42,\"name\":\"worker-1\",\"enabled\":true,\"weight\":2.5,\"tags\":[\"fast\",\"simd\"],\"unknown_meta\":{\"skip\":true}}";
    let cfg = AgentConfig{};
    zig {
        cfg = try JSON.parseTyped(AgentConfig, allocator, cfg_json);
    }

    // 3. 线程局部 4KB 缓冲池复用
    const node = JsonBufferPool.acquire(allocator);
    defer JsonBufferPool.recycle(node);
    node.data[0] = '{';
}
```

---

## 四、 与 Bun `src/interchange/` 对照表

| 功能分类 | Bun (`src/interchange/json.cp` & JS Runtime) | CP 标准库 (`cap:json`) |
| :--- | :--- | :--- |
| **动态 JSON 解析** | `JSON.parse(text)` / `interchange/json.cp` | `JSON.parse(allocator, src)` -> `!JsonValue` |
| **零拷贝字符串扫描** | SIMD 字符串扫描器 | `FastJsonScanner.indexOfQuoteOrEscape(input)` |
| **强类型结构体解析** | `std.json.parseFromSliceLeaky(T, ...)` | `JSON.parseTyped(T, allocator, src)` |
| **未知字段跳过** | AST 丢弃 / 流式跳过 | `JsonParser.skipValue()`（零堆分配） |
| **临时缓冲池** | `HashMapPool` / Thread-local 缓冲复用 | `JsonBufferPool.acquire()` / `JsonBufferPool.recycle()` |
| **JSON 序列化** | `JSON.stringify(value)` | `JSON.stringify(obj)` |
