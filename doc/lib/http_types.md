# `cap:http_types` — 纯 CP HTTP 核心类型与 ETag / URLPath 解析引擎（吸收自 Bun `http_types`）

## 概述

`cap:http_types`（亦支持 `cap.http_types` 动态挂载）从 Bun `src/http_types/` 吸收并纯 CP 化：包含 36 种 HTTP `Method`（`hasBody` / `hasRequestBody` / `isIdempotent` / 大小写不敏感 `find`）、`Encoding`、`FetchCacheMode`、`FetchRedirect`、`FetchRequestMode`、RFC 7232 `ETag` 解析/弱匹配与 `computeETag`、以及零堆 `URLPath` 路径/扩展名/SourceMap/查询串解析。

## 核心类型与 API

- `Method`: 36 种 HTTP 方法枚举与 `find(str)` / `hasBody()` / `hasRequestBody()` / `isIdempotent()`
- `Encoding` / `FetchCacheMode` / `FetchRedirect` / `FetchRequestMode`
- `ParsedETag` / `parseETag(s)` / `computeETag(bytes, &buf)` / `ifNoneMatch(etag, header)`
- `URLPath`: `parse(pathname)` / `isRoot(prefix)` / `pathWithoutAssetPrefix(prefix)`

## 示例

```cp
import { Method, ifNoneMatch, URLPath } from "cap:http_types";

const m = Method.find("post").?;
const matched = ifNoneMatch("\"abc\"", "W/\"abc\", \"def\"");
const p = URLPath.parse("/assets/app.js.map?v=1");
```
