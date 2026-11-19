# CP 标准库：SourceMap v3 源码映射与位置反查子系统使用手册 (`cap:sourcemap`)

## 一、 架构总览与设计哲学

`cap:sourcemap` 是 CP 语言官方内置的工业级 **SourceMap v3 (ECMA-426)** 源码位置映射、Base64 VLQ 变长编解码、行偏移 $O(\log N)$ 二分定位以及紧凑二进制检查点（`InternalSourceMap`）标准库。完整吸收自 Bun（`/data/dev/bun/src/sourcemap/`），专为 CP 编译器转译堆栈还原、打包器（Bundler）分块拼接与调试器（LSP / DAP）单步断点定位设计。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                            cap:sourcemap 源码映射架构体系                               │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│   高级生成与消费门面       │    紧凑内存与二分索引引擎    │     底层变长编码原语          │
│  • SourceMapGenerator     │  • LineOffsetTable<MAX>     │  • Base64 VLQ (7B 栈内联)    │
│  • SourceMapConsumer      │  • DynamicLineOffsetTable   │  • ZigZag 有符号映射         │
│  • Chunk (分块拼接与JSON) │  • ParsedSourceMap<CAP>     │  • Varint 7-bit 字节流       │
│  • ParsedSourceMapRecord  │  • InternalSourceMap (64步) │                              │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 Base64 VLQ 编码原理与 SourceMap v3 规范

### 1. Base64 VLQ (Variable-Length Quantity) 编码机制

SourceMap v3 的 `"mappings"` 字符串由分号 `;`（代表生成代码换行，且生成列归零）与逗号 `,`（代表同一生成行内的下一个映射段）分隔。每个映射段包含 **1、4 或 5 个相对增量字段**，每个有符号整数 `value: i32` 按如下规则编码为 Base64 VLQ：

1. **符号位最低位折叠（LSB Sign Bit）**：
   - 将有符号整数映射为无符号 `u32`：若 `value >= 0`，则 `vlq = value << 1`；若 `value < 0`，则 `vlq = ((-value) << 1) | 1`。
2. **5-bit 分组与第 6 位延续标志（Continuation Bit）**：
   - 每次提取 `vlq` 的低 5 位（`digit = vlq & 31`），并将 `vlq >>= 5`；
   - 若后续仍有非零高位（`vlq != 0`），则置第 6 位延续位 `digit |= 32`（`0x20`）；
   - 将 `0..63` 的 `digit` 通过标准 Base64 字符表（`A-Z a-z 0-9 + /`）映射为单个 ASCII 字节。
3. **栈内联零堆分配（`VLQ` 结构体）**：
   - 32 位有符号整数最多仅需 7 个 Base64 字节，`VLQ` 结构体直接在栈帧内使用 `bytes: [7]u8` 存储，零堆内存分配。

| 原始有符号值 (`i32`) | 折叠后无符号值 (`u32`) | 6-bit 组序列 | Base64 VLQ 字符串 |
| :---: | :---: | :---: | :---: |
| `0` | `0` (`0b00000`) | `[0]` | `"A"` |
| `1` | `2` (`0b00010`) | `[2]` | `"C"` |
| `-1` | `3` (`0b00011`) | `[3]` | `"D"` |
| `16` | `32` (`0b100000`) | `[32 \| 0, 1]` | `"gB"` |

---

## 三、 核心组件详解与完整代码示例

### 1. Base64 VLQ 与 ZigZag / Varint 编解码 (`vlq.cp` & `internal_source_map.cp`)

```typescript
import {
    VLQ,
    zigzagEncode,
    zigzagDecode,
    writeVarint,
    readVarint,
} from "cap:sourcemap";

pub fn demoVlqAndVarint() {
    // 1. 单值 Base64 VLQ 编解码
    let v = VLQ.encode(-12345);
    const decoded = VLQ.decode(v.slice(), 0);
    // decoded.valid == true, decoded.value == -12345

    // 2. 批量数组编码为连续 Base64 VLQ 字符串
    let fields: [4]i32 = [0, 1, -1, 16];
    let out_buf: [32]u8 = undefined;
    const written = VLQ.encodeValues(&fields, &out_buf); // "ACDgB"

    // 3. ZigZag + Varint 二进制紧凑编解码
    let varint_buf: [16]u8 = undefined;
    const n = writeVarint(&varint_buf, 0, -9999);
    let pos: usize = 0;
    const val = readVarint(&varint_buf, &pos); // -9999
}
```

---

### 2. 行偏移表与 $O(\log N)$ 二分行列定位 (`LineOffsetTable` / `DynamicLineOffsetTable`)

在编译器与打包器中，词法分析器通常只记录 32 位字节偏移 `byte_offset: u32`。`LineOffsetTable` 预先扫描源码中的 `'\n'` 构建行首偏移数组，随后通过二分查找在 $O(\log N)$ 时间内将任意字节偏移与 `(line, column)` 双向互转：

