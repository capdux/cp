[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/picohttp

CP 原生生态包 `@cap/picohttp`。

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`picohttpparser`](https://github.com/h2o/picohttpparser)（`MIT`）（原库名 `picohttpparser`，CP 包名 `@cap/picohttp`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/picohttp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/picohttp
```

```cp
import {
    PicoHttpError,
    phr_header,
    phr_chunked_decoder,
    ParsedHeader,
    ParsedRequest,
    ParsedResponse,
    HttpParser,
    ChunkedDecoder,
} from "@cap/picohttp";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `PicoHttpError` | `OutOfMemory, NotInitialized, ParseFailed, Incomplete, InvalidChunkedEncoding` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `phr_header` | `class` | 字段: `name: [*]&u8!?, name_len: usize, value: [*]&u8!?, value_len: usize` |
| `phr_chunked_decoder` | `class` | 字段: `bytes_left_in_chunk: usize, consume_trailer: u8, _hex_count: u8, _state: u8` |
| `ParsedHeader` | `class` | 字段: `name: &[]u8, value: &[]u8` |
| `ParsedRequest` | `class` | 字段: `method: &[]u8, path: &[]u8, minor_version: i32, header_count: usize, bytes_read: usize` |
| `ParsedResponse` | `class` | 字段: `minor_version: i32, status: i32, message: &[]u8, header_count: usize, bytes_read: usize` |
| `HttpParser` | `class` | 字段: `headers_ptr: [*]phr_header!?, max_headers: usize, allocator: std.mem.Allocator` · 方法: `init, parseRequest, parseResponse, headerAt, __cp_deinit` |
| `ChunkedDecoder` | `class` | 字段: `inner: phr_chunked_decoder` · 方法: `init, decode, isInData, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `phr_parse_request` | `(buf: [*]&u8!, len: usize, method: *[*]&u8!?!, method_len: *usize!, path: *[*]&u8!?!, path_len: *usize!, minor_version: *i32!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32` | 调用 `phr_parse_request` 并返回 `i32`。 |
| `phr_parse_response` | `(buf: [*]&u8!, len: usize, minor_version: *i32!, status: *i32!, msg: *[*]&u8!?!, msg_len: *usize!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32` | 调用 `phr_parse_response` 并返回 `i32`。 |
| `phr_decode_chunked` | `(decoder: *phr_chunked_decoder!, buf: [*]u8!, bufsz: *usize!): isize` | 调用 `phr_decode_chunked` 并返回 `isize`。 |
| `phr_decode_chunked_is_in_data` | `(decoder: *phr_chunked_decoder!): i32` | 调用 `phr_decode_chunked_is_in_data` 并返回 `i32`。 |

## 最小调用示例

```cp
import { phr_decode_chunked_is_in_data } from "@cap/picohttp";

pub fn main() {
    let result = phr_decode_chunked_is_in_data(/* decoder: *phr_chunked_decoder! */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
