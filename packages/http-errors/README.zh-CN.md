[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# http-errors

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTTP 4xx/5xx error constructors and RFC 9457 application/problem+json serializer for CP

## 安装与导入

```bash
cap add http-errors
```

```cp
import { statusTitle } from "http-errors";
```

## 核心导出 API

- `export fn statusTitle(status: u16): string;`
- `export fn errorClassName(status: u16, out: []u8): usize;`
- `export fn statusCodeName(status: u16): string;`
- `export fn createHttpError(status: u16, detail: string): HttpError;`
- `export fn badRequest(detail: string): HttpError;`
- `export fn unauthorized(detail: string): HttpError;`
- `export fn forbidden(detail: string): HttpError;`
- `export fn notFound(detail: string): HttpError;`
- `export fn methodNotAllowed(detail: string): HttpError;`
- `export fn conflict(detail: string): HttpError;`
- `export fn gone(detail: string): HttpError;`
- `export fn payloadTooLarge(detail: string): HttpError;`
- `export fn unsupportedMediaType(detail: string): HttpError;`
- `export fn unprocessableEntity(detail: string): HttpError;`
- `export fn tooManyRequests(detail: string): HttpError;`
- `export fn internalServerError(detail: string): HttpError;`
- `export fn notImplemented(detail: string): HttpError;`
- `export fn badGateway(detail: string): HttpError;`
- `export fn serviceUnavailable(detail: string): HttpError;`
- `export fn gatewayTimeout(detail: string): HttpError;`
- `export fn isClientError(err: HttpError): bool;`
- `export fn isServerError(err: HttpError): bool;`
- `export fn isRetryable(err: HttpError): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn hexDigitLower(v: u8): u8;`
- `export fn appendJsonEscaped(out: []u8, start_pos: usize, s: string): usize;`
- `export fn appendU16(out: []u8, pos: usize, val: u16): usize;`
- `export fn toProblemJson(err: HttpError, include_hidden_detail: bool, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`http-errors@1.0.0`)，许可证：MIT
