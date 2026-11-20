# CP 标准库：Protocol Buffers 线缆编解码与 gRPC 传输原语引擎 (`cap:protobuf` / `cap.protobuf`)

## 一、 架构总览与设计准则

`cap:protobuf` 是 CP 语言官方内置的纯原生 Protocol Buffers 二进制线缆编解码与 gRPC 分帧引擎，专为高性能微服务、AI Agent 集群通信与跨节点 RPC 协议打造。

核心特性：
1. **流式零拷贝解码 (`ProtoReader` / `ProtoField`)**：直接基于输入切片 `[]const u8` 游标流式迭代 `next() -> ProtoField?!`，`asBytes()` / `asString()` / `readSubmessage(field)` 零堆分配返回原缓冲视图；支持未知字段按 `ProtoField` 前向跳过 (`skipField(field)`)。
2. **完整 Proto3 线缆编码 (`ProtoWriter`)**：支持 LEB128 变长整数 (`writeVarint`)、`ZigZag` 有符号编码 (`zigzagEncode32/64`、`writeSint32/64`)、`writeInt32/64`、`writeBool`、`writeFixed32/64`、`writeFloat/Double`、`writeBytes`、`writeString`、`writeSubmessage` 与 `writePackedVarints`。
3. **严格协议安全校验**：
   - 字段号严格校验 `1..536870911`（拒绝 `0` 号字段与超限字段号，返回 `ProtoError.InvalidFieldNumber`）；
   - 10 字节 Varint 溢出防护（超过 10 字节或第 10 字节溢出立即返回 `ProtoError.VarintOverflow`）；
   - WireType 合法性校验（`6`/`7` 返回 `ProtoError.InvalidWireType`）与长度前缀越界检查（`ProtoError.TruncatedLengthDelimited`）；
   - 嵌套子消息最大递归深度保护（默认 `DEFAULT_MAX_DEPTH = 64`，超限返回 `ProtoError.MaxDepthExceeded`）；
   - gRPC 5 字节分帧默认 4 MiB (`DEFAULT_MAX_MESSAGE_BYTES`) 内存耗尽防护。
4. **gRPC 协议基元 (`grpc.cp`)**：内置 17 个标准 `GrpcStatus` 状态码、`GrpcFrameCodec` 与跨 chunk 增量分帧器 `GrpcFrameStream`，以及 `grpc-timeout` 纳秒级编解码器 `GrpcTimeout`。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:protobuf"` 与 `import cap from "cap"; cap.protobuf.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/protobuf/protobuf.cp` | `// Source: cp/lib/protobuf/protobuf.cp (Protocol Buffers Wire Format Zero-Copy Codec)` | `ProtoError`、`WireType`、`zigzagEncode/Decode32/64`、`ProtoField`、`ProtoWriter`、`ProtoReader` |
| `lib/protobuf/grpc.cp` | `// Source: cp/lib/protobuf/grpc.cp (gRPC Length-Prefixed Framing, Status Codes & Timeout Codec)` | `GrpcError`、`GrpcStatus`、`GrpcFrame`、`GrpcFrameCodec`、`GrpcFrameStream`、`GrpcTimeout` |
| `lib/protobuf/index.cp` | `// Source: cp/lib/protobuf/index.cp (Unified Exports for cap:protobuf)` | `cap:protobuf` 与 `cap.protobuf` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. Protobuf 线缆编解码（`protobuf.cp`）

- **`ProtoError`**：`UnexpectedEof`、`VarintOverflow`、`InvalidWireType`、`InvalidFieldNumber`、`TruncatedLengthDelimited`、`MaxDepthExceeded`、`OutOfMemory`
- **`WireType` (`u8`)**：`Varint = 0`、`Fixed64 = 1`、`LengthDelimited = 2`、`StartGroup = 3`、`EndGroup = 4`、`Fixed32 = 5`；方法 `fromInt(v): WireType!`、`toInt(): u8`
- **ZigZag 编解码函数**：
  - `zigzagEncode32(v: i32): u32`、`zigzagDecode32(v: u32): i32`
  - `zigzagEncode64(v: i64): u64`、`zigzagDecode64(v: u64): i64`
