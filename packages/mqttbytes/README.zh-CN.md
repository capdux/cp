[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mqttbytes

Zero-heap OASIS MQTT v3.1.1 binary control packet encoder/decoder with 1..4-byte Remaining Length, PUBLISH/PUBACK/PINGREQ/PINGRESP/DISCONNECT, and partial-frame detection.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mqttbytes` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `mqttbytes` → CP 包名 `mqttbytes`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mqttbytes` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mqttbytes
```

```cp
import {
    MqttHeader,
    MqttPublish,
    makeHeaderStatus,
    makePublishStatus,
    encodeRemainingLen,
    decodeHeader,
    encodePublish,
    decodePublish,
} from "mqttbytes";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MqttHeader` | `class` | 字段: `status: i32, pkt_type: u8, flags: u8, remaining_len: u32, header_len: usize, total_len: usize` |
| `MqttPublish` | `class` | 字段: `status: i32, qos: u8, retain: bool, dup: bool, packet_id: u16, topic_start: usize, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeHeaderStatus` | `(status: i32): MqttHeader` | 调用 `makeHeaderStatus` 并返回 `MqttHeader`。 |
| `makePublishStatus` | `(status: i32): MqttPublish` | 调用 `makePublishStatus` 并返回 `MqttPublish`。 |
| `encodeRemainingLen` | `(len: u32, dst: []u8): i64` | 通过 `encodeRemainingLen` 编码或格式化输出，返回 `i64`。 |
| `decodeHeader` | `(buf: &[]u8): MqttHeader` | 通过 `decodeHeader` 解析输入数据，返回 `MqttHeader`。 |
| `encodePublish` | `(topic: &[]u8, payload: &[]u8, qos: u8, retain: bool, packet_id: u16, dst: []u8): i64` | 通过 `encodePublish` 编码或格式化输出，返回 `i64`。 |
| `decodePublish` | `(buf: &[]u8): MqttPublish` | 通过 `decodePublish` 解析输入数据，返回 `MqttPublish`。 |
| `encodeSimple` | `(pkt_type: u8, dst: []u8): i64` | 通过 `encodeSimple` 编码或格式化输出，返回 `i64`。 |
| `encodePuback` | `(packet_id: u16, dst: []u8): i64` | 通过 `encodePuback` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { encodeRemainingLen } from "mqttbytes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeRemainingLen(0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
