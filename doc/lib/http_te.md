# `cap:http_te` — RFC 9110/9112 `Transfer-Encoding` 与 `TE` 头部校验库

> 来源：吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 `Transfer-Encoding` & `TE` Parser）

`cap:http_te` 提供纯 CP、零外部依赖、零堆分配的 HTTP `Transfer-Encoding` 传输编码解析、RFC 9112 §6.1/§6.3 异常报文边界校验及 `TE` 请求头 `trailers` 协商解析能力。

## 核心类型与函数

### `TransferCoding` & `TransferEncodingValidation`

```cp
pub enum TransferCoding: u8 {
    chunked = 0,
    gzip = 1,
    deflate = 2,
    compress = 3,
    br = 4,
    zstd = 5,
    unknown = 6,

    static fn parse(tok: []const u8): TransferCoding
    fn asStr(): string
}

pub enum TransferEncodingValidation: u8 {
    valid = 0,
    empty = 1,
    duplicate_chunked = 2,
    chunked_not_final = 3,
    unknown_coding = 4,

    fn asStr(): string
}
```

### 迭代器与校验函数

```cp
pub struct TransferEncodingIter {
    static fn init(header: []const u8): TransferEncodingIter
    fn next(*_): ([]const u8)?
}

pub fn validateTransferEncoding(header: []const u8, require_chunked_final: bool): TransferEncodingValidation
pub fn isChunkedTransfer(header: []const u8): bool
pub fn hasConflictingClAndTe(has_content_length: bool, transfer_encoding: ([]const u8)?): bool
pub fn teAcceptsTrailers(te_header: []const u8): bool
```

- `validateTransferEncoding`：遵循 RFC 9112 §6.1/§6.3，拒绝空值、重复 `chunked`、`chunked` 非末项以及未知编码。
- `isChunkedTransfer`：判断末项是否为唯一合法的 `chunked`（大小写无关）。
- `hasConflictingClAndTe`：检测同时存在 `Content-Length` 与 `Transfer-Encoding` 的异常报文。
- `teAcceptsTrailers`：解析 `TE` 头是否接受 `trailers`，支持 `;q=...` 权重并拒绝 `q=0`。

## 使用示例

```cp
import { validateTransferEncoding, isChunkedTransfer, teAcceptsTrailers } from "cap:http_te";

// validateTransferEncoding("gzip, chunked", true) == .valid
// validateTransferEncoding("chunked, gzip", false) == .chunked_not_final
// isChunkedTransfer("gzip, CHUNKED") == true
// teAcceptsTrailers("gzip;q=0.5, trailers") == true
```
