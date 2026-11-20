# CP 标准库：MQTT 3.1.1 二进制协议编解码与增量流式拆包引擎手册 (`cap:mqtt` / `cap.mqtt`)

## 一、 架构总览与设计准则

`cap:mqtt` 是 CP 语言官方内置的纯原生、零 C 依赖 **MQTT 3.1.1 二进制 Wire 协议编解码与增量流式拆包引擎**，专为 IoT 边缘计算、消息中间件客户端及分布式设备通信打造（可与 `cap:pubsub` 的 `TopicStyle.Mqtt` 主题匹配无缝协同）。

核心特性：
1. **严格符合 MQTT 3.1.1 变长 Remaining Length 编解码**：
   - 支持 1..4 字节变长编码，理论上限 `268,435,455`（`0x0FFFFFFF`）；
   - 严格拦截第 4 字节延续位 `0x80` 置位（返回 `MqttError.MalformedRemainingLength`）与超限长度（返回 `MqttError.PacketTooLarge`）。
2. **全报文类型强类型覆盖与协议级合规校验**：
   - 完整覆盖 `CONNECT`、`CONNACK`、`PUBLISH`（QoS 0/1/2、DUP、RETAIN、可选 `packet_id`）、`PUBACK`、`PUBREC`、`PUBREL`、`PUBCOMP`、`SUBSCRIBE`、`SUBACK`、`UNSUBSCRIBE`、`UNSUBACK`、`PINGREQ`、`PINGRESP`、`DISCONNECT`。
   - 严格校验 `QoS > 0` 的 `PUBLISH` 必须携带非零 `packet_id`（否则返回 `MqttError.MissingPacketId`）；`PUBREL`、`SUBSCRIBE`、`UNSUBSCRIBE` 强制写入并校验 Fixed Header 固定标志位 `0b0010`；空主题列表返回 `MqttError.EmptyTopics`。
3. **增量流式拆包器（`MqttStreamDecoder`）**：
   - 原生支持 TCP 分片半包等待（逐字节喂入仍可正确还原）、多报文黏包连续拆分，以及基于 `max_packet_bytes` 的超大报文提前拦截（`MqttError.PacketTooLarge`），避免内存耗尽。
4. **确定性所有权与零内存泄漏**：
   - `MqttCodec.decodeOne` 与 `MqttStreamDecoder.nextPacket` 返回拥有独立堆切片的 `MqttPacket`，调用 `pkt.deinit(alloc)` 确定性释放所有内部分配。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:mqtt"` 与 `import cap from "cap"; cap.mqtt.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/mqtt/packet.cp` | `// Source: cp/lib/mqtt/packet.cp (Pure CP MQTT 3.1.1 Wire Codec and Incremental Stream Decoder)` | `MqttError`、报文类型与数据结构、`MqttCodec`、`MqttStreamDecoder` |
| `lib/mqtt/index.cp` | `// Source: cp/lib/mqtt/index.cp (Unified Exports for cap:mqtt)` | `cap:mqtt` 与 `cap.mqtt` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、常量与基础枚举

- **`MAX_REMAINING_LENGTH: usize = 268435455`**
- **`MqttError`**：`InvalidPacketType`、`MalformedRemainingLength`、`InvalidProtocolName`、`InvalidQos`、`MissingPacketId`、`PacketTooLarge`、`Truncated`、`EmptyTopics`、`OutOfMemory`
- **`MqttPacketType: u8`**：`Connect = 1`、`ConnAck = 2`、`Publish = 3`、`PubAck = 4`、`PubRec = 5`、`PubRel = 6`、`PubComp = 7`、`Subscribe = 8`、`SubAck = 9`、`Unsubscribe = 10`、`UnsubAck = 11`、`PingReq = 12`、`PingResp = 13`、`Disconnect = 14`
- **`MqttQos: u8`**：`AtMostOnce = 0`、`AtLeastOnce = 1`、`ExactlyOnce = 2`
- **`ConnAckCode: u8`**：`Accepted = 0`、`UnacceptableProtocolVersion = 1`、`IdentifierRejected = 2`、`ServerUnavailable = 3`、`BadUserNameOrPassword = 4`、`NotAuthorized = 5`

### 2. 报文数据结构与 `MqttPacket`

- `MqttWill`：`topic: string`、`message: []const u8`、`qos: MqttQos`、`retain: bool`
- `ConnectPacket`：`protocol_name`（默认 `"MQTT"`）、`protocol_level`（默认 `4`）、`clean_session`、`keep_alive`、`client_id`、`will: MqttWill?`、`username: string?`、`password: ([]const u8)?`
- `ConnAckPacket`：`session_present: bool`、`return_code: ConnAckCode`
- `PublishPacket`：`dup: bool`、`qos: MqttQos`、`retain: bool`、`topic: string`、`packet_id: u16?`、`payload: []const u8`
- `PacketIdOnly`：`packet_id: u16`（用于 `PubAck`、`PubRec`、`PubRel`、`PubComp`、`UnsubAck`）
- `TopicFilter`：`topic: string`、`qos: MqttQos`
- `SubscribePacket`：`packet_id: u16`、`topics: []const TopicFilter`
- `SubAckPacket`：`packet_id: u16`、`return_codes: []const u8`
- `UnsubscribePacket`：`packet_id: u16`、`topics: []const string`
- **`MqttPacket`**（tagged union）：包含上述全部报文变体及 `PingReq`、`PingResp`、`Disconnect`，并提供 `deinit(*_, alloc: std.mem.Allocator)` 释放解码分配的内存。

### 3. 编解码器（`MqttCodec`）

- `static encodeRemainingLength(len: usize, out_buf: []u8): usize ! MqttError`
- `static decodeRemainingLength(bytes: []const u8): RemainingLengthResult ! MqttError`
- `static encode(alloc: std.mem.Allocator, pkt: MqttPacket): []u8 ! MqttError`
- `static decodeOne(alloc: std.mem.Allocator, buf: []const u8): DecodeResult ! MqttError`

### 4. 增量流式解码器（`MqttStreamDecoder`）

- `static init(alloc: std.mem.Allocator, max_packet_bytes: usize): MqttStreamDecoder`
- `deinit(*_)`
- `feed(*_, chunk: []const u8) ! MqttError`
- `bufferedLen(): usize`
- `nextPacket(*_, alloc: std.mem.Allocator): MqttPacket? ! MqttError`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    MqttPacket,
    PublishPacket,
    MqttQos,
    MqttCodec,
    MqttStreamDecoder,
} from "cap:mqtt";

const std = @import("std");

pub fn demoMqtt(): ! {
    const alloc = std.heap.page_allocator;

    const wire = MqttCodec.encode(alloc, MqttPacket{
        Publish: PublishPacket{
            dup: false,
            qos: MqttQos.AtLeastOnce,
            retain: true,
            topic: "sensors/rack1/temp",
            packet_id: 42,
            payload: "39.8",
        },
    })?;
    defer alloc.free(wire);

    let decoder = MqttStreamDecoder.init(alloc, 64 * 1024);
    defer decoder.deinit();

    // 模拟半包喂入
    decoder.feed(wire[0..5])?;
    const none_yet = decoder.nextPacket(alloc)?; // null

    decoder.feed(wire[5..])?;
    if (pkt := decoder.nextPacket(alloc)?) {
        let owned = pkt;
        defer owned.deinit(alloc);
        // owned 为 PublishPacket
    }

    // cap.mqtt 动态挂载等价用法
    const ping_bytes = cap.mqtt.MqttCodec.encode(alloc, cap.mqtt.MqttPacket.PingReq)?;
    defer alloc.free(ping_bytes);
}
```
