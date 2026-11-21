# `cap:http_fetch_meta` — 纯 CP Fetch 规范元数据、HTTP 内容协商与零拷贝 URL 路径拆分库

## 概述

`cap:http_fetch_meta` 吸收自 Bun `src/http_types/{Encoding,FetchCacheMode,FetchRedirect,FetchRequestMode,URLPath}.cp`，纯 CP 零外部依赖、零堆分配，提供 RFC 9110 `Accept-Encoding` 权重协商、WHATWG Fetch 枚举双向解析以及零拷贝 `UrlPath` 路径/查询/锚点/扩展名拆分。

## 核心类型与 API

- `ContentEncoding`（`identity`/`gzip`/`deflate`/`br`/`zstd`）+ `ENC_MASK_*` 位掩码 + `negotiateEncoding(accept_encoding, supported_mask)`
- `FetchCacheMode`（`default_mode`/`no_store`/`reload`/`no_cache`/`force_cache`/`only_if_cached`）
- `FetchRedirect`（`follow`/`error_mode`/`manual`）
- `FetchRequestMode`（`navigate`/`same_origin`/`no_cors`/`cors`）
- `UrlPath`：`parse(raw)`（返回 `{ path, query, fragment, extname, basename }`）与 `isNormalized()`

## 示例

```cp
import { negotiateEncoding, ENC_MASK_ALL, UrlPath } from "cap:http_fetch_meta";

const enc = negotiateEncoding("zstd;q=0.9, br;q=1.0, gzip;q=0.5", ENC_MASK_ALL); // ContentEncoding.br
const up = UrlPath.parse("/assets/app.min.js?v=2#init");
// up.basename == "app.min.js", up.extname == ".js", up.query.? == "v=2", up.fragment.? == "init"
```
