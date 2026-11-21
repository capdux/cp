# cap:http_trailer

> 源码吸收自 `/data/dev/bun/src/http/`（RFC 9110 §6.5.1 HTTP `Trailer` 尾部字段声明解析与禁传字段安全校验规范），提供纯 CP、零外部依赖、零堆分配的 `Trailer` 头部解析、字段校验与查询库。

## 导入方式

```cp
import {
    isForbiddenTrailerField,
    isValidTrailerFieldName,
    TrailerFieldIter,
    validateTrailerHeader,
    hasTrailerField,
} from "cap:http_trailer";
```

## 核心类型与 API

- `isForbiddenTrailerField(name: []const u8): bool`：按 RFC 9110 §6.5.1 大小写不敏感拦截禁止出现在 HTTP Trailer 中的 25 个核心字段（`transfer-encoding`、`content-length`、`host`、`cache-control`、`max-forwards`、`te`、`authorization`、`set-cookie`、`content-encoding`、`content-type`、`content-range`、`trailer`、`connection`、`keep-alive`、`upgrade`、`proxy-authenticate`、`proxy-authorization`、`www-authenticate`、`age`、`expires`、`date`、`location`、`retry-after`、`vary`、`warning`）。
- `isValidTrailerFieldName(name: []const u8): bool`：校验字段名为合法 RFC 9110 `tchar` token 且不属于禁传 Trailer 字段。
- `TrailerFieldIter`：
  - `init(header: []const u8): TrailerFieldIter`
  - `next(*_): ([]const u8)?`：零分配解析逗号分隔的 `Trailer` 头字段列表，跳过空项、前后空白与非法 token，保留合法 token 切片。
- `validateTrailerHeader(header: []const u8): bool`：校验整条 `Trailer` 头部——每个非空项均为合法 `tchar` token 且不含任何禁传字段；空字符串、纯空白或全逗号返回 `false`。
- `hasTrailerField(header: []const u8, field_name: []const u8): bool`：大小写不敏感查询目标字段是否在 `Trailer` 头部中声明。

## 使用示例

```cp
import {
    isForbiddenTrailerField,
    isValidTrailerFieldName,
    TrailerFieldIter,
    validateTrailerHeader,
    hasTrailerField,
} from "cap:http_trailer";

fn example() {
    const hdr = "X-Checksum-Sha256, Server-Timing, Grpc-Status";
    * = validateTrailerHeader(hdr); // true
    * = hasTrailerField(hdr, "grpc-status"); // true
    * = isForbiddenTrailerField("Transfer-Encoding"); // true
    * = isValidTrailerFieldName("X-Signature"); // true

    let it = TrailerFieldIter.init(hdr);
    while (field := it.next()) {
        * = field.len;
    }
}
```
