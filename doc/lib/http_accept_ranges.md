# cap:http_accept_ranges

> 源码吸收自 `/data/dev/bun/src/http/`（RFC 9110 §14.3 HTTP `Accept-Ranges` 响应头规范），提供纯 CP、零外部依赖、零堆分配的 `Accept-Ranges` 解析、分片单位迭代与能力判定库。

## 导入方式

```cp
import {
    AcceptRangesKind,
    AcceptRanges,
    AcceptRangesIter,
    parseAcceptRanges,
    supportsByteRanges,
    supportsRangeUnit,
} from "cap:http_accept_ranges";
```

## 核心类型与 API

- `AcceptRangesKind`：枚举类型，包含 `none`、`bytes`、`custom`、`invalid`。
- `AcceptRanges`：结构体，包含 `kind: AcceptRangesKind`、`supports_bytes: bool`、`has_custom: bool`。
- `AcceptRangesIter`：
  - `init(header: []const u8): AcceptRangesIter`
  - `next(*_): ([]const u8)?`：零分配遍历逗号分隔的合法 RFC 9110 `range-unit` token。
- `parseAcceptRanges(header: []const u8): AcceptRanges`：严格按 RFC 9110 §14.3 解析；`none`（大小写不敏感）不可与其他单位混用或重复，混用判 `invalid`；包含 `bytes` 置 `supports_bytes = true`；空头、全逗号或含非法 token 判 `invalid`。
- `supportsByteRanges(header: []const u8): bool`：快捷判定头部是否合法且支持 `bytes` 分片。
- `supportsRangeUnit(header: []const u8, unit: []const u8): bool`：大小写不敏感查询是否支持指定分片单位，对 `invalid`、`none` 或非法单位名均返回 `false`。

## 使用示例

```cp
import {
    AcceptRangesKind,
    parseAcceptRanges,
    supportsByteRanges,
    supportsRangeUnit,
} from "cap:http_accept_ranges";

fn example() {
    const parsed = parseAcceptRanges("bytes, items");
    * = parsed.kind == AcceptRangesKind.bytes; // true
    * = parsed.has_custom; // true
    * = supportsByteRanges("bytes"); // true
    * = supportsRangeUnit("bytes, items", "ITEMS"); // true
}
```
