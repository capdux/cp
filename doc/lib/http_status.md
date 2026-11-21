# `cap:http_status` — 纯 CP HTTP 状态码短语与语义分类库

## 概述

`cap:http_status` 从 Bun `src/runtime/server/HTTPStatusText.cp` 吸收并增强，提供零分配、O(1) 的标准 RFC 9110 / IANA HTTP 状态码原因短语查找表（覆盖 100–599 全部 63 个标准状态码）、状态行片段生成、五类状态归类以及消息体允许性、默认缓存性与瞬态可重试性判定。

## 核心类型与 API

### 枚举

- `StatusClass`：HTTP 状态码大类枚举
  - `Informational`（100–199）
  - `Success`（200–299）
  - `Redirection`（300–399）
  - `ClientError`（400–499）
  - `ServerError`（500–599）
  - `Unknown`（< 100 或 > 599）

### 状态文本查找

- `statusText(code: u16): string?` — 返回标准原因短语（如 `"OK"`、`"Not Found"`），未知返回 `null`。
- `statusTextOrEmpty(code: u16): string` — 返回标准原因短语，未知返回 `""`。
- `statusLine(code: u16): string?` — 返回状态行片段（如 `"200 OK"`、`"404 Not Found"`），未知返回 `null`。

### 分类与语义判定

- `classify(code: u16): StatusClass` — 返回状态码所属大类。
- `isInformational(code: u16): bool` — 是否为 1xx（100..199）。
- `isSuccess(code: u16): bool` — 是否为 2xx（200..299）。
- `isRedirect(code: u16): bool` — 是否为 3xx（300..399）。
- `isClientError(code: u16): bool` — 是否为 4xx（400..499）。
- `isServerError(code: u16): bool` — 是否为 5xx（500..599）。
- `isError(code: u16): bool` — 是否为错误响应（400..599）。
- `canHaveBody(code: u16): bool` — 按 RFC 9110 §6.4.1 判定响应是否允许携带消息体（1xx / 204 / 304 / 越界码返回 `false`）。
- `isCacheableByDefault(code: u16): bool` — 按 RFC 9110 §15.1 判定是否默认可启发式缓存（200/203/204/206/300/301/308/404/405/410/414/501）。
- `isRetryable(code: u16): bool` — 判定是否为典型瞬态可重试状态码（408/425/429/500/502/503/504）。

## 示例

```cp
import {
    StatusClass,
    statusText,
    statusTextOrEmpty,
    statusLine,
    classify,
    isSuccess,
    canHaveBody,
    isCacheableByDefault,
    isRetryable,
} from "cap:http_status";

if (line := statusLine(200)) {
    // line == "200 OK"
}
const phrase = statusTextOrEmpty(404); // "Not Found"
const cls = classify(503);             // StatusClass.ServerError
const retry = isRetryable(503);        // true
const body_ok = canHaveBody(204);      // false
const cached = isCacheableByDefault(301); // true
```
