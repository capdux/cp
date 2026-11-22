# CP 标准库：SourceMap v3 源码映射与位置反查子系统使用手册 (`cap:sourcemap`)

> 参考来源：`/data/dev/bun/src/sourcemap/sourcemap.cp`, `/data/dev/bun/src/sourcemap/` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:sourcemap` 是 CP 语言官方内置的工业级 **SourceMap v3 (ECMA-426)** 源码位置映射、Base64 VLQ 变长编解码、行偏移 $O(\log N)$ 二分定位、`MappingList` 排序与反查、`SourceMapChunk` 多段拼接以及紧凑二进制检查点（`InternalSourceMap`）标准库。完整吸收自 Bun（`/data/dev/bun/src/sourcemap/`）。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                            cap:sourcemap 源码映射架构体系                               │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│   高级生成、拼接与消费     │    紧凑内存与二分索引引擎    │     底层变长编码原语          │
│  • SourceMapGenerator     │  • MappingList (sort/find)  │  • Base64 VLQ (i32全范围)    │
│  • SourceMapConsumer      │  • LineOffsetTable<MAX>     │  • VLQ64 (i64 64位有符号)    │
│  • SourceMapChunk / Chunk │  • DynamicLineOffsetTable   │  • ZigZag 有符号映射         │
│  • ParsedSourceMapRecord  │  • InternalSourceMap (64步) │  • Varint 7-bit 字节流       │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 核心能力与边界规格

### 1. 完整 Base64 VLQ 有符号编解码 (`vlq.cp`)

- **`VLQ.encode(value: i32)` / `VLQ.decode(encoded, start)`**：内部采用 64 位无符号算术，完整覆盖 `i32` 全边界（包括 `-2147483648` 最小负整数与 `2147483647` 最大正整数），彻底避免取反溢出与移位截断。
- **`VLQ.encodeValues` / `VLQ.decodeValues`**：批量编码与解码连续 VLQ 序列。
- **`VLQ.encode64(value: i64)` / `VLQ.decode64(encoded, start)`**：支持完整 64 位有符号整数（最多 13 个 Base64 字节，`VLQ64`），适用于超大偏移与大整数边界场景。

### 2. `MappingList` 排序与二分反查 (`mapping.cp`)

- **`MappingList`**：动态可增长映射列表，自动追踪排序状态：
  - `append(allocator, mapping)` / `appendParts(...)`：追加映射项；
  - `sort()`：按 `(generated_line, generated_column)` 升序执行稳定原地排序；
  - `find(generated_line, generated_col)` / `findIndex(...)` / `findOnLine(...)`：在 $O(\log N)$ 时间内二分定位最近左侧映射项（若尚未排序则自动先排序）；
  - `parseMappings(allocator, str)` / `toMappingsString(allocator)`：标准 SourceMap v3 `"mappings"` 双向编解码。

### 3. `LineOffsetTable` 字节偏移与行列双向转换 (`lineOffset.cp`)

- 同时提供栈内零堆分配 **`LineOffsetTable<MAX_LINES>`** 与动态 **`DynamicLineOffsetTable`**：
  - `resolveLocation(byte_offset)` / `resolveByteOffset(line, column)` / `resolveByteOffsetClamped(line, column)`；
  - `lineRange(line)` / `lineLength(line)` / `lineText(contents, line)` / `lineCount()`；
  - `utf8ToUtf16Column` / `utf16ToUtf8Column` / `resolveUtf16Location`：精确处理多字节 UTF-8（CJK 与 4 字节 Emoji 代理对）与 SourceMap v3 UTF-16 码元列号换算。

### 4. SourceMap v3 JSON 解析/序列化与 `SourceMapChunk` 多段拼接 (`parsedSourceMap.cp`, `chunk.cp`)

- **`ParsedSourceMapRecord.parse` / `toJSON`**：完整顶层 JSON 状态机扫描，正确处理字符串转义（`\"`, `\\`, `\n`, `\r`, `\t`, `\uXXXX`）、`sourcesContent` 中的 `null` 占位符、`sourceRoot` 与 `version: 3`。
- **`SourceMapChunk` (`Chunk`)**：
  - `appendChunk(allocator, &other, line_offset, col_offset)`：将其他 Chunk 合并到当前 Chunk，自动对 `sources`、`sourcesContent`、`names` 去重并建立重映射表，同步偏移生成行列；
  - `Chunk.merge(allocator, chunks, line_offsets)`：批量合并多个分段 Chunk 并排序；
  - `Chunk.fromParsed(allocator, &record)`：将已解析 JSON 还原为可拼接 Chunk。