```typescript
import { LineOffsetTable, DynamicLineOffsetTable } from "cap:sourcemap";
const std = @import("std");

pub fn demoLineOffset()! {
    const source = "fn add(a: i32, b: i32): i32 {\n    return a + b;\n}\n";

    // 1. 零堆分配编译期定容行偏移表（适用于栈内快速定位）
    const table = LineOffsetTable<64>.build(source);
    const loc = table.resolveLocation(34); // line = 1, column = 4 ("return a + b;")
    const offset = table.resolveByteOffset(loc.line, loc.column); // 34

    // 2. 动态堆分配行偏移表（适用于任意超大源码文件）
    const allocator = std.heap.page_allocator;
    let dyn_table = try DynamicLineOffsetTable.build(allocator, source);
    defer dyn_table.deinit(allocator);
    const dyn_loc = dyn_table.resolveLocation(34);
}
```

---

### 3. 零堆分配定容映射表与二分位置反查 (`ParsedSourceMap<CAP>`)

```typescript
import { ParsedSourceMap } from "cap:sourcemap";

pub fn demoStaticParsedSourceMap() {
    // 解析两行共 3 个映射段的 SourceMap v3 mappings 字符串
    const sm = ParsedSourceMap<16>.parseMappings("AAAA,IAAIA;ECAA");

    // 二分查找生成代码第 1 行、第 5 列对应的最近左侧原始位置映射
    if (m := sm.findMapping(1, 5)) {
        @print("原始文件索引: {d}, 原始行: {d}, 原始列: {d}\n",
            m.source_index, m.original_line, m.original_column);
    }
}
```

---

### 4. SourceMap v3 生成器与消费者闭环 (`SourceMapGenerator` & `SourceMapConsumer`)

```typescript
import { SourceMapGenerator, SourceMapConsumer } from "cap:sourcemap";
const std = @import("std");

pub fn demoGeneratorAndConsumer()! {
    const allocator = std.heap.page_allocator;

    // 1. 使用 SourceMapGenerator 构建完整 SourceMap v3 JSON
    let gen = SourceMapGenerator.init(allocator, "bundle.js");
    defer gen.deinit();

    const src_idx = try gen.addSourceWithContent(
        "src/math.cp",
        "pub fn multiply(x: i32, y: i32): i32 { return x * y; }"
    );
    const name_idx = try gen.addName("multiply");

    try gen.addMapping(0, 0, 0, 0, src_idx, -1);
    try gen.addMapping(0, 9, 0, 7, src_idx, name_idx);

    const json_bytes = try gen.generateJSON(allocator, true);
    defer allocator.free(json_bytes);

    // 2. 使用 SourceMapConsumer 解析 JSON 并反查报错位置
    let consumer = try SourceMapConsumer.init(allocator, json_bytes);
    defer consumer.deinit();

    if (orig := consumer.originalPositionFor(0, 12)) {
        @print("定位成功 -> 源文件: {s}, 行: {d}, 列: {d}, 符号名: {s}\n",
            orig.source, orig.line, orig.column, orig.name);
    }
}
```

---

### 5. 紧凑二进制同步检查点映射表 (`InternalSourceMap` & `InternalSourceMapBuilder`)

源自 `/data/dev/bun/src/sourcemap/InternalSourceMap.cp`。当运行时长驻数千个模块的 SourceMap 时，全量展开 `SourceMapping` 数组会占用大量内存。`InternalSourceMap` 每隔 `SYNC_INTERVAL = 64` 条映射记录一个全量同步检查点（`SyncEntry`），中间 63 条映射以 ZigZag + Varint 相对增量字节流压缩存储。查询时先在 `sync_entries` 上执行 $O(\log(N/64))$ 二分查找，再重放至多 63 条 Varint 增量，兼顾极致内存压缩率与微秒级反查延迟。

---

## 四、 与 Bun / Mozilla `source-map` 生态对照表

| 功能模块 | Mozilla `source-map` (JS) | Bun (`src/sourcemap/*.cp`) | CP 标准库 (`cap:sourcemap`) |
| :--- | :--- | :--- | :--- |
| **Base64 VLQ 编解码** | `base64-vlq.encode / decode` | `VLQ.encode` / `VLQ.decode` | `VLQ.encode` / `VLQ.decode` / `encodeValues` |
| **字节偏移转行列号** | 需手动 split lines | `LineOffsetTable` | `LineOffsetTable<MAX>` / `DynamicLineOffsetTable` |
| **栈内零堆映射反查** | 不支持（全堆对象） | `ParsedSourceMap` | `ParsedSourceMap<CAP>.parseMappings` / `findMapping` |
| **运行时二进制压缩表**| 不支持 | `InternalSourceMap` | `InternalSourceMapBuilder` / `InternalSourceMap.find` |
| **分块构建与 JSON 序列化** | `SourceNode` | `Chunk` | `Chunk` / `SourceMapGenerator` |
| **JSON 解析与堆栈反查** | `new SourceMapConsumer(json)`| `ParsedSourceMap` | `SourceMapConsumer` / `ParsedSourceMapRecord` |
