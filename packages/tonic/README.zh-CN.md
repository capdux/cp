[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tonic

Zero-allocation gRPC over HTTP/2 length-prefixed frame codec, 17 status codes, and grpc-timeout parser/formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tonic` ([https://github.com/hyperium/tonic](https://github.com/hyperium/tonic)) / `npm:@grpc/grpc-js` ([https://github.com/grpc/grpc-node](https://github.com/grpc/grpc-node))（原库名 `tonic` → CP 包名 `tonic`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `tonic` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tonic
```

```cp
import {
    GrpcFrameHeader,
    statusName,
    isRetryableStatus,
    encodeFrame,
    decodeHeader,
    parseTimeoutMicros,
    formatTimeoutMillis,
} from "tonic";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GrpcFrameHeader` | `class` | 字段: `compressed: bool, payload_len: u32, total_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `statusName` | `(code: u8): string` | 调用 `statusName` 并返回 `string`。 |
| `isRetryableStatus` | `(code: u8): bool` | 执行 `isRetryableStatus` 条件判定，返回 `bool`。 |
| `encodeFrame` | `(compressed: bool, payload: &[]u8, out: []u8): usize` | 通过 `encodeFrame` 编码或格式化输出，返回 `usize`。 |
| `decodeHeader` | `(data: &[]u8, out: &mut GrpcFrameHeader): i32` | 通过 `decodeHeader` 解析输入数据，返回 `i32`。 |
| `parseTimeoutMicros` | `(header: string): i64` | 通过 `parseTimeoutMicros` 解析输入数据，返回 `i64`。 |
| `formatTimeoutMillis` | `(ms: u64, out: []u8): usize` | 通过 `formatTimeoutMillis` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseTimeoutMicros } from "tonic";

pub fn main() {
    let result = parseTimeoutMicros("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
