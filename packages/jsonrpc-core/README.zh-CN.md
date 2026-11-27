[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jsonrpc-core

Zero-heap JSON-RPC 2.0 request/notification/result/error encoder, envelope validator, batch detector, and standard error codes.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:jsonrpc-core` ([https://github.com/paritytech/jsonrpc](https://github.com/paritytech/jsonrpc)) / `npm:json-rpc-2.0` ([https://github.com/shawninder/json-rpc-2.0](https://github.com/shawninder/json-rpc-2.0)) / `crates.io:jsonrpsee` ([https://github.com/paritytech/jsonrpsee](https://github.com/paritytech/jsonrpsee))（原库名 `jsonrpc-core` → CP 包名 `jsonrpc-core`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `jsonrpc-core` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jsonrpc-core
```

```cp
import {
    isServerError,
    errorMessage,
    isWs,
    appendByte,
    appendSlice,
    appendI32,
    appendJsonString,
    encodeRequest,
} from "jsonrpc-core";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isServerError` | `(code: i32): bool` | 执行 `isServerError` 条件判定，返回 `bool`。 |
| `errorMessage` | `(code: i32): string` | 调用 `errorMessage` 并返回 `string`。 |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, b: u8): i64` | 调用 `appendByte` 并返回 `i64`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `appendI32` | `(out: []u8, pos: usize, v: i32): i64` | 调用 `appendI32` 并返回 `i64`。 |
| `appendJsonString` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendJsonString` 并返回 `i64`。 |
| `encodeRequest` | `(id_json: string, method: string, params_json: string, out: []u8): usize` | 通过 `encodeRequest` 编码或格式化输出，返回 `usize`。 |
| `encodeResult` | `(id_json: string, result_json: string, out: []u8): usize` | 通过 `encodeResult` 编码或格式化输出，返回 `usize`。 |
| `encodeError` | `(id_json: string, code: i32, message: string, data_json: string, out: []u8): usize` | 通过 `encodeError` 编码或格式化输出，返回 `usize`。 |
| `isBatch` | `(json: string): bool` | 执行 `isBatch` 条件判定，返回 `bool`。 |
| `isValidEnvelope` | `(json: string): bool` | 执行 `isValidEnvelope` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { encodeResult } from "jsonrpc-core";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeResult("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
