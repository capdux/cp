# `cap:http_method`

纯 CP 零堆分配 HTTP / WebDAV / QUERY 方法枚举（全部 36 种方法）与 RFC 9110 语义谓词库，吸收自 `/data/dev/bun/src/http_types/Method.cp`。

## 导出符号

- `HttpMethod`（36 变体：`GET`/`POST`/`PUT`/`DELETE`/`PATCH`/`HEAD`/`OPTIONS`/`CONNECT`/`TRACE`/`QUERY`/`M-SEARCH`/WebDAV 全集）：
  - `parse(input)`（大小写无关）、`asStr()`、`bit()`
  - `hasResponseBody()`、`hasRequestBody()`、`isSafe()`、`isIdempotent()`、`isCacheable()`、`isCorsSafelisted()`
- `MethodSet`：紧凑 64-bit 方法集合，提供 `empty()`、`insert()`、`remove()`、`contains()` 与 `formatAllowHeader(out)`
