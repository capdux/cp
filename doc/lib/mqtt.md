# `cap:mqtt` — 纯 CP 工业级 MQTT v3.1.1 / v5.0 协议库

## 1. 概述

`cap:mqtt`（同时挂载于 `cap.mqtt`）提供零外部依赖、无 GC 的纯 CP MQTT v3.1.1 / v5.0 二进制报文编解码、增量流式拆包分帧器以及遵循 MQTT §4.7 规范的 Topic 通配符路由匹配与校验能力，支持 `std.testing.allocator` 零内存泄漏验证。

## 2. 核心类型

- **`MqttError`**：协议与校验错误集，包含 `MalformedVarInt`、`IncompletePacket`、`PacketTooLarge`、`InvalidPacketType`、`InvalidFlags`、`MalformedPacket`、`InvalidUtf8`、`InvalidTopicFilter`、`InvalidTopicName`、`InvalidQoS`、`MissingPacketId`、`UnexpectedPacketId`、`UnsupportedVersion`。
- **`MqttVersion`**：协议版本枚举（`v3_1_1 = 4`、`v5_0 = 5`）。
- **`QoS`**：服务质量枚举（`at_most_once = 0`、`at_least_once = 1`、`exactly_once = 2`）。
- **`PacketType`**：MQTT 控制报文类型枚举（`connect`..`auth`）。
- **`MqttPacket`**：统一标签联合体报文模型，涵盖 `connect`、`connack`、`publish`、`puback`、`pubrec`、`pubrel`、`pubcomp`、`subscribe`、`suback`、`unsubscribe`、`unsuback`、`pingreq`、`pingresp`、`disconnect`，提供 `deinit(alloc)` 释放可变长切片。
- **`MqttStreamDecoder`**：增量流式分帧器，支持任意半包/黏包切片 `feed(chunk)` 与 `nextPacket()`，内置 `max_packet_bytes` 资源耗尽拦截与 `deinit()` 清理。

## 3. API 速查

| 函数 / 方法 | 签名 | 说明 |
| --- | --- | --- |
| `encodeVarByteInt` | `(value: usize, out: *[4]u8): MqttError!usize` | 编码 1..4 字节变长整数（上限 `268,435,455`） |
| `decodeVarByteInt` | `(data: []const u8): MqttError!VarIntResult` | 解码变长整数，拦截第 4 字节延续位溢出 |
| `encodePacket` | `(alloc: Allocator, packet: MqttPacket): ![]u8` | 编码并校验 MQTT 报文为二进制帧 |
| `decodePacket` | `(alloc: Allocator, data: []const u8): !MqttPacket` | 解码并校验完整二进制 MQTT 报文 |
| `MqttStreamDecoder.init` | `(alloc: Allocator, max_packet_bytes: usize): MqttStreamDecoder` | 初始化流式拆包器 |
| `MqttStreamDecoder.feed` | `(*_, chunk: []const u8): !void` | 喂入任意长度网络分片 |
| `MqttStreamDecoder.nextPacket` | `(*_): !MqttPacket?` | 提取下一帧完整报文，不足则返回 `null` |
| `validateTopicName` | `(topic: []const u8): !void` | 校验发布主题名（禁止 `+`、`#` 与 `\x00`） |
| `validateTopicFilter` | `(filter: []const u8): !void` | 校验订阅过滤器（`#` 仅限末级、`+` 须独占整级） |
| `topicMatches` | `(filter: []const u8, topic: []const u8): bool` | 按 MQTT §4.7 匹配主题（含 `$SYS` 隔离） |

## 4. 示例

```cp
import { MqttPacket, QoS, encodePacket, MqttStreamDecoder, topicMatches } from "cap:mqtt";
import cap from "cap";

const alloc = std.heap.page_allocator;
const wire = try encodePacket(alloc, MqttPacket{
    .publish = .{
        .dup = false,
        .qos = QoS.at_least_once,
        .retain = false,
        .topic = "sensors/room1/temp",
        .packet_id = 42,
        .payload = "23.5",
    },
});
defer alloc.free(wire);

let dec = MqttStreamDecoder.init(alloc, 4096);
defer dec.deinit();
try dec.feed(wire[0..3]);
try dec.feed(wire[3..]);
if (pkt := try dec.nextPacket()) {
    defer pkt.deinit(alloc);
    const matched = cap.mqtt.topicMatches("sensors/+/temp", "sensors/room1/temp");
    _ = matched;
}
```
