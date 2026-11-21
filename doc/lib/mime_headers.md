# cap:mime_headers

> 源码吸收自 `/data/dev/bun/src/http/MimeType.zig`（RFC 5322 §2.2 / §2.2.3 规范），提供纯 CP、零外部依赖、零堆分配的 MIME / 邮件头块解析、折行展开（unfolding）与头体分离库。

## 导入方式

```cp
import {
    MimeMessageSplit,
    MimeHeader,
    splitHeadersAndBody,
    MimeHeaderIter,
    findMimeHeader,
    countMimeHeader,
} from "cap:mime_headers";
```

## 核心类型与 API

- `MimeMessageSplit` & `splitHeadersAndBody(raw)`: 分离头块与正文（兼容 `\r\n\r\n` 与 `\n\n`，首行即空行时 `headers_raw` 为空）。
- `MimeHeader`：`name: []const u8`、`value: []const u8`。
- `MimeHeaderIter`：`init(headers_raw)`、`next(*_, val_buf): MimeHeader?`、`hasError(): bool`（自动将 `\r\n[ \t]+` / `\n[ \t]+` 续行折叠为单空格，校验合法字段名 ASCII 33..126 且不含 `:`）。
- `findMimeHeader(headers_raw, name, val_buf): ([]const u8)?`：大小写不敏感查找首个匹配头并展开折行。
- `countMimeHeader(headers_raw, name): usize`：零缓冲统计同名头出现次数（如多条 `Received:`）。

## 使用示例

```cp
import { splitHeadersAndBody, findMimeHeader, countMimeHeader } from "cap:mime_headers";

fn example() {
    const msg = "Subject: Hello\r\n\tWorld\r\nReceived: a\r\nReceived: b\r\n\r\nBody";
    const parts = splitHeadersAndBody(msg);
    let buf: [128]u8 = undefined;
    * = findMimeHeader(parts.headers_raw, "subject", &buf); // "Hello World"
    * = countMimeHeader(parts.headers_raw, "received");     // 2
}
```
