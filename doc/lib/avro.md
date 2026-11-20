# CP 标准库：Apache Avro 二进制编解码与 OCF 容器引擎 (`cap:avro`)

## 一、 架构总览与设计准则

`cap:avro` 是 CP 语言官方内置的纯 CP **Apache Avro Schema 驱动二进制编解码与 Object Container File (OCF `"Obj\x01"`) 容器引擎**。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:avro"`，也支持 `import cap from "cap"; cap.avro.*` 动态懒挂载调用。

### 核心特性

- **完整 Avro Schema 解析 (`AvroSchema.parseSchema`)**：基于 `std.json` 解析并验证 Avro 全部 14 种类型：`null`、`boolean`、`int`、`long`、`float`、`double`、`bytes`、`string`、`array`、`map`、`union`、`enum`、`fixed`、`record`（含 `namespace` 命名空间与命名类型引用解析），内部使用 `ArenaAllocator` 托管 Schema AST，调用 `schema.deinit()` 即可一键释放。
- **标准 Avro 二进制 Wire Format**：
  - `int` / `long` 采用 ZigZag + 变长整数（Varint）编码，严格校验 32-bit/64-bit 溢出（`VarintOverflow`）；
  - `float` / `double` 采用小端序（Little-Endian）IEEE 754 编码；
  - `bytes` / `string` 采用 ZigZag `long` 长度前缀；
  - `array` / `map` 支持块计数编码（含负块计数+块字节长度跳过）与尾随 `0` 块结束符；
  - `union` 采用 `long` 分支索引前缀，严格校验越界索引（`InvalidUnionIndex`）。
- **OCF 容器格式 (`"Obj\x01"`)**：支持 `writeContainer` 与 `readContainer`，完整编解码 `"Obj\x01"` 魔数、`"avro.schema"` / `"avro.codec"` 元数据、多记录数据块以及 16 字节 `sync_marker` 同步标记逐块一致性校验。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/avro/avro.cp` | `// Source: cp/lib/avro/avro.cp (...)` | `AvroError`、`AvroSchema`、`AvroValue`、`AvroContainer`、`encodeDatum`、`decodeDatum`、`writeContainer`、`readContainer` |
| `lib/avro/index.cp` | `// Source: cp/lib/avro/index.cp (...)` | `cap:avro` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 (`AvroError`)

- `InvalidSchema`：Schema JSON 非法、缺少必需字段、嵌套 Union 或命名类型未定义。
- `UnexpectedEof`：输入二进制缓冲区截断。
- `VarintOverflow`：Varint 超过 5 字节（`int`）或 10 字节（`long`）上限。
- `SchemaMismatch`：值与 Schema 类型不匹配、枚举索引越界、`fixed` 长度不符或非法布尔值。
- `InvalidUnionIndex`：`union` 分支索引超出分支数组范围。
- `InvalidSyncMarker`：OCF 数据块尾部 16 字节同步标记与文件头不一致。
- `InvalidContainerHeader`：OCF 魔数非 `"Obj\x01"`、缺少 `"avro.schema"`、不支持的压缩编码或非法块头。
- `MaxDepthExceeded`：嵌套深度超过 `DEFAULT_MAX_DEPTH`（64）。
- `OutOfMemory`：内存分配失败。

### 2. Schema 与动态值树 (`AvroSchema` / `AvroValue`)

- **`AvroSchema.parseSchema(allocator, schema_json): AvroError!AvroSchema`** / **`schema.deinit()`**
- **`AvroValue`**：
  - 构造器：`makeNull()`、`makeBoolean(v)`、`makeInt(v)`、`makeLong(v)`、`makeFloat(v)`、`makeDouble(v)`、`makeBytes(v)`、`makeString(v)`、`makeArray(items)`、`makeMap(entries)`、`makeUnion(index, value_ptr)`、`makeEnum(index, symbol)`、`makeFixed(data)`、`makeRecord(fields)`
  - 查询与比较：`isNull()`、`asBoolean()`、`asInt()`、`asLong()`、`asFloat()`、`asDouble()`、`asBytes()`、`asString()`、`asArray()`、`asMap()`、`asUnion()`、`asEnum()`、`asFixed()`、`asRecord()`、`getField(name)`、`eql(other)`
  - 内存释放：`deinit(allocator)` 递归释放解码产生的字符串、字节切片、容器切片及联合体指针。

### 3. Datum 与 OCF 容器编解码

- **`encodeDatum(allocator, schema, value): AvroError![]u8`**
- **`decodeDatum(allocator, schema, bytes): AvroError!AvroValue`**
- **`writeContainer(allocator, schema_json, sync_marker: [16]u8, records: []const AvroValue): AvroError![]u8`**
- **`readContainer(allocator, ocf_bytes): AvroError!AvroContainer`**（返回 `AvroContainer{ schema_json, sync_marker, codec, records }`，调用 `container.deinit(allocator)` 释放全部资源）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { AvroSchema, AvroValue, AvroRecordField, encodeDatum, decodeDatum, writeContainer, readContainer } from "cap:avro";
import cap from "cap";

pub fn demoAvro()! {
    const alloc = std.heap.page_allocator;
    const schema_json = "{\"type\":\"record\",\"name\":\"Event\",\"fields\":[{\"name\":\"id\",\"type\":\"long\"},{\"name\":\"tag\",\"type\":[\"null\",\"string\"]}]}";

    let schema = AvroSchema.parseSchema(alloc, schema_json)?;
    defer schema.deinit();

    let tag_str = AvroValue.makeString("deploy");
    let fields: [2]AvroRecordField = [
        AvroRecordField.init("id", AvroValue.makeLong(42)),
        AvroRecordField.init("tag", AvroValue.makeUnion(1, &tag_str)),
    ];
    const rec = AvroValue.makeRecord(&fields);

    const encoded = encodeDatum(alloc, schema, rec)?;
    defer alloc.free(encoded);

    let decoded = decodeDatum(alloc, schema, encoded)?;
    defer decoded.deinit(alloc);

    const sync_marker: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
    const records: [1]AvroValue = [rec];
    const ocf = cap.avro.writeContainer(alloc, schema_json, sync_marker, &records)?;
    defer alloc.free(ocf);

    let container = cap.avro.readContainer(alloc, ocf)?;
    defer container.deinit(alloc);
}
```
