# CP 标准库：MongoDB BSON 1.1 二进制文档与 12 字节 ObjectId 编解码器 (`cap:bson`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 一、 架构总览与设计准则

`cap:bson` 是 CP 语言官方内置的 **MongoDB BSON 1.1（Binary JSON）** 高性能编解码标准库，严格遵循 BSON 1.1 小端序二进制规范与 12 字节 `ObjectId` 布局（`[timestamp:4B BE][random:5B][counter:3B BE]`）。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:binfmt"` 静态导入，也支持 `import cap from "cap"; cap.bson.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│               CP MongoDB BSON 1.1 编解码引擎 (cap:bson)                  │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ 强类型 BSON AST       │ 12 字节 ObjectId        │ 严苛边界与深度防护     │
│ BsonValue / BsonField │ fromBytes / fromHex     │ InvalidDocumentSize   │
│ BsonBinary/Timestamp  │ toHex / generate        │ MissingTerminator     │
│ deinit / get / eql    │ timestampSec / eql      │ InvalidCString/Utf8   │
│ encodeDocument        │ Big-Endian 布局解析     │ 64 层 MaxDepthExceeded│
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bson/bson.cp` | `// Source: cp/lib/bson/bson.cp (MongoDB BSON 1.1 Binary Document & 12-Byte ObjectId Codec)` | `BsonError`、`ObjectId`、`BsonBinary`、`BsonTimestamp`、`BsonField`、`BsonValue`（含 `deinit`、`get`、`eql`）、`encodeDocument`、`decodeDocument`、`decodeDocumentWithMaxDepth` |
| `lib/bson/index.cp` | `// Source: cp/lib/bson/index.cp (Unified Exports for cap:bson)` | `cap:bson` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `BsonError`

```typescript
pub err BsonError {
    UnexpectedEof,       // 缓冲区提前结束
    InvalidDocumentSize, // 文档或字符串长度非法（如 < 5 或超出载荷边界）
    MissingTerminator,   // 文档或 UTF-8 字符串缺少 0x00 终止符
    InvalidElementType,  // 未知的 BSON 元素类型字节
    InvalidUtf8,         // 键名或字符串包含非法 UTF-8 序列
    InvalidBoolean,      // Boolean 字节非 0x00 / 0x01
    InvalidCString,      // CString 键名包含内嵌 \x00 字节
    InvalidObjectId,     // ObjectId Hex 长度非 24 或含非十六进制字符
    MaxDepthExceeded,    // 嵌套文档/数组超过最大递归深度（默认 64 层）
    TrailingBytes,       // 根文档结束后存在多余尾随字节
    OutOfMemory,         // 内存分配失败
}
```

### 2. 12 字节 `ObjectId`

内存布局：`[timestamp: 4B BE][machine_rand: 5B][counter: 3B BE]`。

| 方法签名 | 说明 |
| :--- | :--- |
| `ObjectId.fromBytes(bytes: [12]u8): ObjectId` | 从 12 字节数组构造 `ObjectId` |
| `ObjectId.fromHex(hex: []const u8): ObjectId!` | 解析 24 位十六进制字符串，非法则抛出 `BsonError.InvalidObjectId` |
| `oid.toHex(alloc: std.mem.Allocator): ([]u8)!` | 分配并生成 24 位小写十六进制字符串（调用方负责 `alloc.free`） |
| `ObjectId.generate(timestamp_sec: u32, machine_rand: [5]u8, counter: u32): ObjectId` | 按标准大端序时间戳与 24 位计数器生成 `ObjectId` |
| `oid.timestampSec(): u32` | 提取前 4 字节大端序 Unix 秒级时间戳 |
| `oid.eql(other: ObjectId): bool` | 逐字节比对两个 `ObjectId` 是否相等 |

### 3. `BsonValue`、`BsonField` 与辅助结构

- `BsonBinary { subtype: u8, data: []const u8 }`
- `BsonTimestamp { increment: u32, timestamp: u32 }`
- `BsonField { key: []const u8, value: BsonValue }`（提供 `BsonField.init(key, value)`）
- `BsonValue` 标签联合支持：`double: f64`、`utf8: []const u8`、`document: []BsonField`、`array: []BsonValue`、`binary: BsonBinary`、`object_id: ObjectId`、`boolean: bool`、`utc_datetime: i64`、`null_val`、`int32: i32`、`timestamp: BsonTimestamp`、`int64: i64`。
- `BsonValue` 方法：
  - `val.deinit(alloc)`: 递归释放解码分配的所有字符串、键名、切片及子文档/数组。
  - `val.get(key)`: 若当前为 `.document`，按键名查找并返回 `BsonValue?`；否则返回 `null`。
  - `val.eql(other)`: 深度结构等价比较（含 `NaN == NaN` 处理）。
- 顶层辅助函数：`deinitDocument(alloc, fields)`、`getField(fields, key)`、`eqlFields(a, b)`。

### 4. 文档编解码函数

| 函数签名 | 说明 |
| :--- | :--- |
| `encodeDocument(alloc: std.mem.Allocator, fields: []const BsonField): ([]u8)!` | 编码 BSON 1.1 小端序文档；校验 CString 无内嵌 `\x00` 与合法 UTF-8，数组键按 `"0"`, `"1"`, ... 顺序编码 |
| `decodeDocument(alloc: std.mem.Allocator, bytes: []const u8): ([]BsonField)!` | 使用默认 64 层深度上限 (`BSON_MAX_DEPTH`) 解码 BSON 文档 |
| `decodeDocumentWithMaxDepth(alloc: std.mem.Allocator, bytes: []const u8, max_depth: usize): ([]BsonField)!` | 指定最大递归深度解码 BSON 文档，超限抛出 `BsonError.MaxDepthExceeded` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    BsonField,
    BsonValue,
    ObjectId,
    deinitDocument,
    encodeDocument,
    decodeDocument,
    getField,
} from "cap:binfmt";

pub fn main(): void! {
    const alloc = std.heap.page_allocator;
    const oid = ObjectId.generate(1700000000, [1, 2, 3, 4, 5], 42);

    let fields: [2]BsonField = [
        BsonField.init("_id", BsonValue{ object_id: oid }),
        BsonField.init("name", BsonValue{ utf8: "alice" }),
    ];

    const bytes = encodeDocument(alloc, &fields)?;
    defer alloc.free(bytes);

    const decoded = decodeDocument(alloc, bytes)?;
    defer deinitDocument(alloc, decoded);

    if (v := getField(decoded, "name")) {
        // v is BsonValue.utf8("alice")
        * = v;
    }
}
```
