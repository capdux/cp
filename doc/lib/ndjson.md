# CP 标准库：NDJSON / JSON Lines 流式分帧、校验与写入引擎手册 (`cap:ndjson`)

## 一、 架构总览与设计准则

`cap:ndjson` 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 **NDJSON（Newline-Delimited JSON / JSON Lines）流式分帧、严格校验与安全写入引擎**（基于 `std.json` 构建）。支持任意网络/文件分块（Chunk）的半包与黏包增量解析、`\n` 与 `\r\n` 行尾自动兼容、纯空白行跳过、单行最大长度防资源耗尽守卫、以及禁止裸换行符破坏帧边界的写入校验。既支持 `import { ... } from "cap:ndjson"` 静态导入，也支持 `import cap from "cap"; cap.ndjson.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ndjson/ndjson.cp` | `// Source: cp/lib/ndjson/ndjson.cp (NDJSON / JSON Lines Streaming Framing, Validation & Writer Engine)` | `NdjsonError`、`NdjsonRecord`、`NdjsonStreamParser`、`NdjsonWriter`、`parseAll`、`stringifyAll` |
| `lib/ndjson/index.cp` | `// Source: cp/lib/ndjson/index.cp (Unified Exports for cap:ndjson)` | `cap:ndjson` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与记录结构

- **`NdjsonError`**：
  - `InvalidJsonLine`：某一行不是合法完整的 JSON 值，或写入原始行时包含未转义的 `\n` / `\r` 破坏行边界。
  - `MaxLineLengthExceeded`：单行有效载荷字节数超过 `max_line_bytes` 限制（触发资源耗尽防护）。
  - `UnterminatedFinalLine`：在严格模式（`allow_trailing_without_newline = false`）下调用 `finish`，缓冲区尾部存在未以 `\n` 结尾的非空数据。
  - `InvalidLimit`：初始化 `NdjsonStreamParser` 时传入 `max_line_bytes == 0`。
  - `OutOfMemory`：底层内存分配失败。
- **`NdjsonRecord`**：
  - 字段：`line_number: usize`（1-based 物理行号）、`raw_line: []const u8`、`parsed: std.json.Parsed(std.json.Value)`。
  - 方法：`deinit(*_)` 释放解析后的 JSON Arena 与持有的 `raw_line` 缓冲区。

### 2. 流式增量解析器 (`NdjsonStreamParser`)

- **`NdjsonStreamParser.init(allocator: std.mem.Allocator, max_line_bytes: usize): NdjsonStreamParser ! NdjsonError`**：
  初始化流式解析器并设置单行最大字节上限。若 `max_line_bytes == 0` 返回 `NdjsonError.InvalidLimit`。
- **`deinit(*_)`**：释放内部行缓冲及解析器持有的全部 `NdjsonRecord`。
- **`feed(*_, chunk: []const u8): ([]const NdjsonRecord) ! NdjsonError`**：
  喂入任意大小的数据块（支持半包跨块、单块多行黏包、`\n` 与 `\r\n` 行尾，自动跳过纯空白行），返回本次新解析出的只读记录切片；记录所有权仍归解析器管理直至 `deinit()`。
- **`nextRecord(*_): (*const NdjsonRecord)?`**：
  通过内部游标逐条拉取尚未消费的 `NdjsonRecord` 只读指针，拉取完毕返回 `null`。
- **`finish(*_, allow_trailing_without_newline: bool): ([]const NdjsonRecord) ! NdjsonError`**：
  处理流结束时缓冲区中剩余的最后一行。若 `allow_trailing_without_newline` 为 `false` 且存在非空白残余，返回 `NdjsonError.UnterminatedFinalLine`；否则将其作为最后一条记录解析并返回。
- **统计与诊断**：
  - `parsedCount(*_): usize`：已成功解析的有效 JSON 记录总数。
  - `skippedEmptyLines(*_): usize`：已跳过的纯空白行总数。
  - `lastErrorLine(*_): usize`：最近一次发生分帧或 JSON 校验错误时的 1-based 行号。

### 3. 帧安全写入器 (`NdjsonWriter`) 与批处理接口

- **`NdjsonWriter.init(allocator: std.mem.Allocator): NdjsonWriter`** / **`deinit(*_)`**：初始化与释放写入缓冲区。
- **`writeValue(*_, val: std.json.Value): void ! NdjsonError`**：将 `std.json.Value` 序列化为单行紧凑 JSON 并追加 `\n`。
- **`writeRawJson(*_, line: []const u8): void ! NdjsonError`**：严格校验 `line` 为非空合法 JSON 且绝不包含裸 `\n` 或 `\r`，随后追加该行与 `\n`。
- **`bytes(*_): []const u8`**：获取当前已写入的 NDJSON 字节只读视图。
- **`parseAll(allocator: std.mem.Allocator, input: []const u8): ([]NdjsonRecord) ! NdjsonError`**：
  一次性解析完整 NDJSON 文本，返回由调用方负责逐条 `.deinit()` 并 `allocator.free(...)` 的记录切片。
- **`stringifyAll(allocator: std.mem.Allocator, values: []const std.json.Value): ([]u8) ! NdjsonError`**：
  将 `std.json.Value` 切片批量序列化为单行分隔的 NDJSON 字节串。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { NdjsonStreamParser, NdjsonWriter, parseAll } from "cap:ndjson";
import cap from "cap";

pub fn demoNdjson(): ! {
    const alloc = std.heap.page_allocator;

    let parser = NdjsonStreamParser.init(alloc, 4096)?;
    defer parser.deinit();

    * = parser.feed("{\"id\":1,\"msg\":\"hi\"}\r\n   \r\n{\"id\":2")?;
    * = parser.feed(",\"ok\":true}")?;
    * = parser.finish(true)?;

    while (rec := parser.nextRecord()) {
        * = rec.line_number;
    }

    let writer = cap.ndjson.NdjsonWriter.init(alloc);
    defer writer.deinit();
    writer.writeRawJson("{\"event\":\"ping\"}")?;

    const all = parseAll(alloc, writer.bytes())?;
    defer {
        let i: usize = 0;
        while (i < all.len) : (i += 1) {
            all[i].deinit();
        }
        alloc.free(all);
    }
}
```