---

## 三、 核心组件完整代码示例

### 1. `VLQ` 极端边界、`MappingList` 排序与二分反查

```typescript
const std = @import("std");
import { VLQ, MappingList, SourceMapping, LineOffsetTable } from "cap:sourcemap";

pub fn demoMappingListAndVlq()! {
    const alloc = std.heap.page_allocator;

    // 1. 极端边界 i32 & i64 VLQ 编解码
    let v_min = VLQ.encode(-2147483648);
    const d_min = VLQ.decode(v_min.slice(), 0);
    // d_min.valid == true && d_min.value == -2147483648

    let v64 = VLQ.encode64(-9007199254740991);
    const d64 = VLQ.decode64(v64.slice(), 0);
    // d64.valid == true && d64.value == -9007199254740991

    // 2. 乱序追加后自动排序并二分查找
    let list = MappingList.init();
    defer list.deinit(alloc);
    try list.appendParts(alloc, 2, 10, 20, 4, 0, 1);
    try list.appendParts(alloc, 0, 0, 0, 0, 0, -1);
    try list.appendParts(alloc, 1, 4, 5, 2, 0, 0);
    list.sort();

    if (m := list.find(1, 8)) {
        // 命中 (1, 4) -> original (5, 2)
        * = m;
    }
}
```

### 2. `SourceMapChunk` 多段映射拼接合并

```typescript
const std = @import("std");
import { SourceMapChunk, Chunk, SourceMapping, SourceMapConsumer } from "cap:sourcemap";

pub fn demoChunkMerge()! {
    const alloc = std.heap.page_allocator;

    let c1 = SourceMapChunk.init();
    defer c1.deinit(alloc);
    const s1 = try c1.addSource(alloc, "a.cp", "fn a() {}");
    try c1.addMapping(alloc, SourceMapping{ generated_line: 0, generated_column: 0, source_index: s1, original_line: 0, original_column: 0 });

    let c2 = SourceMapChunk.init();
    defer c2.deinit(alloc);
    const s2 = try c2.addSource(alloc, "b.cp", "fn b() {}");
    const n2 = try c2.addName(alloc, "b");
    try c2.addMapping(alloc, SourceMapping{ generated_line: 0, generated_column: 0, source_index: s2, original_line: 3, original_column: 2, name_index: n2 });

    // c2 拼接至第 10 行起
    let merged = SourceMapChunk.init();
    defer merged.deinit(alloc);
    try merged.appendChunk(alloc, &c1, 0, 0);
    try merged.appendChunk(alloc, &c2, 10, 0);

    const json = try merged.printSourceMapJSON(alloc, "bundle.js", true);
    defer alloc.free(json);
    let consumer = try SourceMapConsumer.init(alloc, json);
    defer consumer.deinit();
    // consumer.originalPositionFor(10, 2) => source="b.cp", line=3, column=2, name="b"
}
```

---

## 四、 与 Bun / Mozilla `source-map` 生态对照表

| 功能模块 | Mozilla `source-map` (JS) | Bun (`src/sourcemap/*.cp`) | CP 标准库 (`cap:sourcemap`) |
| :--- | :--- | :--- | :--- |
| **Base64 VLQ 编解码** | `base64-vlq.encode / decode` | `VLQ.encode` / `VLQ.decode` | `VLQ.encode` / `decode` / `encode64` / `decode64` |
| **字节偏移与 UTF-16 行列** | 需手动 split lines | `LineOffsetTable` | `LineOffsetTable<MAX>` / `DynamicLineOffsetTable` / `utf8ToUtf16Column` |
| **映射排序与二分查找** | `SourceMapConsumer` | `Mapping.List` (`sort`, `find`) | `MappingList` (`sort`, `find`, `findOnLine`, `findIndex`) |
| **运行时二进制压缩表** | 不支持 | `InternalSourceMap` | `InternalSourceMapBuilder` / `InternalSourceMap.find` |
| **多段 Chunk 拼接合并** | `SourceNode` | `Chunk` / `appendSourceMapChunk` | `SourceMapChunk` (`appendChunk`, `merge`, `fromParsed`) |
| **JSON 解析与序列化** | `new SourceMapConsumer(json)` | `ParsedSourceMap` | `SourceMapConsumer` / `ParsedSourceMapRecord.parse` / `toJSON` |

