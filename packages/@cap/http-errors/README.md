# @cap/http-errors

Zero-allocation HTTP 4xx/5xx error constructors and RFC 9457 `application/problem+json` serializer for CP — clean-room MIT parity with npm `http-errors` and RFC 9457.

零堆分配的 CP HTTP 标准错误构造与 RFC 9457 `application/problem+json` 序列化引擎，4xx 默认 `expose = true`、5xx 默认 `expose = false` 防内部堆栈泄漏，内置全部常用 4xx/5xx 快捷构造器与安全 JSON 转义（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/http-errors
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `HttpError` | `pub struct { status, code, title, detail, instance, expose }` | HTTP 错误结构体，提供 `withInstance`、`withExpose`、`withCode` |
| `createHttpError` | `(status: u16, detail: string) -> HttpError` | 通用 HTTP 错误构造器（非法状态码自动回退 `500`） |
| 4xx/5xx Constructors | `badRequest`, `unauthorized`, `forbidden`, `notFound`, `methodNotAllowed`, `conflict`, `gone`, `payloadTooLarge`, `unsupportedMediaType`, `unprocessableEntity`, `tooManyRequests`, `internalServerError`, `notImplemented`, `badGateway`, `serviceUnavailable`, `gatewayTimeout` | 16 个标准快捷构造器 |
| `isClientError` / `isServerError` / `isRetryable` | `(err: HttpError) -> bool` | 语义判断（`isRetryable` 对 408/429/502/503/504 返回 `true`） |
| `toProblemJson` | `(err: HttpError, include_hidden_detail: bool, out: []u8) -> string` | RFC 9457 `application/problem+json` 零堆序列化与安全脱敏 |

## Usage / 使用示例

```cp
import { notFound, internalServerError, toProblemJson } from "@cap/http-errors";

let out: [512]u8 = undefined;
const err404 = notFound("User 42 not found").withInstance("/api/users/42");
const json404 = toProblemJson(err404, false, out[0..512]);
// => {"type":"about:blank","title":"Not Found","status":404,"code":"NOT_FOUND","detail":"User 42 not found","instance":"/api/users/42"}

const err500 = internalServerError("pg: password auth failed");
const safe500 = toProblemJson(err500, false, out[0..512]);
// detail is automatically hidden because err500.expose == false!
```

## License

MIT
