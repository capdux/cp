# CP 标准库：Protocol Buffers v3 与 gRPC 线缆协议引擎 (`cap:protobuf` / `cap.protobuf`)

## 一、 架构总览与设计准则

`cap:protobuf` 是 CP 语言官方内置的纯原生 Protocol Buffers v3 二进制线缆编解码与 gRPC 传输层原语库，专为高性能微服务、AI Agent 集群通信与跨节点 gRPC 协议引擎打造。

核心特性：
1. **零拷贝解码 (`ProtoReader`)**：直接基于输入切片 `[]const u8` 游标读取，`readBytes()` / `readString()` / `readSubmessage()` 零堆分配返回原缓冲视图；支持未知字段按 `WireType` 前向跳过 (`skipField`)。
2. **完整 Proto3 线缆编码 (`ProtoWriter`)**：支持 LEB128 变长整数 (`Varint`)、`ZigZag` 有符号编码 (`sint32` / `sint64`)、`Fixed32` / `Fixed64` / `Float` / `Double`、嵌套子消息 (`writeSubmessage`) 与无临时分配的 `writePackedVarints`。
3. **严格协议安全校验**：
   - 字段号范围严格校验 `1..536870911`，拒绝保留段 `19000..19999`；
   - 10 字节 Varint 溢出防护（第 10 字节续传位或高比特溢出立即报错 `ProtoError.VarintOverflow`）；
   - gRPC 5 字节分帧默认 4 MiB (`DEFAULT_MAX_MESSAGE_BYTES`) 内存耗尽防护。
4. **gRPC 协议基元 (`grpc.cp`)**：内置 17 个标准 `GrpcStatus` 状态码、`GrpcFrameCodec` 与跨 chunk 增量分帧器 `GrpcFrameStream`，以及 `grpc-timeout` (`H`/`M`/`S`/`m`/`u`/`n`) 纳秒级编解码器 `GrpcTimeout`。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:protobuf"` 与 `import cap from "cap"; cap.protobuf.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/protobuf/wire.cp` | `// Source: cp/lib/protobuf/wire.cp (Protocol Buffers v3 Wire Format Zero-Copy Codec)` | `ProtoError`、`WireType`、`ZigZag`、`ProtoTag`、`ProtoWriter`、`ProtoReader` |
| `lib/protobuf/grpc.cp` | `// Source: cp/lib/protobuf/grpc.cp (gRPC Length-Prefixed Framing, Status Codes & Timeout Codec)` | `GrpcError`、`GrpcStatus`、`GrpcFrame`、`GrpcFrameCodec`、`GrpcFrameStream`、`GrpcTimeout` |
| `lib/protobuf/index.cp` | `// Source: cp/lib/protobuf/index.cp (Unified Exports for cap:protobuf)` | `cap:protobuf` 与 `cap.protobuf` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. Proto3 线缆编解码（`wire.cp`）

- **`ProtoError`**：`UnexpectedEof`、`VarintOverflow`、`InvalidWireType`、`InvalidFieldNumber`、`InvalidLength`、`OutOfMemory`
- **`WireType` (`u8`)**：`Varint = 0`、`Fixed64 = 1`、`Len = 2`、`Fixed32 = 5`；方法 `fromInt(v): WireType!`、`toInt(): u8`
- **`ZigZag`**：`encode32(n: i32): u32`、`decode32(n: u32): i32`、`encode64(n: i64): u64`、`decode64(n: u64): i64`
- **`ProtoTag`**：字段 `field_number: u32`、`wire_type: WireType`；方法 `init(field_number, wire_type): ProtoTag!`、`decode(raw): ProtoTag!`、`encode(): u32`、`isValidFieldNumber(n): bool`
- **`ProtoWriter`**：
  - 生命周期：`init(alloc)`、`deinit(*_)`、`clear(*_)`、`len(): usize`、`slice(): []const u8`、`toOwnedSlice(*_): ([]u8)!`
  - 写入：`writeTag`、`writeRawVarint`、`writeVarint`、`writeInt32`、`writeInt64`、`writeSint32`、`writeSint64`、`writeBool`、`writeFixed32`、`writeFixed64`、`writeFloat`、`writeDouble`、`writeBytes`、`writeString`、`writeSubmessage`、`writePackedVarints`
- **`ProtoReader`**：
  - 构造与游标：`init(data: []const u8)`、`hasMore()`、`remaining()`、`position()`、`reset(*_)`
  - 读取与前向兼容：`nextTag(*_): ProtoTag?!`、`readVarint`、`readInt32`、`readInt64`、`readSint32`、`readSint64`、`readBool`、`readFixed32`、`readFixed64`、`readFloat`、`readDouble`、`readBytes`（零拷贝切片）、`readString`、`readSubmessage`、`readPackedVarints(alloc)`、`skipField(*_, wire_type)`

### 2. gRPC 分帧、状态码与超时（`grpc.cp`）

- **`GrpcStatus` (`u8`)**：完整 17 个标准 gRPC 状态码（`0=Ok` .. `16=Unauthenticated`），提供 `fromCode(code): GrpcStatus?`、`code(): i32`、`name(): []const u8`、`isOk(): bool`、`isRetryable(): bool`（对 `Unavailable`、`ResourceExhausted`、`Aborted` 返回 `true`）。
- **`GrpcFrame`**：`compressed: bool`、`payload: []const u8`
- **`GrpcFrameCodec`**：`encode(alloc, compressed, payload)`、`encodeWithLimit(alloc, compressed, payload, max_bytes)`、`encodeFrame(alloc, frame)`、`encodeFrameWithLimit(alloc, frame, max_bytes)`、`decodeHeader(header)`、`decodeHeaderWithLimit(header, max_bytes)`、`decode(data)`、`decodeWithLimit(data, max_bytes)`
- **`GrpcFrameStream`**：增量跨 chunk 分帧解码器：`init(alloc)`、`initWithLimit(alloc, max_message_bytes)`、`deinit(*_)`、`reset(*_)`、`bufferedBytes()`、`feed(*_, chunk): ([]GrpcFrame)!`、`freeFrames(alloc, frames)`
- **`GrpcTimeout`**：`parse(raw): u64!`（解析 `"100m"`/`"5S"`/`"2H"` 等为纳秒）、`formatMs(alloc, ms): ([]u8)!`

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { ProtoWriter, ProtoReader, GrpcFrameCodec, GrpcStatus, GrpcTimeout } from "cap:protobuf";

pub fn demoProtobufGrpc(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 编码 Proto3 消息
    let w = ProtoWriter.init(alloc);
    defer w.deinit();
    w.writeVarint(1, 150)?;
    w.writeString(2, "agent-node-01")?;
    w.writeSint32(3, -42)?;

    // 2. 封装为 gRPC 5 字节头分帧
    const framed = GrpcFrameCodec.encode(alloc, false, w.slice())?;
    defer alloc.free(framed);

    // 3. 解帧并零拷贝解码（同时验证 cap.protobuf 动态挂载等价性）
    const frame = cap.protobuf.GrpcFrameCodec.decode(framed)?;
    let r = ProtoReader.init(frame.payload);
    while (tag := r.nextTag()?) {
        switch (tag.field_number) {
            1 => { * = r.readVarint()?; },
            2 => { * = r.readString()?; },
            3 => { * = r.readSint32()?; },
            else => r.skipField(tag.wire_type)?,
        }
    }

    const ns = GrpcTimeout.parse("100m")?;
    * = ns;
    * = GrpcStatus.Unavailable.isRetryable();
}
```