- **`ProtoWriter`**：
  - 生命周期：`init(allocator)`、`deinit(*_)`、`clear(*_)`、`len(): usize`、`slice(): []const u8`、`toOwnedSlice(*_): ([]u8)!`
  - 写入：`writeTag(field_num, wire_type)`、`writeVarint(v: u64)`、`writeUint32(field_num, v)`、`writeUint64(field_num, v)`、`writeInt32(field_num, v)`、`writeInt64(field_num, v)`、`writeSint32(field_num, v)`、`writeSint64(field_num, v)`、`writeBool(field_num, v)`、`writeFixed32(field_num, v)`、`writeFixed64(field_num, v)`、`writeFloat(field_num, v)`、`writeDouble(field_num, v)`、`writeBytes(field_num, data)`、`writeString(field_num, s)`、`writeSubmessage(field_num, sub_bytes)`、`writePackedVarints(field_num, vals)`
- **`ProtoField`**：
  - 字段：`field_number: u32`、`wire_type: WireType`、`raw_varint: u64`、`raw_fixed64: u64`、`raw_fixed32: u32`、`raw_bytes: []const u8`
  - 零拷贝辅助访问：`asUint64()`、`asUint32()`、`asInt64()`、`asInt32()`、`asSint32()`、`asSint64()`、`asBool()`、`asFixed32()`、`asFixed64()`、`asSfixed32()`、`asSfixed64()`、`asFloat()`、`asDouble()`、`asBytes()`、`asString()`
- **`ProtoReader`**：
  - 构造与游标：`init(data: []const u8)`、`initWithMaxDepth(data: []const u8, max_depth: usize)`、`hasMore()`、`remaining()`、`position()`、`reset(*_)`
  - 流式解码与递归：`next(*_): ProtoField?!`、`skipField(*_, field: ProtoField): !`、`readSubmessage(field: ProtoField): ProtoReader!`、`decodePackedVarints(allocator, raw_bytes): ([]u64)!`

### 2. gRPC 分帧、状态码与超时（`grpc.cp`）

- **`GrpcStatus` (`u8`)**：完整 17 个标准 gRPC 状态码（`0=Ok` .. `16=Unauthenticated`），提供 `fromCode(code)`、`code()`、`name()`、`isOk()`、`isRetryable()`。
- **`GrpcFrameCodec`**：`encode(alloc, compressed, payload)`、`decode(data)`、`decodeWithLimit(data, max_bytes)` 等。
- **`GrpcFrameStream`**：跨 chunk 增量分帧解码器 `init(alloc)`、`feed(*_, chunk)`、`freeFrames(alloc, frames)`、`deinit(*_)`。
- **`GrpcTimeout`**：`parse(raw): u64!`、`formatMs(alloc, ms): ([]u8)!`。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { ProtoWriter, ProtoReader } from "cap:protobuf";

pub fn demoProtobuf(): !void {
    const alloc = std.heap.page_allocator;

    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    w.writeInt32(1, 150)?;
    w.writeString(2, "agent-node-01")?;
    w.writeSint32(3, -42)?;
    const packed_vals: [3]u64 = [3, 270, 86942];
    w.writePackedVarints(4, &packed_vals)?;

    let r = cap.protobuf.ProtoReader.init(w.slice());
    while (field := r.next()?) {
        switch (field.field_number) {
            1 => { * = field.asInt32(); },
            2 => { * = field.asString(); },
            3 => { * = field.asSint32(); },
            4 => {
                const nums = ProtoReader.decodePackedVarints(alloc, field.asBytes())?;
                defer alloc.free(nums);
            },
            else => r.skipField(field)?,
        }
    }
}
```
