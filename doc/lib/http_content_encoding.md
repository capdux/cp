# cap:http_content_encoding

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 §8.4 `Content-Encoding` 规范），提供纯 CP、零外部依赖、零堆分配的压缩编码栈解析、迭代与层数安全校验库。

## 导入方式

```cp
import {
    ContentCoding,
    parseContentCoding,
    ContentEncodingIter,
    encodingLayerCount,
    isCompressed,
    hasContentEncoding,
    isValidContentEncoding,
} from "cap:http_content_encoding";
```

## 核心类型与 API

- `ContentCoding`：`identity`、`gzip`、`deflate`、`br`、`zstd`、`compress`、`unknown`（提供 `asStr()`）。
- `parseContentCoding(tok): ContentCoding`：大小写不敏感映射（`x-gzip`→`gzip`、`x-compress`→`compress`）。
- `ContentEncodingIter`：`init(header)` / `next(*_): ContentCoding?`。
- `encodingLayerCount(header): usize`：忽略 `identity` 统计实际压缩层数。
- `isCompressed(header): bool` 与 `hasContentEncoding(header, coding): bool`。
- `isValidContentEncoding(header, max_layers): bool`：拒绝 `unknown` 并限制最大压缩层数。

## 使用示例

```cp
import { ContentCoding, isValidContentEncoding, hasContentEncoding } from "cap:http_content_encoding";

fn example() {
    const hdr = "x-gzip, br";
    * = isValidContentEncoding(hdr, 2);                // true
    * = hasContentEncoding(hdr, ContentCoding.gzip);   // true
}
```
