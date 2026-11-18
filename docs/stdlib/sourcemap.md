# `cap:sourcemap` — 工业级 SourceMap v3 标准库

`cap:sourcemap` 是 CP 语言的高性能 SourceMap v3 基础设施标准库，完全对标并吸收自 Bun 原生源码地图运行时与打包器资产。它提供了从底层的 Base64 VLQ 编解码、紧凑二进制内存表示（InternalSourceMap）、行偏移快速检索表（LineOffsetTable），到高层的产物分块管理（Chunk）、SourceMap v3 JSON 解析（ParsedSourceMap）与生成/消费门面（SourceMapGenerator / SourceMapConsumer）的全套工业级支持。

## 核心组件与特性

1. **Base64 VLQ 编解码器 (`VLQ`)**：
   - 遵循 RFC-4648 与 SourceMap v3 规范；
   - 7 字节内联定长栈缓冲，单数值编解码 100% 零堆内存分配；
   - 支持多数值批量编解码（`encodeValues` / `decode`）。

2. **行偏移检索表 (`LineOffsetTable` / `DynamicLineOffsetTable`)**：
   - **定容版本 `LineOffsetTable<MAX_LINES>`**：零堆分配，适合轻量级/嵌入式代码或定容栈帧环境；
   - **动态版本 `DynamicLineOffsetTable`**：支持任意长度超大源码文本的动态行偏移索引；
   - 提供基于 $O(\log N)$ 二分查找的高速行号、列号与字节偏移互查（`findLine`、`resolveLocation`、`resolveByteOffset`）。

3. **映射表数据模型 (`SourceMapping` / `ParsedSourceMap<CAP>`)**：
   - 精确记录 `(generated_line, generated_column, source_index, original_line, original_column, name_index)`；
   - 提供快速有效性判定（`hasOriginal`、`hasName`）；
   - 内置栈内定容映射表 `ParsedSourceMap<CAP>`，支持直接解析 `;`/`,` 分隔的 v3 `"mappings"` 字符串。

4. **紧凑二进制存储流 (`InternalSourceMap`)**：
   - 吸收自 Bun 的极致性能设计；
   - 基于 Zigzag 与 Varint 变长整型压缩存储增量偏移；
   - 采用每 64 项设立全局同步检查点（`SyncEntry`）的机制，将检索复杂度严格限制在 $O(\log S + 64)$ 内。

5. **产物分块管理器 (`Chunk`)**：
   - 专为打包器与转译器设计的多源文件与符号管理；
   - 自动维护 `sources`、`sourcesContent` 与 `names` 去重索引表；
   - 输出符合官方标准的 v3 JSON（`printSourceMapJSON`）与 mappings 串（`generateMappingsString`）。

6. **高阶生成与消费门面 (`SourceMapGenerator` / `SourceMapConsumer`)**：
   - 统一面向开发者的现代 API；
   - 支持链式添加源码、映射并直接产出格式化 JSON；
   - 支持反序列化消费 JSON 并反向定位源码文件名与行列（`originalPositionFor`）。

---

## API 使用示例

### 1. Base64 VLQ 编解码

```typescript
import { VLQ } from "cap:sourcemap";

// 编码单个有符号整型
let v0 = VLQ.encode(0);    // => "A"
let v1 = VLQ.encode(1);    // => "C"
let vm1 = VLQ.encode(-1);  // => "D"
let v16 = VLQ.encode(16);  // => "gB"

// 解码
let res = VLQ.decode("gB", 0);
if (res.valid) {
    // res.value => 16
    // res.next_offset => 2
}
```

### 2. 行偏移与源码行列定位

```typescript
import { DynamicLineOffsetTable } from "cap:sourcemap";
import std from "std";

const allocator = std.heap.page_allocator;
const source_code = "const a = 1;\nconst b = 2;\nconsole.log(a + b);\n";

let table = try DynamicLineOffsetTable.build(allocator, source_code);
defer table.deinit(allocator);

// 查询字节偏移 34 对应的源码行列号
const loc = table.resolveLocation(34);
// loc.line => 2 (0-based)
// loc.column => 8 (0-based)
```

### 3. 生成与消费 SourceMap v3

```typescript
import { SourceMapGenerator, SourceMapConsumer } from "cap:sourcemap";
import std from "std";

const allocator = std.heap.page_allocator;

// --- 1. 生成地图 ---
let gen = SourceMapGenerator.init(allocator, "bundle.js");
defer gen.deinit();

const src_idx = try gen.addSourceWithContent("math.cp", "export fn add(a: i32, b: i32) => a + b;");
const name_idx = try gen.addName("add");

// 添加映射：generated (0, 7) -> original (0, 10), source math.cp, symbol add
try gen.addMapping(0, 7, 0, 10, src_idx, name_idx);

const json_output = try gen.generateJSON(allocator, true);
defer allocator.free(json_output);

// --- 2. 消费与反查 ---
let consumer = try SourceMapConsumer.init(allocator, json_output);
defer consumer.deinit();

if (pos := consumer.originalPositionFor(0, 8)) {
    // pos.source => "math.cp"
    // pos.line => 0
    // pos.column => 10
    // pos.name => "add"
}
```

---

## 溯源关系

本模块全部资产对齐 Bun 生产级源码：
- `lib/sourcemap/vlq.cp`: `// Source: bun/src/sourcemap/VLQ.cp`
- `lib/sourcemap/line_offset.cp`: `// Source: bun/src/sourcemap/LineOffsetTable.cp`
- `lib/sourcemap/mapping.cp`: `// Source: bun/src/sourcemap/Mapping.cp`
- `lib/sourcemap/internal_source_map.cp`: `// Source: bun/src/sourcemap/InternalSourceMap.cp`
- `lib/sourcemap/chunk.cp`: `// Source: bun/src/sourcemap/Chunk.cp`
- `lib/sourcemap/parsed_source_map.cp`: `// Source: bun/src/sourcemap/ParsedSourceMap.cp`
- `lib/sourcemap/sourcemap.cp`: `// Source: bun/src/sourcemap/sourcemap.cp`
- `lib/sourcemap/index.cp`: `// Source: bun/src/sourcemap/`
