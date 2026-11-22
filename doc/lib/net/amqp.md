# CP 标准库：AMQP 0-9-1 二进制线协议帧与 FieldTable 编解码器 (`cap:amqp`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

## 一、 架构总览与设计准则

`cap:amqp` 是 CP 语言官方内置的 **AMQP 0-9-1（Advanced Message Queuing Protocol）** 纯 CP 二进制线协议编解码与增量拆帧标准库。严格遵循 AMQP 0-9-1 大端序（Big-Endian）帧格式与 `FieldTable` 递归编解码规范，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:net"` 静态导入，也支持 `import cap from "cap"; cap.amqp.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│             CP AMQP 0-9-1 协议帧与 FieldTable 引擎 (cap:amqp)            │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ 线协议帧编解码         │ FieldTable & AmqpValue  │ 增量流式拆帧与防护     │
│ Method / Header       │ boolean/i32/i64/f64     │ AmqpStreamDecoder     │
│ Body / Heartbeat      │ long_str/field_array    │ 半包/黏包增量拆帧      │
│ 大端序 + 0xCE 帧尾     │ 嵌套 field_table/void   │ 32 层 MaxDepthExceeded │
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/amqp/amqp.cp` | `// Source: cp/lib/amqp/amqp.cp (AMQP 0-9-1 Binary Wire Framing, FieldTable & Stream Decoder)` | 协议常量、`AmqpError`、`AmqpFrameType`、`AmqpValue`/`AmqpField`、`FieldTable` 编解码、Method/ContentHeader/Heartbeat 帧辅助、`AmqpStreamDecoder` |
| `lib/amqp/index.cp` | `// Source: cp/lib/amqp/index.cp (Unified Exports for cap:amqp)` | `cap:amqp` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 协议常量与错误集 `AmqpError`

- `PROTOCOL_HEADER: []const u8 = "AMQP\x00\x00\x09\x01"`（8 字节 AMQP 0-9-1 协议头）
- `FRAME_END: u8 = 0xCE`（帧尾定界字节）
- `AMQP_MAX_DEPTH: usize = 32`（`FieldTable`/`field_array` 最大递归深度）
- `DEFAULT_MAX_FRAME_SIZE: usize = 131072`（128 KiB 默认最大帧载荷）

```typescript
pub err AmqpError {
    InvalidFrameEnd,   // 帧尾字节非 0xCE
    InvalidFrameType,  // 非法帧类型（仅允许 1=Method, 2=Header, 3=Body, 8=Heartbeat）
    UnexpectedEof,     // 缓冲区提前结束或负载不足
    FrameTooLarge,     // 帧载荷超出 max_frame_size 或 u32 上限
    InvalidFieldType,  // 未知 FieldTable 值类型标签
    StringTooLong,     // shortstr 超过 255 字节或 longstr 越界
    MaxDepthExceeded,  // FieldTable / field_array 递归超过 32 层
    InvalidChannel,    // Heartbeat 帧使用了非 0 channel
    OutOfMemory,       // 内存分配失败
}
```

### 2. 帧类型与帧结构 (`AmqpFrameType`, `AmqpFrame`)

```typescript
pub enum AmqpFrameType: u8 {
    Method = 1,
    Header = 2,
    Body = 3,
    Heartbeat = 8,
}
```

- `AmqpFrame { frame_type: AmqpFrameType, channel: u16, payload: []const u8 }`：提供 `frame.deinit(alloc)` 释放解帧分配的 `payload`。
- `AmqpMethodPayload { class_id: u16, method_id: u16, args: []const u8 }`
- `AmqpContentHeader { class_id: u16, weight: u16, body_size: u64, content_type: ([]const u8)?, headers: []AmqpField }`：提供 `hdr.deinit(alloc)` 递归释放 `content_type` 与 `headers`。

### 3. `AmqpValue` 与 `AmqpField`（`FieldTable` AST）

