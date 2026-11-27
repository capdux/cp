[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# http-errors

Zero-allocation HTTP 4xx/5xx error constructors and RFC 9457 application/problem+json serializer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:http-errors` ([https://github.com/jshttp/http-errors](https://github.com/jshttp/http-errors))

| 元信息 | 值 |
| --- | --- |
| 包名 | `http-errors` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add http-errors
```

```cp
import {
    HttpError,
    statusTitle,
    errorClassName,
    statusCodeName,
    createHttpError,
    badRequest,
    unauthorized,
    forbidden,
} from "http-errors";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HttpError` | `class` | 字段: `status: u16, code: string, title: string, detail: string, instance: string, expose: bool` · 方法: `withInstance, withExpose, withCode` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `statusTitle` | `(status: u16): string` | 调用 `statusTitle` 并返回 `string`。 |
| `errorClassName` | `(status: u16, out: []u8): usize` | 调用 `errorClassName` 并返回 `usize`。 |
| `statusCodeName` | `(status: u16): string` | 调用 `statusCodeName` 并返回 `string`。 |
| `createHttpError` | `(status: u16, detail: string): HttpError` | 创建或初始化实例，返回 `HttpError`。 |
| `badRequest` | `(detail: string): HttpError` | 调用 `badRequest` 并返回 `HttpError`。 |
| `unauthorized` | `(detail: string): HttpError` | 调用 `unauthorized` 并返回 `HttpError`。 |
| `forbidden` | `(detail: string): HttpError` | 调用 `forbidden` 并返回 `HttpError`。 |
| `notFound` | `(detail: string): HttpError` | 调用 `notFound` 并返回 `HttpError`。 |
| `methodNotAllowed` | `(detail: string): HttpError` | 调用 `methodNotAllowed` 并返回 `HttpError`。 |
| `conflict` | `(detail: string): HttpError` | 调用 `conflict` 并返回 `HttpError`。 |
| `gone` | `(detail: string): HttpError` | 调用 `gone` 并返回 `HttpError`。 |
| `payloadTooLarge` | `(detail: string): HttpError` | 调用 `payloadTooLarge` 并返回 `HttpError`。 |
| `unsupportedMediaType` | `(detail: string): HttpError` | 调用 `unsupportedMediaType` 并返回 `HttpError`。 |
| `unprocessableEntity` | `(detail: string): HttpError` | 调用 `unprocessableEntity` 并返回 `HttpError`。 |
| `tooManyRequests` | `(detail: string): HttpError` | 调用 `tooManyRequests` 并返回 `HttpError`。 |
| `internalServerError` | `(detail: string): HttpError` | 调用 `internalServerError` 并返回 `HttpError`。 |
| `notImplemented` | `(detail: string): HttpError` | 调用 `notImplemented` 并返回 `HttpError`。 |
| `badGateway` | `(detail: string): HttpError` | 调用 `badGateway` 并返回 `HttpError`。 |
| `serviceUnavailable` | `(detail: string): HttpError` | 调用 `serviceUnavailable` 并返回 `HttpError`。 |
| `gatewayTimeout` | `(detail: string): HttpError` | 调用 `gatewayTimeout` 并返回 `HttpError`。 |
| `isClientError` | `(err: HttpError): bool` | 执行 `isClientError` 条件判定，返回 `bool`。 |
| `isServerError` | `(err: HttpError): bool` | 执行 `isServerError` 条件判定，返回 `bool`。 |
| `isRetryable` | `(err: HttpError): bool` | 执行 `isRetryable` 条件判定，返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `hexDigitLower` | `(v: u8): u8` | 调用 `hexDigitLower` 并返回 `u8`。 |
| `appendJsonEscaped` | `(out: []u8, start_pos: usize, s: string): usize` | 调用 `appendJsonEscaped` 并返回 `usize`。 |
| `appendU16` | `(out: []u8, pos: usize, val: u16): usize` | 调用 `appendU16` 并返回 `usize`。 |
| `toProblemJson` | `(err: HttpError, include_hidden_detail: bool, out: []u8): string` | 调用 `toProblemJson` 并返回 `string`。 |

## 最小调用示例

```cp
import { createHttpError } from "http-errors";

pub fn main() {
    let result = createHttpError(0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
