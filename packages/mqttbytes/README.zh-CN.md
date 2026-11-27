[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mqttbytes

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap OASIS MQTT v3.1.1 binary control packet encoder/decoder with 1..4-byte Remaining Length, PUBLISH/PUBACK/PINGREQ/PINGRESP/DISCONNECT, and partial-frame detection.

## 安装与导入

```bash
cap add mqttbytes
```

```cp
import { makeHeaderStatus } from "mqttbytes";
```

## 核心导出 API

- `export fn makeHeaderStatus(status: i32): MqttHeader;`
- `export fn makePublishStatus(status: i32): MqttPublish;`
- `export fn encodeRemainingLen(len: u32, dst: []u8): i64;`
- `export fn decodeHeader(buf: &[]u8): MqttHeader;`
- `export fn encodePublish(topic: &[]u8, payload: &[]u8, qos: u8, retain: bool, packet_id: u16, dst: []u8): i64;`
- `export fn decodePublish(buf: &[]u8): MqttPublish;`
- `export fn encodeSimple(pkt_type: u8, dst: []u8): i64;`
- `export fn encodePuback(packet_id: u16, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`mqttbytes@1.0.0`)，许可证：MIT