- `AmqpField { name: []const u8, value: AmqpValue }`（提供 `AmqpField.init(name, value)`）
- `AmqpValue` 标签联合支持：
  - `'t'` `boolean: bool`
  - `'I'` `i32_val: i32`
  - `'l'` `i64_val: i64`
  - `'d'` `f64_val: f64`
  - `'S'` `long_str: []const u8`
  - `'A'` `field_array: []AmqpValue`
  - `'F'` `field_table: []AmqpField`
  - `'V'` `void_val`
- 方法与辅助函数：`val.deinit(alloc)`、`val.get(name)`、`val.eql(other)`、`deinitFieldTable(alloc, fields)`、`getField(fields, name)`、`eqlFieldTable(a, b)`。
- 编解码函数：
  - `encodeFieldTable(alloc, fields): ([]u8)!`
  - `decodeFieldTable(alloc, bytes): ([]AmqpField)!`
  - `decodeFieldTableWithMaxDepth(alloc, bytes, max_depth): ([]AmqpField)!`

### 4. 帧编码与高级辅助函数

| 函数签名 | 说明 |
| :--- | :--- |
| `encodeFrame(alloc, frame_type, channel, payload): ([]u8)!` | 编码通用 AMQP 帧 `[type:1][channel:2 BE][size:4 BE][payload][0xCE:1]`；`Heartbeat` 非 0 channel 返回 `InvalidChannel` |
| `encodeMethodFrame(alloc, channel, class_id, method_id, args): ([]u8)!` | 编码 Method 帧（`Basic.Publish` 等） |
| `decodeMethodPayload(payload): AmqpMethodPayload!` | 解析 Method 帧载荷中的 `class_id`、`method_id` 与 `args` |
| `encodeContentHeader(alloc, channel, class_id, body_size, content_type, headers): ([]u8)!` | 编码 Content Header 帧（支持 `content_type` 与 `headers` `FieldTable` 属性标志） |
| `decodeContentHeader(alloc, payload): AmqpContentHeader!` | 解析 Content Header 帧载荷 |
| `encodeHeartbeat(alloc): ([]u8)!` | 编码 channel=0 空载荷 Heartbeat 帧（8 字节） |

### 5. 增量流式拆帧器 (`AmqpStreamDecoder`)

| 方法签名 | 说明 |
| :--- | :--- |
| `AmqpStreamDecoder.init(alloc, max_frame_size): AmqpStreamDecoder` | 初始化流式拆帧器并设置单帧最大载荷上限 |
| `decoder.deinit()` | 释放内部累积缓冲区 |
| `decoder.feed(chunk): void!` | 喂入任意长度的网络分块字节 |
| `decoder.nextFrame(): (AmqpFrame?)!` | 尝试解析下一完整帧；半包返回 `null`，校验类型、channel、上限及 `0xCE` 帧尾 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    PROTOCOL_HEADER,
    AmqpField,
    AmqpValue,
    AmqpStreamDecoder,
    encodeMethodFrame,
    encodeContentHeader,
    encodeFrame,
    encodeHeartbeat,
    decodeMethodPayload,
    decodeContentHeader,
} from "cap:net";

pub fn main(): void! {
    const alloc = std.heap.page_allocator;
    let headers: [1]AmqpField = [
        AmqpField.init("x-trace-id", AmqpValue{ long_str: "req-42" }),
    ];
    const hdr_frame = encodeContentHeader(alloc, 1, 60, 12, "application/json", &headers)?;
    defer alloc.free(hdr_frame);

    let decoder = AmqpStreamDecoder.init(alloc, 65536);
    defer decoder.deinit();
    decoder.feed(hdr_frame)?;
    if (frame := decoder.nextFrame()?) {
        defer frame.deinit(alloc);
        const hdr = decodeContentHeader(alloc, frame.payload)?;
        defer hdr.deinit(alloc);
    }
}
```
