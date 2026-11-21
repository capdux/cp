# cap:http_content_range

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 §14.4 HTTP `Content-Range` 响应头规范），提供纯 CP、零外部依赖、零堆分配的断点续传 `Content-Range` 解析与格式化库。

## 导入方式

```cp
import {
    ContentRangeKind,
    ContentRange,
    parseContentRange,
    formatContentRange,
} from "cap:http_content_range";
```

## 核心类型与 API

- `ContentRangeKind`：`range`、`unsatisfied`。
- `ContentRange`：
  - 字段：`unit: []const u8`、`kind: ContentRangeKind`、`start: u64`、`end: u64`、`complete_length: u64?`（`*` 时为 `null`）。
  - `isBytes(): bool`：大小写不敏感匹配 `"bytes"`。
  - `sliceLength(): u64?`：`range` 模式下返回 `end - start + 1`，`unsatisfied` 或溢出返回 `null`。
  - `isComplete(): bool`：`range` 模式且 `start == 0` 且 `complete_length == end + 1`。
- `parseContentRange(header: []const u8): ContentRange?`：支持 `bytes 0-499/1234`、`bytes 0-499/*`、`bytes */1234`；严格拒绝 `bytes */*`、`start > end`、`end >= complete_length`、整数溢出与尾随垃圾。
- `formatContentRange(cr: ContentRange, out: []u8): ([]const u8)?`：校验语义合法性后规范化输出，非法状态或缓冲不足返回 `null`。

## 使用示例

```cp
import { parseContentRange, formatContentRange } from "cap:http_content_range";

fn example() {
    if (cr := parseContentRange("bytes 0-499/1234")) {
        * = cr.sliceLength(); // 500
        let buf: [64]u8 = undefined;
        * = formatContentRange(cr, &buf); // "bytes 0-499/1234"
    }
}
```
