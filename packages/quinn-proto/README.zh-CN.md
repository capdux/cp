[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# quinn-proto

Pure CP zero-allocation RFC 9000 QUIC variable-length integer codec and Long Header parser (quinn-proto parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:quinn-proto` ([https://github.com/quinn-rs/quinn](https://github.com/quinn-rs/quinn)) / `crates.io:quinn` ([https://github.com/quinn-rs/quinn](https://github.com/quinn-rs/quinn))（原库名 `quinn-proto` → CP 包名 `quinn-proto`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `quinn-proto` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add quinn-proto
```

```cp
import {
    QuicVarint,
    QuicLongHeader,
    encodedLen,
    encodeVarint,
    decodeVarint,
    qvInvalidHeader,
    parseLongHeader,
} from "quinn-proto";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `QuicVarint` | `class` | 字段: `val: u64, bytes_read: usize, ok: bool` |
| `QuicLongHeader` | `class` | 字段: `is_long: bool, packet_type: u8, version: u32, dcid_len: u8, scid_len: u8, header_len: usize, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodedLen` | `(val: u64): usize` | 通过 `encodedLen` 编码或格式化输出，返回 `usize`。 |
| `encodeVarint` | `(val: u64, out: []u8): usize` | 通过 `encodeVarint` 编码或格式化输出，返回 `usize`。 |
| `decodeVarint` | `(buf: []u8): QuicVarint` | 通过 `decodeVarint` 解析输入数据，返回 `QuicVarint`。 |
| `qvInvalidHeader` | `(): QuicLongHeader` | 调用 `qvInvalidHeader` 并返回 `QuicLongHeader`。 |
| `parseLongHeader` | `(pkt: []u8): QuicLongHeader` | 通过 `parseLongHeader` 解析输入数据，返回 `QuicLongHeader`。 |

## 最小调用示例

```cp
import { encodedLen } from "quinn-proto";

pub fn main() {
    let result = encodedLen(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
