// Source: cp/doc/lib/picohttp.md
# `cap:picohttp` — 纯 CP 零堆分配 HTTP/1.1 & Chunked 流式解析器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

纯 CP、零 C 依赖、零堆分配（Zero-Allocation）的高性能 HTTP/1.1 请求/响应/头部与 Chunked Transfer-Encoding 流式解析标准库。所有解析结果均以只读切片视图直接指向调用方输入缓冲区与预先提供的 `HttpHeader` 切片数组。

## 导入方式

```cp
import {
    PicoError,
    HttpHeader,
    ParsedRequest,
    ParsedResponse,
    ParsedHeaders,
    ChunkedState,
    ChunkDecodeResult,
    ChunkedDecoder,
    parseRequest,
    parseResponse,
    parseHeaders,
    findHeader,
    parseContentLength,
} from "cap:web";

// 或动态命名空间挂载：
import cap from "cap";
// cap.picohttp.parseRequest / cap.picohttp.ChunkedDecoder
```

## 核心类型与错误集

### `PicoError`
- `Incomplete`：输入缓冲区尚未包含完整的请求行、状态行或头部结束标记（`\r\n\r\n`）
- `InvalidMethod`：请求方法为空或包含非 RFC 9110 `tchar` 字符
- `InvalidTarget`：请求目标为空或包含控制字符 / 非法空白
- `InvalidVersion`：HTTP 版本不是 `HTTP/1.0` 或 `HTTP/1.1`，或响应状态码不是合法 3 位数字（`100..999`）
- `InvalidHeaderName`：头部名称为空、包含非 `tchar` 字符（含冒号前空白）或首行即出现 obs-fold
- `InvalidHeaderValue`：头部值包含 `\0`、裸 `\r`、裸 `\n`、DEL（`0x7F`）或其他非法控制字符；或 `Content-Length` 多值冲突
- `TooManyHeaders`：头部条目数超出调用方传入的 `headers_out` 容量
- `HeaderTooLarge`：单行头部超过 `MAX_HEADER_LINE_LEN`（16 KiB）或总头部块超过 `MAX_HEADERS_BYTES`（64 KiB）
- `InvalidChunkSize`：Chunked 块长度非合法十六进制、缺失 CRLF 或非法 chunk-ext 控制字符
- `ChunkOverflow`：Chunked 块长度超出 `usize` 表示范围

### 核心数据结构
- `HttpHeader { name: []const u8, value: []const u8 }`：`isMultiline()` 在 `name.len == 0`（obs-fold 续行）时返回 `true`，`value` 自动去除前后 OWS（空格与制表符）。
- `ParsedRequest { method, path, minor_version, headers, bytes_consumed }`
- `ParsedResponse { minor_version, status, reason, headers, bytes_consumed }`
- `ParsedHeaders { headers, bytes_consumed }`
- `ChunkDecodeResult { bytes_written, bytes_consumed, done }`

## 解析原语

- `parseRequest(buf: []const u8, headers_out: []HttpHeader): ParsedRequest ! PicoError`
  解析 HTTP/1.0 与 HTTP/1.1 请求行及全部头部，返回 `bytes_consumed` 以支持 HTTP Pipelining 连续报文切分。
- `parseResponse(buf: []const u8, headers_out: []HttpHeader): ParsedResponse ! PicoError`
  解析 HTTP/1.0 与 HTTP/1.1 响应状态行（校验 `100..999` 三位状态码）与头部。
- `parseHeaders(buf: []const u8, headers_out: []HttpHeader): ParsedHeaders ! PicoError`
  独立解析头部块（适用于 Chunked Trailer 或分段头解析）。

## Chunked 流式解码器 (`ChunkedDecoder`)

- `ChunkedDecoder.init(consume_trailer: bool = true): ChunkedDecoder`
- `decode(*_, buf: []u8): ChunkDecodeResult ! PicoError`：原地零拷贝紧缩解码，`buf[0..bytes_written]` 为解码后的连续载荷，`bytes_consumed` 为消耗的输入字节数。
- `decodeTo(*_, src: []const u8, dst: []u8): ChunkDecodeResult ! PicoError`：非破坏性流式解码到目标切片 `dst`。
- `isInData(): bool` / `isDone(): bool` / `reset(*_)`
- 完整支持任意 `1..N` 字节半包跨分片连续喂入、`;chunk-ext` 扩展参数跳过、`0\r\n` 终结块及可选 Trailer 头部跳过。

## 安全辅助函数

- `findHeader(headers: []const HttpHeader, name: []const u8): ([]const u8)?`：ASCII 大小写无关查找头部值。
- `parseContentLength(headers: []const HttpHeader): (usize?) ! PicoError`：解析并校验 `Content-Length`，拦截非法数字、溢出及多 `Content-Length` 不一致（防请求走私）。