---

## 五、 上游 Bun 行为契约与边界注意事项

1. **0-based 坐标与 UTF-16 列偏移约定（ECMA-426）**：
   - SourceMap v3 协议的 `line` 与 `column` 均从 `0` 开始计，且列号按 **UTF-16 码元**计量（非 UTF-8 字节偏移）；对包含中文或 Emoji（代理对占 2 个 UTF-16 码元）的源码行，务必通过 `LineOffsetTable` / `utf8ToUtf16Column` 转换后再编码 VLQ，避免堆栈反查列偏移错位。
2. **VLQ 极端整数与畸形输入防护**：
   - `VLQ.encode` / `VLQ.decode` 在 64 位无符号算术下处理 `i32` 边界（含 `INT32_MIN = -2147483648`），拒绝超长连续续位字节与非法 Base64 字符，防止畸形 `.map` 文件触发位移溢出或死循环。
3. **运行时紧凑检查点（`InternalSourceMap`）性能特征**：
   - 对标 Bun 运行时异常堆栈反解机制：`InternalSourceMap` 每 64 条映射记录一个稀疏检查点，在极低内存占用下先二分定位检查点再线性推进至多 64 步，适合嵌入 AOT 二进制或长驻服务中做毫秒级错误堆栈还原。
4. **多 Chunk 拼接与偏移重定位**：
   - 使用 `SourceMapChunk.appendChunk(alloc, &chunk, line_offset, col_offset)` 合并多份打包产物时，自动按目标行/列偏移重算生成坐标并重映射 `sources` / `names` 索引，支持直接通过 `printSourceMapJSON` 输出标准 v3 JSON。
5. **DevServer SourceMap 强弱引用缓存与延迟拉取（`lib/sourcemap/sourceMapStore.cp`，吸收自 `/data/dev/bun/src/bake/DevServer/SourceMapStore.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
   - **导出符号**：`SOURCEMAP_WEAK_REF_EXPIRY_SECS` (`10`)、`SOURCEMAP_WEAK_REF_MAX` (`16`)、`SOURCEMAP_STORE_MAX_ENTRIES` (`128`)、`SOURCEMAP_URL_PREFIX` (`"/_bun/client/"`)、`SOURCEMAP_URL_SUFFIX` (`".js.map"`)、`SourceMapStoreError`、`SourceMapChunkKind`（`InitialResponse`/`HmrChunk`）、`SourceMapStoreId`（64-bit 打包键值：`forInitialResponse`/`forHmrChunk`/`kind`/`generationId`/`contentHash`/`formatUrl`）、`SourceMapStoreEntry`、`SourceMapStore`（`init`/`totalCount`/`strongCount`/`weakCount`/`totalMemoryBytes`/`putOrRef`/`get`/`unref`/`pruneExpired`/`removeImmediate`）；
   - **Bun Bundler `sourcemap` 策略与 HMR 弱引用宽限期最佳实践**：
     - 生产打包按需选择 `none` / `linked`（推荐，外置 `.map` 文件并附带 `//# sourceMappingURL=`）/ `external`（不注入注释）/ `inline`（DataURL 内联）；
     - 开发期热重载失效时，浏览器 DevTools 往往在数秒后才异步请求 `/_bun/client/{id}.js.map`：`SourceMapStore.unref()` 在强引用归零时不立即销毁条目，而是转入上限 16 条、宽限期 10 秒的弱引用缓存池；再次 `putOrRef()` 可原地复活，超时由 `pruneExpired(now_sec)` 回收，避免 HMR 频繁刷新引发内存泄漏。

```cp
import { SourceMapStore, SourceMapStoreId } from "cap:sourcemap";

let store = SourceMapStore.init();
const id = SourceMapStoreId.forInitialResponse(7);
* = store.putOrRef(id, 12, 8192)?;
* = store.unref(id, 100)?;                // 转入弱引用宽限期（100s..110s）
const still_ok = store.get(id, 105);      // 仍在 10s 宽限期内，DevTools 命中！
const pruned = store.pruneExpired(110);   // 110s 过期自动物理回收 (pruned == 1)
* = still_ok; * = pruned;
```
