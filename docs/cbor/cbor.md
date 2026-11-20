# CP 标准库：RFC 8949 CBOR 与 MessagePack 紧凑二进制编解码引擎 (`cap:cbor`)

## 一、 架构总览与设计准则

`cap:cbor` 是 CP 语言官方内置的高性能紧凑二进制序列化标准库，提供 **RFC 8949 CBOR** 与 **MessagePack** 双向编解码能力。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:cbor"`，也支持 `import cap from "cap"; cap.cbor.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 紧凑二进制编解码引擎 (cap:cbor)                    │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ 共享二进制 AST        │ RFC 8949 CBOR 编解码器   │ MessagePack 编解码器  │
│ BinValue / BinMapEntry│ Cbor.encode / decode    │ MsgPack.encode/decode │
│ BinTag / BinDocument  │ Major 0-7 / Tag / f16   │ fix*/int*/uint*/bin*  │
│ Arena 统一 O(1) 释放  │ 最短编码 + 64 层深度防护 │ 最短编码 + 64 层防护   │
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/cbor/value.cp` | `// Source: cp/lib/cbor/value.cp (Shared Compact Binary Value AST for CBOR & MessagePack)` | `BinError`、`BinTag`、`BinMapEntry`、`BinValue`、`eql(other)` 与 `BinDocument`（Arena 统一释放） |
| `lib/cbor/cbor.cp` | `// Source: cp/lib/cbor/cbor.cp (RFC 8949 CBOR Encoder & Decoder)` | RFC 8949 Major 0-7 全支持、最小整数/浮点编码、Float16/32/64 解码、语义 Tag 与不定长解码 |
| `lib/cbor/msgpack.cp` | `// Source: cp/lib/cbor/msgpack.cp (MessagePack Binary Encoder & Decoder)` | MessagePack 全格式编解码（fixint/uint/int/float/str/bin/array/map）与安全边界防护 |
| `lib/cbor/index.cp` | `// Source: cp/lib/cbor/index.cp (Unified Exports for cap:cbor)` | `cap:cbor` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 共享 AST 与文档容器（`value.cp`）

- **`BinError`**：`UnexpectedEof`、`InvalidFormat`、`UnsupportedFeature`、`MaxDepthExceeded`、`TrailingBytes`、`OutOfMemory`。
- **`BinTag`**：`{ tag: u64, value: *const BinValue }`，承载 CBOR Major 6 语义标签。
- **`BinMapEntry`**：`{ key: BinValue, val: BinValue }`，支持任意 `BinValue` 作为键（整数、文本、字节等）。
- **`BinValue`**：带载荷联合枚举：
  - 变体：`Null`、`Bool(bool)`、`Uint(u64)`、`Int(i64)`、`Float(f64)`、`Bytes([]const u8)`、`Text([]const u8)`、`Array([]BinValue)`、`Map([]BinMapEntry)`、`Tag(BinTag)`；
  - 工厂构造：`makeNull()`、`makeBool(v)`、`makeUint(v)`、`makeInt(v)`、`makeFloat(v)`、`makeBytes(v)`、`makeText(v)`、`makeArray(items)`、`makeMap(entries)`、`makeTag(tag, ptr)`、`allocTag(alloc, tag, inner)`；
  - 查询与比较：`isNull()`、`asBool()`、`asUint()`、`asInt()`、`asFloat()`、`asBytes()`、`asText()`、`asArray()`、`asMap()`、`asTag()`、`get(key_text)`、`getIndex(idx)`、`len()`、`eql(other)`。
- **`BinDocument`**：封装 `root: BinValue` 与内部 `std.heap.ArenaAllocator`，调用 `doc.deinit()` 即可 O(1) 一次性释放整棵解码树的所有字符串、字节串、数组、Map 与 Tag 节点内存。

### 2. RFC 8949 CBOR 编解码器（`cbor.cp`）

- **`Cbor.encode(alloc, val): ([]u8)!`**：按 RFC 8949 优选最短整数与 IEEE 754 最短浮点（Float16/Float32/Float64）编码全部 Major 0-7 类型。
- **`Cbor.decode(alloc, bytes): BinDocument!`** / **`Cbor.decodeValue(alloc, bytes): BinValue!`**（Arena 直出）：
  - 完整解码 Major 0（`u64`）、Major 1（`-1 - n` 负整数，支持至 `i64` 最小值）、Major 2（定长/不定长字节串）、Major 3（定长/不定长 UTF-8 校验文本串）、Major 4（定长/不定长数组）、Major 5（定长/不定长 Map）、Major 6（Tag 0/1/24/任意 `u64` 标签）、Major 7（`false`/`true`/`null`/`undefined` 及 IEEE 754 Float16/Float32/Float64）；
  - 最大嵌套深度 `CBOR_MAX_DEPTH = 64`，拦截截断报文（`UnexpectedEof`）与尾随垃圾字节（`TrailingBytes`）。

### 3. MessagePack 编解码器（`msgpack.cp`）

- **`MsgPack.encode(alloc, val): ([]u8)!`**：按最小空间编码 `positive/negative fixint`、`uint8/16/32/64`、`int8/16/32/64`、`nil`、`bool`、`float32/64`、`fixstr/str8/16/32`、`bin8/16/32`、`fixarray/array16/32`、`fixmap/map16/32`。
- **`MsgPack.decode(alloc, bytes): BinDocument!`** / **`MsgPack.decodeValue(alloc, bytes): BinValue!`**（Arena 直出）：
  - 完整解码 MessagePack 规范类型；最大嵌套深度 `MSGPACK_MAX_DEPTH = 64`，拒绝截断、保留字节 `0xC1`、非法 UTF-8 及尾随字节。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { BinValue, BinMapEntry, Cbor, MsgPack } from "cap:cbor";
import cap from "cap";

pub fn demoCborAndMsgPack()! {
    const alloc = std.heap.page_allocator;

    let entries: [2]BinMapEntry = [
        BinMapEntry.init(BinValue.makeText("model"), BinValue.makeText("cp-agent")),
        BinMapEntry.init(BinValue.makeText("score"), BinValue.makeFloat(1.0)),
    ];
    const doc_val = BinValue.makeMap(&entries);

    // 1. CBOR 编码与解码
    const cbor_bytes = Cbor.encode(alloc, doc_val)?;
    defer alloc.free(cbor_bytes);

    let decoded_cbor = Cbor.decode(alloc, cbor_bytes)?;
    defer decoded_cbor.deinit();
    const same_cbor = decoded_cbor.root.eql(doc_val);

    // 2. MessagePack 编码与动态 cap.cbor 命名空间解码
    const mp_bytes = MsgPack.encode(alloc, doc_val)?;
    defer alloc.free(mp_bytes);

    let decoded_mp = cap.cbor.MsgPack.decode(alloc, mp_bytes)?;
    defer decoded_mp.deinit();
    const same_mp = decoded_mp.root.eql(doc_val);
    * = same_cbor && same_mp;
}
```
